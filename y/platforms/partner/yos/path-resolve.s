        ;; Resident CP/M path parser and cached folder aliases.
        ;; Common entry adapters live in path.s so their external references
        ;; receive kernel veneers even when invoked from an application bank.
        .module cpm_path_resolve
        .globl __cpm_path, __cpm_resolve, __cpm_alias_set, __cpm_directory_name
        .globl __cpm_directory_visible
        .area _CODE
__cpm_resolve::
        call .initialize
        ld de,#.fd
        ld a,(hl)
        call .lower
        ld c,a
        inc hl
        ld a,(hl)
        dec hl
        cp #58
        jr nz,.native
        ld a,c
        cp #97
        jr z,.letter
        cp #98
        jp nz,.bad
        ld de,#.hd
.letter:
        inc hl
        inc hl
        jr .device
.native:
        push hl
        ld b,#3
.native_length:
        ld a,(hl)
        or a
        jr z,.short_native
        inc hl
        djnz .native_length
        ld a,(hl)
        pop hl
        cp #58
        jr nz,.device
        ld a,c
        cp #102
        jr z,.native_prefix
        cp #104
        jr nz,.device
        ld de,#.hd
.native_prefix:
        inc hl
        ld a,(hl)
        call .lower
        cp #100
        jp nz,.bad
        inc hl
        ld a,(hl)
        cp #48
        jp nz,.bad
        inc hl
        ld a,(hl)
        cp #58
        jp nz,.bad
        inc hl
        jr .device
.short_native:
        pop hl
.device:
        push hl
        ex de,hl
        ld de,#__cpm_path
        ld bc,#5
        ldir
        pop hl
        ld a,(hl)
        cp #47
        jr nz,.component
        inc hl
.component:
        ld a,(hl)
        or a
        jp z,.root
        push de
        call .split
        push hl
        ld de,#.alto
        call .equal
        pop hl
        jr z,.installed
        call .area_number
        jp nc,.selected
        ld a,(hl)
        cp #48
        jr c,.find_alias
        cp #58
        jp c,.invalid_pop              ; invalid numeric area never a filename
.find_alias:
        ld c,#0
.alias_loop:
        push hl
        ld a,c
        call __cpm_directory_name
        ex de,hl
        pop hl
        push hl
        call .equal
        pop hl
        jr z,.alias_found
        inc c
        ld a,c
        cp #16
        jr c,.alias_loop
        ;; A bare filename remains a compatibility alias for area zero.
        ld de,(.tail)
        ld a,d
        or e
        jp nz,.invalid_pop
        ld (.tail),hl
        xor a
        jp .selected
.alias_found:
        ld a,c
        jp .selected
.installed:
        ld hl,(.tail)
        ld a,h
        or l
        jp z,.installed_root
        call .split
        ld de,#.roles
        ld c,#0
.role_loop:
        push hl
        push de
        call .equal
        pop de
        pop hl
        jr z,.role_found
.skip_role:
        ld a,(de)
        inc de
        or a
        jr nz,.skip_role
        inc c
        ld a,c
        cp #6
        jr c,.role_loop
        jp .invalid_pop
.role_found:
        ld a,c
        cp #5                           ; usr/share/{fonts,bitmaps} -> sys
        jp nz,.selected
        ld hl,(.tail)
        ld a,h
        or l
        jr z,.resource_end
        push hl
        pop de
.resource_scan:
        ld a,(hl)
        inc hl
        or a
        jr z,.resource_done
        cp #47
        jr nz,.resource_scan
        ld d,h
        ld e,l
        jr .resource_scan
.resource_done:
        ld (.tail),de
.resource_end:
        ld a,#4
.selected:
        pop de
        cp #10
        jr c,.digit
        ld c,a
        ld a,#49
        ld (de),a
        inc de
        ld a,c
        sub #10
.digit:
        add a,#48
        ld (de),a
        inc de
        ld a,#47
        ld (de),a
        inc de
        ld hl,(.tail)
        ld a,h
        or l
        jp z,.root
        ld b,#13
.filename:
        ld a,(hl)
        inc hl
        cp #47
        jp z,.bad
        ld (de),a
        inc de
        or a
        ret z
        djnz .filename
.bad:
        ld a,#255
        ret
.invalid_pop:
        pop de
        jp .bad
.installed_root:
        pop de
.root:
        xor a
        ld (de),a
        ret
        ;; Split/lowercase one component in the private common snapshot.
.split:
        push hl
        ld de,#0
        ld (.tail),de
.split_next:
        ld a,(hl)
        or a
        jr z,.split_done
        cp #47
        jr z,.split_slash
        call .lower
        ld (hl),a
        inc hl
        jr .split_next
.split_slash:
        ld (hl),#0
        inc hl
        ld a,(hl)
        or a
        jr z,.split_done
        ld (.tail),hl
.split_done:
        pop hl
        ret
.lower:
        cp #65
        ret c
        cp #91
        ret nc
        or #32
        ret
.equal:
        ld a,(de)
        cp (hl)
        ret nz
        inc hl
        inc de
        or a
        jr nz,.equal
        ret
.area_number:
        push hl
        ld c,#0
.number_next:
        ld a,(hl)
        sub #48
        cp #10
        jr nc,.number_bad
        ld b,a
        ld a,c
        cp #2
        jr nc,.number_bad
        add a,a
        add a,a
        add a,c
        add a,a
        add a,b
        cp #16
        jr nc,.number_bad
        ld c,a
        inc hl
        ld a,(hl)
        or a
        jr nz,.number_next
        ld a,c
        pop hl
        or a
        ret
.number_bad:
        pop hl
        scf
        ret
        ;; A=area -> HL=kernel-owned alias; empty means display decimal area.
__cpm_directory_name::
        push bc
        ld l,a
        ld h,#0
        ld b,h
        ld c,l
        add hl,hl
        add hl,bc
        add hl,hl
        add hl,hl
        add hl,bc                       ; area * 13
        ld bc,#.aliases
        add hl,bc
        pop bc
        ret
        ;; A=area -> NZ if a root listing should expose it. Numeric paths
        ;; remain valid independently of this display policy.
__cpm_directory_visible::
        call __cpm_directory_name
        ld a,(hl)
        or a
        ret nz
        ld a,(.show_unmapped)
        or a
        ret
__cpm_alias_set::
        call .initialize
        ld a,e
        cp #255
        jr nz,.set_area
        ld a,(hl)
        sub #48
        cp #2
        jp nc,.bad
        inc hl
        ld e,a
        ld a,(hl)
        or a
        jp nz,.bad
        ld a,e
        ld (.show_unmapped),a
        xor a
        ret
.set_area:
        cp #16
        jp nc,.bad
        push de
        push hl
        ;; A folder is one component, never dot/dotdot or a numeric synonym.
        ld a,(hl)
        cp #46
        jr z,.alias_bad
        cp #48
        jr c,.validate_alias
        cp #58
        jr c,.alias_bad
.validate_alias:
        ld a,(hl)
        or a
        jr z,.alias_valid
        cp #33
        jr c,.alias_bad
        cp #47
        jr z,.alias_bad
        cp #58
        jr z,.alias_bad
        cp #92
        jr z,.alias_bad
        call .lower
        ld (hl),a
        inc hl
        jr .validate_alias
.alias_valid:
        pop hl
        pop de
        ld a,(hl)
        or a
        jr z,.store_alias               ; empty restores the numeric folder
        push de
        push hl
        ld de,#.alto
        call .equal
        pop hl
        pop de
        jp z,.bad                       ; ALTO is the fixed installation prefix
        ld b,e
        ld c,#0
        push de
.unique_alias:
        ld a,c
        cp b
        jr z,.unique_next
        push hl
        call __cpm_directory_name
        ex de,hl
        pop hl
        push hl
        call .equal
        pop hl
        jr z,.duplicate_alias
.unique_next:
        inc c
        ld a,c
        cp #16
        jr c,.unique_alias
        pop de
.store_alias:
        push hl
        ld a,e
        call __cpm_directory_name
        ex de,hl
        pop hl
.alias_copy:
        ld a,(hl)
        inc hl
        ld (de),a
        inc de
        or a
        jr nz,.alias_copy
        ret
.duplicate_alias:
        pop de
        jp .bad
.alias_bad:
        pop hl
        pop de
        jp .bad
.initialize:
        ld a,(.ready)
        or a
        ret nz
        inc a
        ld (.ready),a
        push hl
        push de
        push bc
        ld hl,#.roles
        ld de,#.aliases
        ld b,#5
.default_next:
        push bc
        push de
.default_copy:
        ld a,(hl)
        inc hl
        ld (de),a
        inc de
        or a
        jr nz,.default_copy
        pop de
        ex de,hl
        ld bc,#13
        add hl,bc
        ex de,hl
        pop bc
        djnz .default_next
        pop bc
        pop de
        pop hl
        ret
.fd: .ascii "fd0:/"
.hd: .ascii "hd0:/"
.alto: .asciz "alto"
.roles: .asciz "bin"
        .asciz "lib"
        .asciz "etc"
        .asciz "var"
        .asciz "sys"
        .asciz "usr"

        ;; Writable resident RAM image: keep aliases out of scarce common RAM
        ;; and the CPC low-bank BSS arena. Zero bytes compress in boot media.
.aliases: .ds 208
.ready: .ds 1
.show_unmapped: .db 0
.tail: .ds 2
