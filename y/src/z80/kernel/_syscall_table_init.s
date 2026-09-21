        ; Initialize the public YOS kernel and POSIX service table.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module _syscall_table_init
        .optsdcc -mz80 sdcccall(1)

        .globl  __yos
        .globl  _yos_version
        .globl  __yos_malloc
        .globl  __yos_free
        .globl  __clock
        .globl  _enter_critical_section
        .globl  _leave_critical_section
        .globl  __yos_install_timer
        .globl  _tmr_uninstall
        .globl  _evt_create
        .globl  _evt_destroy
        .globl  _evt_set
        .globl  _thread_create
        .globl  _thread_exit
        .globl  _thread_suspend
        .globl  _thread_resume
        .globl  _process_start
        .globl  _process_exit
        .globl  __svc_query
        .globl  _svc_register
        .globl  _svc_unregister
        .globl  _sys_vec_get
        .globl  _sys_vec_set
        .globl  _kbd_read
        .globl  _mouse_calibrate
        .globl  _mouse_read
        .globl  __errno_value
        .globl  _open
        .globl  _close
        .globl  _read
        .globl  _write
        .globl  _lseek
        .globl  _fsync
        .globl  _unlink
        .globl  _rename
        .globl  _chdir
        .globl  _getcwd
        .globl  _mkdir
        .globl  _rmdir
        .globl  _stat
        .globl  _fstat
        .globl  _opendir
        .globl  _readdir
        .globl  _rewinddir
        .globl  _closedir
        .globl  _enumerate_disks
        .globl  _process_load
        .globl  _process_last_error
        .globl  _library_load
        .globl  __yos_shrink
        .globl  _evt_wait
        .globl  _exec_command
        .globl  _set_print_hook

        .area   _HEADER_DATA
__yos::
        ; Kernel identity, memory, clock and critical sections.
        .dw     _yos_version
        .dw     __yos_malloc
        .dw     __yos_free
        .dw     __clock
        .dw     _enter_critical_section
        .dw     _leave_critical_section

        ; Timers and synchronization events.
        .dw     __yos_install_timer
        .dw     _tmr_uninstall
        .dw     _evt_create
        .dw     _evt_destroy
        .dw     _evt_set

        ; Threads and processes.
        .dw     _thread_create
        .dw     _thread_exit
        .dw     _thread_suspend
        .dw     _thread_resume
        .dw     _process_start
        .dw     _process_exit

        ; Named services and installable interrupt handlers.
        .dw     __svc_query
        .dw     _svc_register
        .dw     _svc_unregister
        .dw     _sys_vec_get
        .dw     _sys_vec_set

        ; Nonblocking keyboard events and Kempston mouse state.
        .dw     _kbd_read
        .dw     _mouse_calibrate
        .dw     _mouse_read

        ; POSIX-style esxDOS filesystem.
        .dw     __errno_value
        .dw     _open
        .dw     _close
        .dw     _read
        .dw     _write
        .dw     _lseek
        .dw     _fsync
        .dw     _unlink
        .dw     _rename
        .dw     _chdir
        .dw     _getcwd
        .dw     _mkdir
        .dw     _rmdir
        .dw     _stat
        .dw     _fstat
        .dw     _opendir
        .dw     _readdir
        .dw     _rewinddir
        .dw     _closedir
        .dw     _enumerate_disks
        .dw     _process_load
        .dw     _process_last_error
        .dw     _library_load

        ; Memory extension appended after the ABI 1 baseline.
        .dw     __yos_shrink
        ; ABI 2 appends event wait; all ABI 1 offsets remain stable.
        .dw     _evt_wait
        ; ABI 3 executes esxDOS dot commands with a temporary print sink.
        .dw     _exec_command
        .dw     _set_print_hook
