        ; YOS-native filesystem primitives implemented on NextZXOS' native
        ; +3DOS/IDEDOS ROM API (see p3dos_core.s).
        ;
        ; The entry points, register contracts and native result formats are
        ; those of the native filesystem calls they replace, so the POSIX layer above is
        ; unchanged: success is carry clear; failure is carry set with A a
        ; native filesystem error number (mapped from the +3DOS result code).
        ; Native handles: 1..14 are +3DOS file numbers (15 is reserved for
        ; stat), 0x20.. are directory slots. File positions and sizes are
        ; tracked here, so reads never run into end of file.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module  native_calls
        .optsdcc -mz80 sdcccall(1)

        .globl  __next_fs_real_f_open
        .globl  __next_fs_real_f_close
        .globl  __next_fs_real_f_sync
        .globl  __next_fs_real_f_read
        .globl  __next_fs_real_f_write
        .globl  __next_fs_real_f_seek
        .globl  __next_fs_real_f_fgetpos
        .globl  __next_fs_real_f_fstat
        .globl  __next_fs_real_f_getcwd
        .globl  __next_fs_real_f_chdir
        .globl  __next_fs_real_f_mkdir
        .globl  __next_fs_real_f_rmdir
        .globl  __next_fs_real_f_stat
        .globl  __next_fs_real_f_unlink
        .globl  __next_fs_real_f_rename
        .globl  __next_fs_real_f_opendir
        .globl  __next_fs_real_f_readdir
        .globl  __next_fs_real_f_rewinddir
        .globl  __next_fs_real_disk_info
        .globl  __next_fs_real_m_execcmd

        .globl  __p3_drive
        .globl  __p3_invoke
        .globl  __p3_id, __p3_bank, __p3_af, __p3_bc, __p3_de, __p3_hl, __p3_ix
        .globl  __p3_oaf, __p3_obc, __p3_ode, __p3_ohl, __p3_oix
        .globl  __p3_stage, __p3_pa, __p3_pb, __p3_lfn, __p3_cat
        .globl  __p3_ft, __p3_dirs
        .globl  __p3_n, __p3_n2, __p3_dst, __p3_rem, __p3_chunk, __p3_done
        .globl  __p3_t0, __p3_t1
        .globl  _enter_critical_section, _leave_critical_section
        .globl  __sys_heap, _mem_allocate, __os_free

        ; +3DOS call ids
        .equ    DOS_OPEN,         0x0106
        .equ    DOS_CLOSE,        0x0109
        .equ    DOS_READ,         0x0112
        .equ    DOS_WRITE,        0x0115
        .equ    DOS_CATALOG,      0x011e
        .equ    DOS_DELETE,       0x0124
        .equ    DOS_RENAME,       0x0127
        .equ    DOS_SET_POSITION, 0x0136
        .equ    DOS_GET_EOF,      0x0139
        .equ    DOS_FLUSH,        0x0142
        .equ    IDE_PATH,         0x01b1
        .equ    IDE_GET_LFN,      0x01b7

        .equ    STAGE_HI,   2           ; staging buffer = 512 bytes
        .equ    STAGE_SIZE, 512
        .equ    FILE_SIZE,  9           ; used, pos(4), size(4)
        .equ    TEMP_FILE,  15

        ; directory slot
        .equ    DIR_SIZE,   361
        .equ    D_USED,     0
        .equ    D_EOF,      1
        .equ    D_QCOUNT,   2
        .equ    D_QINDEX,   3
        .equ    D_DOTS,     4
        .equ    D_DOTS0,    5
        .equ    D_PRELOAD,  6           ; 13 bytes
        .equ    D_SPEC,     19          ; 130 bytes, $FF terminated
        .equ    D_QUEUE,    149         ; 6 records of 22 bytes
        .equ    D_RAW,      281         ; six original catalog records
        .equ    D_HANDLE,   359         ; IDE_GET_LFN directory handle
        .equ    D_SPEC_MAX, 126         ; path characters kept in a spec
        .equ    DIR_BASE,   0x20
        .equ    DIR_COUNT,  8
        .equ    REC_SIZE,   22

        .area   _CODE

        ; ---------------------------------------------------------------
        ; Helpers
        ; ---------------------------------------------------------------

        ; Map a +3DOS result code in A to an native filesystem native error number.
.p3_map_error:
        cp      #75
        jr      nc,.p3_map_other
        ld      e,a
        ld      d,#0
        ld      hl,#.p3_errors
        add     hl,de
        ld      a,(hl)
        ret
.p3_map_other:
        ld      a,#1
        ret

        ; Issue the call prepared in the parameter block; DE = call id.
        ; Carry set + A = native error on failure, carry clear on success.
.p3_call:
        ld      (__p3_id),de
        call    __p3_invoke
        ld      hl,(__p3_oaf)
        bit     0,l
        jr      nz,.p3_ok
        ld      a,h
        call    .p3_map_error
        scf
        ret
.p3_ok:
        or      a
        ret

        ; Clear the entry parameters and select RAM 7. Clobbers A, HL.
.p3_clear:
        ld      hl,#0
        ld      (__p3_af),hl
        ld      (__p3_bc),hl
        ld      (__p3_de),hl
        ld      (__p3_hl),hl
        ld      (__p3_ix),hl
        ld      a,#7
        ld      (__p3_bank),a
        ret

        ; A = drive code (0x40 hd0, 0x48 hd1, else default), HL = NUL
        ; terminated path, DE = destination. Writes the optional drive
        ; letter, the path and an $FF terminator.
        ; Returns HL = start, DE = address of the terminator. Clobbers AF.
.p3_pbuild:
        push    de
        cp      #0x40
        jr      z,.p3_drv_c
        cp      #0x48
        jr      z,.p3_drv_d
        ; ALTO names the SD card A:/ and B:/ in paths: emit C:/D: for them
        ld      a,(hl)
        and     #0xdf
        sub     #'A'
        cp      #4
        jr      nc,.p3_drv_default
        push    af
        inc     hl
        ld      a,(hl)
        dec     hl
        cp      #':'
        jr      z,.p3_pfx
        pop     af
        jr      .p3_drv_default
.p3_pfx:
        inc     hl
        inc     hl
        pop     af
        cp      #2
        jr      c,.p3_pfx_alto
        add     a,#'A'
        jr      .p3_drv
.p3_pfx_alto:
        add     a,#'C'
        jr      .p3_drv
.p3_drv_default:
        ld      a,(__p3_drive)
        jr      .p3_drv
.p3_drv_c:
        ld      a,#'C'
        jr      .p3_drv
.p3_drv_d:
        ld      a,#'D'
.p3_drv:
        ld      (de),a
        inc     de
        ld      a,#':'
        ld      (de),a
        inc     de
.p3_pcopy:
        ld      a,(hl)
        or      a
        jr      z,.p3_pterm
        ld      (de),a
        inc     hl
        inc     de
        jr      .p3_pcopy
.p3_pterm:
        ld      a,#0xff
        ld      (de),a
        pop     hl
        ret

        ; A = file number -> HL = table entry. Clobbers BC.
.p3_fent:
        ld      l,a
        ld      h,#0
        ld      c,l
        ld      b,h
        add     hl,hl
        add     hl,hl
        add     hl,hl
        add     hl,bc
        ld      bc,#__p3_ft
        add     hl,bc
        ret

        ; A = native handle. Checks it names an open file (1..14): HL =
        ; entry, A unchanged. Carry set and A = 13 (EBADF) if invalid.
.p3_valid:
        or      a
        jr      z,.p3_badh
        cp      #TEMP_FILE
        jr      nc,.p3_badh
        ld      d,a
        call    .p3_fent
        ld      a,(hl)
        or      a
        ld      a,d
        jr      z,.p3_badh
        or      a
        ret
.p3_badh:
        ld      a,#13
        scf
        ret

        ; HL = entry, BC = value: add BC to the 32-bit position at +1.
.p3_addpos:
        inc     hl
        ld      a,(hl)
        add     a,c
        ld      (hl),a
        inc     hl
        ld      a,(hl)
        adc     a,b
        ld      (hl),a
        inc     hl
        ld      a,(hl)
        adc     a,#0
        ld      (hl),a
        inc     hl
        ld      a,(hl)
        adc     a,#0
        ld      (hl),a
        ret

        ; HL = entry: if position > size then size = position.
.p3_growsize:
        push    hl
        inc     hl                      ; HL = pos
        ld      d,h
        ld      e,l
        ld      bc,#4
        add     hl,bc                   ; HL = size
        ex      de,hl                   ; DE = size, HL = pos
        ld      a,(de)                  ; pos - size
        ld      c,a
        ld      a,(hl)
        sub     c
        inc     hl
        inc     de
        ld      a,(de)
        ld      c,a
        ld      a,(hl)
        sbc     a,c
        inc     hl
        inc     de
        ld      a,(de)
        ld      c,a
        ld      a,(hl)
        sbc     a,c
        inc     hl
        inc     de
        ld      a,(de)
        ld      c,a
        ld      a,(hl)
        sbc     a,c
        pop     hl
        ret     c                       ; pos < size
        push    hl
        inc     hl
        ld      d,h
        ld      e,l
        ld      bc,#4
        add     hl,bc
        ex      de,hl                   ; DE = size, HL = pos
        ld      bc,#4
        ldir
        pop     hl
        ret

        ; A = file number: read its length into t0 (low) and t1 (high).
.p3_eof:
        ld      b,a
        call    .p3_clear
        ld      c,#0
        ld      (__p3_bc),bc
        ld      de,#DOS_GET_EOF
        call    .p3_call
        ret     c
        ld      hl,(__p3_ohl)
        ld      (__p3_t0),hl
        ld      hl,(__p3_ode)
        ld      (__p3_t1),hl
        or      a
        ret

        ; A = file number: DOS_CLOSE.
.p3_close_raw:
        ld      b,a
        call    .p3_clear
        ld      c,#0
        ld      (__p3_bc),bc
        ld      de,#DOS_CLOSE
        jp      .p3_call

        ; ---------------------------------------------------------------
        ; Files
        ; ---------------------------------------------------------------

        ; __next_fs_real_f_open
        ; inputs: A = drive, HL = path, B = native open mode.
        ; outputs: A = native handle on success.
        ; CY set and A = native error on failure.
        ; clobbers: af, bc, de, hl; preserves ix and iy.
__next_fs_real_f_open::
        call    _enter_critical_section
        call    .p3_open
        jp      _leave_critical_section
.p3_open:
        ld      c,b                     ; C = native mode
        ld      de,#__p3_pa
        call    .p3_pbuild              ; clobbers A only (BC kept)
        ; find a free file number 1..14
        ld      hl,#__p3_ft+FILE_SIZE
        ld      d,#1
.p3_open_find:
        ld      a,(hl)
        or      a
        jr      z,.p3_open_free
        ld      a,l
        add     a,#FILE_SIZE
        ld      l,a
        jr      nc,.p3_open_nc
        inc     h
.p3_open_nc:
        inc     d
        ld      a,d
        cp      #TEMP_FILE
        jr      c,.p3_open_find
        ld      a,#12                   ; EMFILE
        scf
        ret
.p3_open_free:
        ld      a,d
        ld      (__p3_n),a
        ; access: 1 read -> shared read (5), 2 write, 3 read/write
        ld      a,c
        and     #3
        ld      b,a
        cp      #1
        jr      nz,.p3_open_acc
        ld      b,#5
.p3_open_acc:
        ; actions: D = create action, E = open action
        ld      a,c
        and     #0x0c
        ld      de,#0x0002              ; existing file only
        jr      z,.p3_open_set
        cp      #0x04
        ld      de,#0x0200              ; create new, fail if present
        jr      z,.p3_open_set
        cp      #0x08
        ld      de,#0x0202              ; open or create
        jr      z,.p3_open_set
        ld      de,#0x0204              ; create, truncating an existing file
.p3_open_set:
        ld      c,b                     ; C = access
        push    de
        call    .p3_clear
        pop     de
        ld      (__p3_de),de
        ld      a,(__p3_n)
        ld      b,a                     ; B = file number
        ld      (__p3_bc),bc
        ld      hl,#__p3_pa
        ld      (__p3_hl),hl
        ld      de,#DOS_OPEN
        call    .p3_call
        ret     c
        ld      a,(__p3_n)
        call    .p3_eof
        jr      c,.p3_open_fail
        ld      a,(__p3_n)
        call    .p3_fent
        ld      a,(__p3_pa)             ; C/D drive retained for DOS_FLUSH
        ld      (hl),a                  ; nonzero = used
        inc     hl
        xor     a
        ld      (hl),a                  ; pos = 0
        inc     hl
        ld      (hl),a
        inc     hl
        ld      (hl),a
        inc     hl
        ld      (hl),a
        inc     hl
        ld      de,(__p3_t0)
        ld      (hl),e                  ; size
        inc     hl
        ld      (hl),d
        inc     hl
        ld      de,(__p3_t1)
        ld      (hl),e
        inc     hl
        ld      (hl),d
        ld      a,(__p3_n)
        or      a
        ret
.p3_open_fail:
        push    af
        ld      a,(__p3_n)
        call    .p3_close_raw
        pop     af
        scf
        ret

        ; __next_fs_real_f_close
        ; inputs: A = native file or directory handle.
__next_fs_real_f_close::
        call    _enter_critical_section
        call    .p3_close
        jp      _leave_critical_section
.p3_close:
        cp      #DIR_BASE
        jr      c,.p3_close_file
        push    af                      ; handle survives pointer lookup
        call    .p3_dslot
        pop     bc
        ret     c
        push    hl                      ; owned catalog allocation
        ld      a,b
        sub     #DIR_BASE
        ld      l,a
        ld      h,#0
        add     hl,hl
        ld      de,#__p3_dirs
        add     hl,de
        xor     a
        ld      (hl),a                  ; release the handle before its storage
        inc     hl
        ld      (hl),a
        pop     hl
        call    __os_free
        or      a
        ret
.p3_close_file:
        call    .p3_valid
        ret     c
        ld      (hl),#0                 ; free the entry first
        jp      .p3_close_raw

        ; __next_fs_real_f_sync
__next_fs_real_f_sync::
        call    _enter_critical_section
        call    .p3_valid
        jr      c,.p3_sync_done
        ld      a,(hl)                  ; drive of this open file
        push    af
        call    .p3_clear
        pop     af
        ld      (__p3_af+1),a
        ld      de,#DOS_FLUSH
        call    .p3_call
.p3_sync_done:
        jp      _leave_critical_section

        ; __next_fs_real_f_read
        ; inputs: A = handle, HL = buffer, BC = byte count.
        ; outputs: BC = actual bytes read on success.
__next_fs_real_f_read::
        call    _enter_critical_section
        call    .p3_read
        jp      _leave_critical_section
.p3_read:
        ld      (__p3_dst),hl
        ld      (__p3_t0),bc
        call    .p3_valid
        ret     c
        ld      (__p3_n),a
        ; BC:DE = size - pos
        inc     hl                      ; pos
        ld      e,(hl)
        inc     hl
        ld      d,(hl)
        inc     hl
        ld      c,(hl)
        inc     hl
        ld      b,(hl)
        inc     hl                      ; HL = size, BC:DE = pos
        ld      a,(hl)
        sub     e
        ld      e,a
        inc     hl
        ld      a,(hl)
        sbc     a,d
        ld      d,a
        inc     hl
        ld      a,(hl)
        sbc     a,c
        ld      c,a
        inc     hl
        ld      a,(hl)
        sbc     a,b
        ld      b,a
        ld      hl,(__p3_t0)            ; requested
        ld      a,b
        or      c
        jr      nz,.p3_read_req         ; 64K or more is left
        or      a
        sbc     hl,de                   ; requested - left
        add     hl,de
        jr      c,.p3_read_req          ; requested < left
        ex      de,hl                   ; read only what is left
.p3_read_req:
        ld      (__p3_rem),hl
        ld      hl,#0
        ld      (__p3_done),hl
.p3_read_loop:
        ld      hl,(__p3_rem)
        ld      a,h
        or      l
        jr      z,.p3_read_end
        ld      a,h
        cp      #STAGE_HI
        jr      c,.p3_read_chunk
        ld      hl,#STAGE_SIZE
.p3_read_chunk:
        ld      (__p3_chunk),hl
        call    .p3_clear
        ld      a,(__p3_n)
        ld      b,a
        ld      c,#7
        ld      (__p3_bc),bc
        ld      hl,#__p3_stage
        ld      (__p3_hl),hl
        ld      hl,(__p3_chunk)
        ld      (__p3_de),hl
        ld      de,#DOS_READ
        call    .p3_call
        ret     c
        ld      bc,(__p3_chunk)
        ld      hl,#__p3_stage
        ld      de,(__p3_dst)
        ldir
        ld      (__p3_dst),de
        ld      a,(__p3_n)
        call    .p3_fent
        ld      bc,(__p3_chunk)
        call    .p3_addpos
        ld      hl,(__p3_rem)
        ld      bc,(__p3_chunk)
        or      a
        sbc     hl,bc
        ld      (__p3_rem),hl
        ld      hl,(__p3_done)
        add     hl,bc
        ld      (__p3_done),hl
        jr      .p3_read_loop
.p3_read_end:
        ld      bc,(__p3_done)
        or      a
        ret

        ; __next_fs_real_f_write
        ; inputs: A = handle, HL = buffer, BC = byte count.
        ; outputs: BC = actual bytes written on success.
__next_fs_real_f_write::
        call    _enter_critical_section
        call    .p3_write
        jp      _leave_critical_section
.p3_write:
        ld      (__p3_dst),hl
        ld      (__p3_rem),bc
        call    .p3_valid
        ret     c
        ld      (__p3_n),a
        ld      hl,#0
        ld      (__p3_done),hl
.p3_write_loop:
        ld      hl,(__p3_rem)
        ld      a,h
        or      l
        jr      z,.p3_write_end
        ld      a,h
        cp      #STAGE_HI
        jr      c,.p3_write_chunk
        ld      hl,#STAGE_SIZE
.p3_write_chunk:
        ld      (__p3_chunk),hl
        ld      b,h
        ld      c,l
        ld      hl,(__p3_dst)
        ld      de,#__p3_stage
        ldir
        ld      (__p3_dst),hl
        call    .p3_clear
        ld      a,(__p3_n)
        ld      b,a
        ld      c,#7
        ld      (__p3_bc),bc
        ld      hl,#__p3_stage
        ld      (__p3_hl),hl
        ld      hl,(__p3_chunk)
        ld      (__p3_de),hl
        ld      de,#DOS_WRITE
        call    .p3_call
        ret     c
        ld      a,(__p3_n)
        call    .p3_fent
        ld      bc,(__p3_chunk)
        push    hl
        call    .p3_addpos
        pop     hl
        call    .p3_growsize
        ld      hl,(__p3_rem)
        ld      bc,(__p3_chunk)
        or      a
        sbc     hl,bc
        ld      (__p3_rem),hl
        ld      hl,(__p3_done)
        add     hl,bc
        ld      (__p3_done),hl
        jr      .p3_write_loop
.p3_write_end:
        ld      bc,(__p3_done)
        or      a
        ret

        ; __next_fs_real_f_seek
        ; inputs: A = handle, BC:DE = offset, L = native seek mode
        ; (0 absolute, 1 forward from the current position). The target is
        ; clamped to the end of the file.
__next_fs_real_f_seek::
        call    _enter_critical_section
        call    .p3_seek
        jp      _leave_critical_section
.p3_seek:
        ld      (__p3_t0),de
        ld      (__p3_t1),bc
        push    af                      ; handle
        ld      a,l
        ld      (__p3_n2),a             ; mode
        pop     af
        call    .p3_valid
        ret     c
        ld      (__p3_n),a
        ld      (__p3_dst),hl           ; entry
        ld      a,(__p3_n2)
        or      a
        jr      z,.p3_seek_clamp
        cp      #1
        jr      nz,.p3_seek_bad
        inc     hl                      ; relative: t = pos + offset
        ld      a,(__p3_t0)
        add     a,(hl)
        ld      (__p3_t0),a
        inc     hl
        ld      a,(__p3_t0+1)
        adc     a,(hl)
        ld      (__p3_t0+1),a
        inc     hl
        ld      a,(__p3_t1)
        adc     a,(hl)
        ld      (__p3_t1),a
        inc     hl
        ld      a,(__p3_t1+1)
        adc     a,(hl)
        ld      (__p3_t1+1),a
.p3_seek_clamp:
        ld      hl,(__p3_dst)
        ld      bc,#5
        add     hl,bc                   ; HL = size
        ld      a,(hl)                  ; size - target
        ld      bc,(__p3_t0)
        sub     c
        inc     hl
        ld      a,(hl)
        sbc     a,b
        inc     hl
        ld      bc,(__p3_t1)
        ld      a,(hl)
        sbc     a,c
        inc     hl
        ld      a,(hl)
        sbc     a,b
        jr      nc,.p3_seek_set         ; target <= size
        dec     hl
        dec     hl
        dec     hl                      ; target = size
        ld      de,#__p3_t0
        ld      bc,#4
        ldir
.p3_seek_set:
        call    .p3_clear
        ld      a,(__p3_n)
        ld      b,a
        ld      c,#0
        ld      (__p3_bc),bc
        ld      hl,(__p3_t1)
        ld      (__p3_de),hl
        ld      hl,(__p3_t0)
        ld      (__p3_hl),hl
        ld      de,#DOS_SET_POSITION
        call    .p3_call
        ret     c
        ld      hl,(__p3_dst)
        inc     hl
        ex      de,hl
        ld      hl,#__p3_t0
        ld      bc,#4
        ldir
        or      a
        ret
.p3_seek_bad:
        ld      a,#2
        scf
        ret

        ; __next_fs_real_f_fgetpos
        ; inputs: A = native file handle.
        ; outputs: BC:DE = current position on success.
__next_fs_real_f_fgetpos::
        call    _enter_critical_section
        call    .p3_valid
        jr      c,.p3_pos_done
        inc     hl
        ld      e,(hl)
        inc     hl
        ld      d,(hl)
        inc     hl
        ld      c,(hl)
        inc     hl
        ld      b,(hl)
        or      a
.p3_pos_done:
        jp      _leave_critical_section

        ; __next_fs_real_f_fstat
        ; inputs: A = handle, HL = 11-byte native status buffer.
__next_fs_real_f_fstat::
        call    _enter_critical_section
        call    .p3_fstat
        jp      _leave_critical_section
.p3_fstat:
        ld      (__p3_dst),hl
        call    .p3_valid
        ret     c
        ld      de,#5
        add     hl,de                   ; entry size
        push    hl
        ld      hl,(__p3_dst)
        ld      b,#11
        xor     a
.p3_fstat_clear:
        ld      (hl),a
        inc     hl
        djnz    .p3_fstat_clear
        ld      hl,(__p3_dst)
        ld      de,#7
        add     hl,de
        ex      de,hl
        pop     hl
        ld      bc,#4
        ldir
        or      a
        ret

        ; ---------------------------------------------------------------
        ; Names
        ; ---------------------------------------------------------------

        ; __next_fs_real_f_getcwd
        ; inputs: A = drive, HL = 256-byte output buffer.
        ; outputs: NUL-terminated absolute path.
__next_fs_real_f_getcwd::
        call    _enter_critical_section
        call    .p3_getcwd
        jp      _leave_critical_section
.p3_getcwd:
        ld      (__p3_dst),hl
        ld      b,a
        ld      a,(__p3_drive)
        ld      c,a
        ld      a,b
        cp      #0x40
        jr      nz,.p3_gc_d
        ld      c,#'C'
.p3_gc_d:
        cp      #0x48
        jr      nz,.p3_gc_spec
        ld      c,#'D'
.p3_gc_spec:
        ld      hl,#__p3_pa             ; location spec "X:" $FF
        ld      (hl),c
        inc     hl
        ld      (hl),#':'
        inc     hl
        ld      (hl),#0xff
        call    .p3_clear
        ld      a,#1                    ; rc_path_get
        ld      (__p3_af+1),a
        ld      hl,#__p3_pa
        ld      (__p3_hl),hl
        ld      de,#IDE_PATH
        call    .p3_call
        ret     c
        ld      hl,#__p3_pa             ; skip a "X:" prefix
        ld      a,(hl)
        cp      #0xff
        jr      z,.p3_cwd_root
        inc     hl
        ld      a,(hl)
        dec     hl
        cp      #':'
        jr      nz,.p3_cwd_chk
        inc     hl
        inc     hl
.p3_cwd_chk:
        ld      de,(__p3_dst)
        ld      a,(hl)
        cp      #0xff
        jr      z,.p3_cwd_root
        cp      #'/'
        jr      z,.p3_cwd_copy
        ld      a,#'/'
        ld      (de),a
        inc     de
.p3_cwd_copy:
        ld      a,(hl)
        cp      #0xff
        jr      z,.p3_cwd_end
        ld      (de),a
        inc     hl
        inc     de
        jr      .p3_cwd_copy
.p3_cwd_end:
        xor     a
        ld      (de),a
        ret
.p3_cwd_root:
        ld      de,(__p3_dst)
        ld      a,#'/'
        ld      (de),a
        inc     de
        xor     a
        ld      (de),a
        ret

        ; __next_fs_real_f_chdir / mkdir / rmdir: IDE_PATH reasons 0 / 2 / 3.
        ; inputs: A = drive, HL = path.
__next_fs_real_f_chdir::
        ld      b,#0
        jr      .p3_pathop
__next_fs_real_f_mkdir::
        ld      b,#2
        jr      .p3_pathop
__next_fs_real_f_rmdir::
        ld      b,#3
.p3_pathop:
        call    _enter_critical_section
        ld      c,b
        ld      de,#__p3_pa
        call    .p3_pbuild              ; keeps BC
        ld      a,c
        ld      (__p3_n2),a
        call    .p3_clear
        ld      a,(__p3_n2)
        ld      (__p3_af+1),a
        ld      hl,#__p3_pa
        ld      (__p3_hl),hl
        ld      de,#IDE_PATH
        call    .p3_call
        push    af
        jr      c,.p3_path_return
        ld      a,(__p3_n2)
        or      a                       ; only successful chdir selects drive
        jr      nz,.p3_path_return
        ld      a,(__p3_pa)
        ld      (__p3_drive),a
.p3_path_return:
        pop     af
        jp      _leave_critical_section

        ; __next_fs_real_f_unlink
        ; inputs: A = drive, HL = path.
__next_fs_real_f_unlink::
        call    _enter_critical_section
        ld      de,#__p3_pa
        call    .p3_pbuild
        call    .p3_clear
        ld      hl,#__p3_pa
        ld      (__p3_hl),hl
        ld      de,#DOS_DELETE
        call    .p3_call
        jp      _leave_critical_section

        ; __next_fs_real_f_rename
        ; inputs: A = drive, HL = old path, DE = new path.
__next_fs_real_f_rename::
        call    _enter_critical_section
        push    de
        push    af
        ld      de,#__p3_pa
        call    .p3_pbuild
        pop     af
        pop     hl
        ld      de,#__p3_pb
        call    .p3_pbuild
        call    .p3_clear
        ld      hl,#__p3_pa
        ld      (__p3_hl),hl
        ld      hl,#__p3_pb
        ld      (__p3_de),hl
        ld      de,#DOS_RENAME
        call    .p3_call
        jp      _leave_critical_section

        ; __next_fs_real_f_stat
        ; inputs: A = drive, HL = path, DE = 11-byte native status buffer.
        ; Files are opened shared-read to learn their length; a directory
        ; is found through DOS_CATALOG.
__next_fs_real_f_stat::
        call    _enter_critical_section
        call    .p3_stat
        jp      _leave_critical_section
.p3_stat:
        ld      (__p3_dst),de           ; status buffer
        ld      de,#__p3_pa
        call    .p3_pbuild
        call    .p3_clear
        ld      b,#TEMP_FILE
        ld      c,#5
        ld      (__p3_bc),bc
        ld      hl,#0x0002              ; create 0, open 2 (ignore header)
        ld      (__p3_de),hl
        ld      hl,#__p3_pa
        ld      (__p3_hl),hl
        ld      de,#DOS_OPEN
        call    .p3_call
        jr      c,.p3_stat_dir
        ld      a,#TEMP_FILE
        call    .p3_eof
        push    af
        ld      a,#TEMP_FILE
        call    .p3_close_raw
        pop     af
        ret     c
        call    .p3_stat_blank          ; HL = buffer
        inc     hl
        inc     hl
        inc     hl
        inc     hl
        inc     hl
        inc     hl
        inc     hl                      ; buffer + 7
        ld      de,(__p3_t0)
        ld      (hl),e
        inc     hl
        ld      (hl),d
        inc     hl
        ld      de,(__p3_t1)
        ld      (hl),e
        inc     hl
        ld      (hl),d
        or      a
        ret
.p3_stat_dir:
        ld      hl,#__p3_cat
        ld      de,#__p3_cat+1
        ld      bc,#13*7-1
        ld      (hl),#0
        ldir
        call    .p3_clear
        ld      bc,#0x0227              ; B = 2 entries; unsorted directory lookup
        ld      (__p3_bc),bc
        ld      hl,#__p3_cat
        ld      (__p3_de),hl
        ld      hl,#__p3_pa
        ld      (__p3_hl),hl
        ld      de,#DOS_CATALOG
        call    .p3_call
        ret     c
        ld      a,(__p3_obc+1)          ; completed entries
        or      a
        jr      z,.p3_stat_nofile
        ld      a,(__p3_cat+13+7)
        bit     7,a                     ; directory flag
        jr      z,.p3_stat_nofile
        call    .p3_stat_blank
        inc     hl
        inc     hl
        ld      (hl),#0x10              ; attribute: directory
        ld      de,#5
        add     hl,de
        ld      a,#0xff
        ld      (hl),a                  ; size sentinel 0xFFFFFFFF
        inc     hl
        ld      (hl),a
        inc     hl
        ld      (hl),a
        inc     hl
        ld      (hl),a
        or      a
        ret
.p3_stat_nofile:
        ld      a,#5                    ; ENOENT
        scf
        ret
        ; Zero the status buffer, set the drive byte. Returns HL = buffer.
.p3_stat_blank:
        ld      hl,(__p3_dst)
        push    hl
        ld      b,#11
        xor     a
.p3_stat_clr:
        ld      (hl),a
        inc     hl
        djnz    .p3_stat_clr
        pop     hl
        ld      (hl),#0x40
        ret

        ; ---------------------------------------------------------------
        ; Directory enumeration: DOS_CATALOG batches + IDE_GET_LFN details
        ; ---------------------------------------------------------------

        ; A = native directory handle -> HL = catalog state; carry set and
        ; A = 13 if it is not open. Clobbers AF, BC, DE, HL.
.p3_dslot:
        sub     #DIR_BASE
        cp      #DIR_COUNT
        jr      nc,.p3_dbad
        ld      l,a
        ld      h,#0
        add     hl,hl
        ld      de,#__p3_dirs
        add     hl,de
        ld      c,(hl)
        inc     hl
        ld      h,(hl)
        ld      l,c
        ld      a,h
        or      l
        jr      z,.p3_dbad
        or      a
        ret
.p3_dbad:
        ld      a,#13
        scf
        ret

        ; __next_fs_real_f_opendir
        ; inputs: A = drive, HL = path, B = native directory mode.
        ; outputs: A = native directory handle on success.
__next_fs_real_f_opendir::
        call    _enter_critical_section
        push    ix
        call    .p3_opendir
        pop     ix
        jp      _leave_critical_section
.p3_opendir:
        ld      de,#__p3_pa
        call    .p3_pbuild              ; DE = terminator
        push    de
        ; "dots": the directory has a parent unless the path is empty or "/"
        ld      hl,#__p3_pa
        ld      a,(hl)
        cp      #0xff
        jr      z,.p3_od_nodots
        inc     hl
        ld      a,(hl)
        dec     hl
        cp      #':'
        jr      nz,.p3_od_nodrive
        inc     hl
        inc     hl
.p3_od_nodrive:
        ld      a,(hl)
        cp      #0xff
        jr      z,.p3_od_nodots
        cp      #'/'
        jr      nz,.p3_od_dots
        inc     hl
        ld      a,(hl)
        cp      #0xff
        jr      z,.p3_od_nodots
.p3_od_dots:
        ld      a,#1
        jr      .p3_od_dotsset
.p3_od_nodots:
        xor     a
.p3_od_dotsset:
        ld      (__p3_n2),a
        pop     de
        ; append "/*.*" (or "*.*" after a drive colon or a slash)
        dec     de
        ld      a,(de)
        inc     de
        cp      #'/'
        jr      z,.p3_od_star
        cp      #':'
        jr      z,.p3_od_star
        ld      hl,#__p3_pa
        or      a
        sbc     hl,de                   ; empty path (HL == DE)?
        add     hl,de
        jr      z,.p3_od_star
        ld      a,#'/'
        ld      (de),a
        inc     de
.p3_od_star:
        ld      a,#'*'
        ld      (de),a
        inc     de
        ld      a,#'.'
        ld      (de),a
        inc     de
        ld      a,#'*'
        ld      (de),a
        inc     de
        ld      a,#0xff
        ld      (de),a
        ; spec length check (stored at most 130 bytes incl. terminator)
        ld      hl,#__p3_pa
        ex      de,hl
        or      a
        sbc     hl,de
        ld      a,l
        cp      #129
        jr      c,.p3_od_len_ok
        ld      a,#2                    ; EINVAL: path too long
        scf
        ret
.p3_od_len_ok:
        ; Find a free handle. Catalogs live in the common heap so idle
        ; browsers do not permanently reserve large ROM-visible buffers.
        ld      hl,#__p3_dirs
        ld      b,#DIR_COUNT
.p3_od_find:
        ld      a,(hl)
        inc     hl
        or      (hl)
        dec     hl
        jr      z,.p3_od_slot
        inc     hl
        inc     hl
        djnz    .p3_od_find
.p3_od_full:
        ld      a,#12                   ; EMFILE
        scf
        ret
.p3_od_slot:
        ld      a,#DIR_COUNT
        sub     b
        ld      (__p3_n),a
        push    hl                      ; native handle table cell
        ; Driver-owned until closed, independent of the UI process which
        ; performs the open. _mem_allocate removes the packed owner word.
        ld      hl,#0
        push    hl
        ld      de,#DIR_SIZE
        ld      hl,#__sys_heap
        call    _mem_allocate
        pop     hl
        ld      a,d
        or      e
        jr      z,.p3_od_full
        ld      (hl),e
        inc     hl
        ld      (hl),d
        ex      de,hl
        push    hl
        ; clear the slot
        ld      d,h
        ld      e,l
        inc     de
        ld      bc,#DIR_SIZE-1
        ld      (hl),#0
        ldir
        pop     hl
        ld      (hl),#1                 ; used
        ld      de,#D_DOTS
        push    hl
        add     hl,de
        ld      a,(__p3_n2)
        ld      (hl),a
        inc     hl
        ld      (hl),a                  ; D_DOTS0
        pop     hl
        push    hl
        ld      de,#D_SPEC
        add     hl,de
        ex      de,hl
        ld      hl,#__p3_pa
        ld      bc,#130
        ldir
        pop     hl
        ; first batch: reports a missing directory now
        push    hl
        pop     ix
        call    .p3_dfill
        jr      nc,.p3_od_done
        push    af
        ld      a,(__p3_n)
        add     a,#DIR_BASE
        call    .p3_close                ; failed catalog owns no live handle
        pop     af
        scf
        ret
.p3_od_done:
        ld      a,(__p3_n)
        add     a,#DIR_BASE
        or      a
        ret

        ; IX = slot: fetch the next catalog batch into the queue. Preserves
        ; IX. Carry set and A = native error on failure.
.p3_dfill:
        ld      (ix+D_QCOUNT),#0
        ld      (ix+D_QINDEX),#0
        ; spec -> pa
        push    ix
        pop     hl
        ld      de,#D_SPEC
        add     hl,de
        ld      de,#__p3_pa
        ld      bc,#130
        ldir
        ; catalog buffer: zero, preload = last entry
        ld      hl,#__p3_cat
        ld      de,#__p3_cat+1
        ld      bc,#13*7-1
        ld      (hl),#0
        ldir
        push    ix
        pop     hl
        ld      de,#D_PRELOAD
        add     hl,de
        ld      de,#__p3_cat
        ld      bc,#13
        ldir
        call    .p3_clear
        ld      bc,#0x0727              ; B = 7 entries; unsorted, no IDE_BANK allocation
        ld      (__p3_bc),bc
        ld      hl,#__p3_cat
        ld      (__p3_de),hl
        ld      hl,#__p3_pa
        ld      (__p3_hl),hl
        ld      de,#DOS_CATALOG
        call    .p3_call
        ret     c
        ld      hl,(__p3_ohl)
        ld      (__p3_t0),hl            ; directory handle for IDE_GET_LFN
        ; Some firmware returns B including the preload/empty end record.
        ; Bound it to our six payload slots and trim zero entries before
        ; using the final entry as the next preload.
        ld      a,(__p3_obc+1)
        cp      #7
        jr      c,.p3_df_trim
        ld      a,#6
.p3_df_trim:
        or      a
        jr      z,.p3_df_count
        ld      l,a
        ld      h,#0
        ld      e,l
        ld      d,h
        add     hl,hl
        add     hl,hl
        add     hl,hl
        add     hl,de
        add     hl,de
        add     hl,de
        add     hl,de
        add     hl,de
        ld      de,#__p3_cat
        add     hl,de
        ld      b,a
        ld      a,(hl)
        or      a
        ld      a,b
        jr      nz,.p3_df_count
        dec     a
        jr      .p3_df_trim
.p3_df_count:
        ld      (__p3_rem),a
        cp      #6
        jr      nc,.p3_df_more
        ld      (ix+D_EOF),#1
.p3_df_more:
        or      a
        jr      nz,.p3_df_have
        ld      (ix+D_EOF),#1
        ret                             ; carry clear
.p3_df_have:
        ; remember the last raw entry as the next preload
        ld      a,(__p3_rem)
        ld      l,a
        ld      h,#0
        ld      e,l
        ld      d,h
        add     hl,hl                   ; 2n
        add     hl,hl                   ; 4n
        add     hl,hl                   ; 8n
        add     hl,de                   ; 9n
        add     hl,de
        add     hl,de
        add     hl,de
        add     hl,de                   ; 13n
        ld      de,#__p3_cat
        add     hl,de                   ; HL = last raw entry
        push    ix
        pop     de
        push    hl
        ld      hl,#D_PRELOAD
        add     hl,de
        ex      de,hl                   ; DE = slot + preload
        pop     hl
        ld      bc,#13
        ldir
        ; convert entries 1..n into queue records
        ld      a,#1
        ld      (__p3_n2),a
.p3_df_loop:
        ld      a,(__p3_n2)
        ld      b,a
        ld      a,(__p3_rem)
        cp      b
        jr      c,.p3_df_end
        call    .p3_dentry
        ret     c
        ld      hl,#__p3_n2
        inc     (hl)
        jr      .p3_df_loop
.p3_df_end:
        or      a
        ret

        ; IX = slot, A = record index -> HL = queue record address.
        ; Clobbers BC, DE.
.p3_rec_addr:
        ld      hl,#D_QUEUE
        or      a
        jr      z,.p3_rec_base
        ld      b,a
        ld      de,#REC_SIZE
.p3_rec_mul:
        add     hl,de
        djnz    .p3_rec_mul
.p3_rec_base:
        push    ix
        pop     de
        add     hl,de
        ret

        ; Convert catalog entry number (n2) and append it to the queue of
        ; IX; "." and ".." are skipped.
.p3_dentry:
        ld      a,(__p3_n2)
        ld      l,a
        ld      h,#0
        ld      e,l
        ld      d,h
        add     hl,hl
        add     hl,hl
        add     hl,hl                   ; 8n
        add     hl,de
        add     hl,de
        add     hl,de
        add     hl,de
        add     hl,de                   ; 13n
        ld      de,#__p3_cat
        add     hl,de
        ld      (__p3_t1),hl            ; raw entry
        ; skip "." and ".."
        ld      a,(hl)
        and     #0x7f
        cp      #'.'
        jr      nz,.p3_de_get
        inc     hl
        ld      a,(hl)
        and     #0x7f
        cp      #' '
        jp      z,.p3_de_skip
        cp      #'.'
        jr      nz,.p3_de_get
        inc     hl
        ld      a,(hl)
        and     #0x7f
        cp      #' '
        jp      z,.p3_de_skip
.p3_de_get:
        ld      a,(ix+D_QCOUNT)
        ld      l,a
        ld      h,#0
        ld      e,l
        ld      d,h
        add     hl,hl
        add     hl,hl
        add     hl,hl
        add     hl,de
        add     hl,de
        add     hl,de
        add     hl,de
        add     hl,de                   ; 13 * queue index
        ld      de,#D_RAW
        add     hl,de
        push    ix
        pop     de
        add     hl,de
        ex      de,hl
        ld      hl,(__p3_t1)
        ld      bc,#13
        ldir
        ld      hl,(__p3_t0)
        push    ix
        pop     de
        ex      de,hl
        ld      bc,#D_HANDLE
        add     hl,bc
        ld      (hl),e
        inc     hl
        ld      (hl),d
        ; IDE_GET_LFN: HL = spec, IX = directory handle, DE = entry,
        ; BC = long-name buffer. IX is the call's own parameter, so the
        ; slot pointer is kept on the stack.
        push    ix
        call    .p3_clear
        ld      hl,#__p3_pa
        ld      (__p3_hl),hl
        ld      hl,(__p3_t1)
        ld      (__p3_de),hl
        ld      hl,#__p3_lfn
        ld      (__p3_bc),hl
        ld      hl,(__p3_t0)
        ld      (__p3_ix),hl
        ld      de,#IDE_GET_LFN
        call    .p3_call
        pop     ix
        ret     c
        ld      a,(ix+D_QCOUNT)
        call    .p3_rec_addr
        ex      de,hl                   ; DE = record
        push    de
        inc     de                      ; the name follows the attribute
        ld      hl,(__p3_t1)
        ld      b,#8
.p3_de_nm:
        ld      a,(hl)
        and     #0x7f
        cp      #' '
        jr      z,.p3_de_nmend
        ld      (de),a
        inc     de
        inc     hl
        djnz    .p3_de_nm
.p3_de_nmend:
        ld      hl,(__p3_t1)
        ld      bc,#8
        add     hl,bc                   ; extension
        ld      a,(hl)
        and     #0x7f
        cp      #' '
        jr      z,.p3_de_noext
        ld      a,#'.'
        ld      (de),a
        inc     de
        ld      b,#3
.p3_de_ex:
        ld      a,(hl)
        and     #0x7f
        cp      #' '
        jr      z,.p3_de_noext
        ld      (de),a
        inc     de
        inc     hl
        djnz    .p3_de_ex
.p3_de_noext:
        xor     a
        ld      (de),a                  ; NUL
        inc     de
        ld      hl,(__p3_obc)           ; date
        ld      a,l
        ld      (de),a
        inc     de
        ld      a,h
        ld      (de),a
        inc     de
        ld      hl,(__p3_ode)           ; time
        ld      a,l
        ld      (de),a
        inc     de
        ld      a,h
        ld      (de),a
        inc     de
        ld      hl,(__p3_oix)           ; size low word (IX result)
        ld      a,l
        ld      (de),a
        inc     de
        ld      a,h
        ld      (de),a
        inc     de
        ld      hl,(__p3_ohl)           ; size high word
        ld      a,l
        ld      (de),a
        inc     de
        ld      a,h
        ld      (de),a
        pop     de                      ; record start
        ; attribute: a directory has bit 7 of the 8th name character set
        ld      hl,(__p3_t1)
        ld      bc,#7
        add     hl,bc
        xor     a
        bit     7,(hl)
        jr      z,.p3_de_attr
        ld      a,#0x10
.p3_de_attr:
        ld      (de),a
        inc     (ix+D_QCOUNT)
.p3_de_skip:
        or      a
        ret

        ; __next_fs_real_f_readdir
        ; inputs: A = directory handle, HL = native entry buffer.
        ; outputs: A = nonzero for an entry (record: attribute, name NUL,
        ; date, time, size), zero at the end.
__next_fs_real_f_readdir::
        call    _enter_critical_section
        call    .p3_readdir
        jp      _leave_critical_section
.p3_readdir:
        push    ix
        ld      (__p3_dst),hl
        call    .p3_dslot
        jr      c,.p3_rd_out
        push    hl
        pop     ix
        ; the parent entry comes first
        ld      a,(ix+D_DOTS)
        or      a
        jr      z,.p3_rd_queue
        ld      (ix+D_DOTS),#0
        ld      hl,(__p3_dst)
        ld      (hl),#0x10
        inc     hl
        ld      (hl),#'.'
        inc     hl
        ld      (hl),#'.'
        inc     hl
        ld      (hl),#0
        inc     hl
        ld      b,#8
.p3_rd_zero:
        ld      (hl),#0
        inc     hl
        djnz    .p3_rd_zero
        jr      .p3_rd_yes
.p3_rd_queue:
        ld      a,(ix+D_QINDEX)
        cp      (ix+D_QCOUNT)
        jr      c,.p3_rd_take
        ld      a,(ix+D_EOF)
        or      a
        jr      nz,.p3_rd_end
        call    .p3_dfill
        jr      c,.p3_rd_out
        jr      .p3_rd_queue
.p3_rd_take:
        call    .p3_rec_addr
        ld      de,(__p3_dst)
        ld      bc,#REC_SIZE
        ldir
        inc     (ix+D_QINDEX)
.p3_rd_yes:
        ld      a,#1
        or      a
        jr      .p3_rd_out
.p3_rd_end:
        xor     a
.p3_rd_out:
        pop     ix
        ret

        ; __next_fs_real_f_rewinddir
        ; inputs: A = directory handle.
__next_fs_real_f_rewinddir::
        call    _enter_critical_section
        push    ix
        call    .p3_dslot
        jr      c,.p3_rw_out
        push    hl
        pop     ix
        xor     a
        ld      (ix+D_EOF),a
        ld      (ix+D_QCOUNT),a
        ld      (ix+D_QINDEX),a
        ld      a,(ix+D_DOTS0)
        ld      (ix+D_DOTS),a
        push    ix
        pop     hl
        ld      de,#D_PRELOAD
        add     hl,de
        ld      d,h
        ld      e,l
        inc     de
        ld      bc,#12
        ld      (hl),#0
        ldir
        or      a
.p3_rw_out:
        pop     ix
        jp      _leave_critical_section

        ; ---------------------------------------------------------------
        ; Not backed by NextZXOS' +3DOS API
        ; ---------------------------------------------------------------

        ; __next_fs_real_disk_info: unused; next_calls.s reports the single
        ; whole-disk record itself.
__next_fs_real_disk_info::
        ld      a,#14                   ; no such device
        scf
        ret

        ; __next_fs_real_m_execcmd: dot commands are an native filesystem feature; the
        ; +3DOS API has no equivalent.
__next_fs_real_m_execcmd::
        ld      a,#14
        scf
        ret

        ; +3DOS result code -> native filesystem native error number.
.p3_errors:
        .db     1,8,1,1,1,1,1,1,8,1             ; 0..9    disk errors
        .db     1,1,1,1,1,1,1,1,1,1             ; 10..19
        .db     2,2,14,5,18,1,9,9,8,13          ; 20..29  badname..number
        .db     8,2,1,1,15,1,8,1,1,1            ; 30..39  denied..
        .db     1,1,1,1,1,1,1,1,1,1             ; 40..49
        .db     1,1,1,1,1,1,1,1,1,1             ; 50..59
        .db     12,1,1,1,1,14,1,1,1,17          ; 60..69  nohandle..notdir
        .db     1,1,1,1,1                       ; 70..74

        ; Supplement the public short-name entry with its full display name.
        ; A = native directory handle, HL = 256-byte destination.
        .globl __p3_directory_name
__p3_directory_name::
        push    ix
        push    hl
        call    .p3_dslot
        jr      c,.lfn_done
        push    hl
        pop     ix
        ld      a,(ix+D_QINDEX)
        or      a                       ; synthetic parent entry
        jr      z,.lfn_done
        dec     a
        ld      l,a
        ld      h,#0
        ld      e,l
        ld      d,h
        add     hl,hl
        add     hl,hl
        add     hl,hl
        add     hl,de
        add     hl,de
        add     hl,de
        add     hl,de
        add     hl,de
        ld      de,#D_RAW
        add     hl,de
        push    ix
        pop     de
        add     hl,de
        ; Heap storage can lie below $8000, where the ROM call temporarily
        ; replaces YOS RAM. Stage the raw entry in the ROM-visible area.
        ld      de,#__p3_cat
        ld      bc,#13
        ldir
        ld      hl,#__p3_cat
        ld      (__p3_t1),hl
        push    ix
        pop     hl
        ld      de,#D_SPEC
        add     hl,de
        ld      de,#__p3_pa
        ld      bc,#130
        ldir
        call    .p3_clear
        ld      hl,#__p3_pa
        ld      (__p3_hl),hl
        ld      hl,(__p3_t1)
        ld      (__p3_de),hl
        ld      hl,#__p3_lfn
        ld      (__p3_bc),hl
        push    ix
        pop     hl
        ld      de,#D_HANDLE
        add     hl,de
        ld      e,(hl)
        inc     hl
        ld      d,(hl)
        ld      (__p3_ix),de
        ld      de,#IDE_GET_LFN
        call    .p3_call
        jr      c,.lfn_done            ; retain the short alias on failure
        pop     de
        push    de
        ld      hl,#__p3_lfn
        ld      b,#255
.lfn_copy:
        ld      a,(hl)
        cp      #0xff
        jr      z,.lfn_end
        or      a
        jr      z,.lfn_end
        ld      (de),a
        inc     hl
        inc     de
        djnz    .lfn_copy
.lfn_end:
        xor     a
        ld      (de),a
.lfn_done:
        pop     hl
        pop     ix
        ret
