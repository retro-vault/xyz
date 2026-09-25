        ; Initialize the public YOS kernel and POSIX service table.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module _syscall_table_init
        .optsdcc -mz80 sdcccall(1)

        .globl  __yos
        .globl  _yos_version
        .globl  _yos_rom_model
        .globl  _yos_get_sys_info
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
        .globl  __so_reap                ; pack helper before 09F0h fixed slots
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
        .globl  _gpx_create
        .globl  _gpx_destroy
        .globl  _gpx_set_page
        .globl  _gpx_width
        .globl  _gpx_height
        .globl  _gpx_clrscr
        .globl  _gpx_set_text_background
        .globl  _gpx_draw_pixel
        .globl  _gpx_draw_line
        .globl  _gpx_draw_bmp
        .globl  _gpx_show_sprite
        .globl  _gpx_hide_sprite
        .globl  _gpx_draw_rectangle
        .globl  _gpx_fill_rectangle
        .globl  _gpx_measure_text
        .globl  _gpx_draw_text
        .globl  _gpx_get_system_font
        .globl  _gpx_get_tiny_font
        .globl  _gpx_get_stock_bmp
        .globl  _gpx_draw_circle
        .globl  _gpx_fill_circle
        .globl  _gpx_draw_box

        .area   _CONST
__yos::
        ; Kernel identity and the global firmware-print hook.
        .dw     _yos_version
        .dw     _yos_rom_model
        .dw     _yos_get_sys_info
        .dw     _set_print_hook

        ; Banked user memory.
        .dw     __yos_malloc
        .dw     __yos_free
        .dw     __yos_shrink

        ; Time and critical sections.
        .dw     __clock
        .dw     _enter_critical_section
        .dw     _leave_critical_section

        ; Timers and synchronization events.
        .dw     __yos_install_timer
        .dw     _tmr_uninstall
        .dw     _evt_create
        .dw     _evt_destroy
        .dw     _evt_set
        .dw     _evt_wait

        ; Threads.
        .dw     _thread_create
        .dw     _thread_exit
        .dw     _thread_suspend
        .dw     _thread_resume

        ; Processes and loadable libraries.
        .dw     _process_start
        .dw     _process_load
        .dw     _process_exit
        .dw     _library_load
        .dw     _process_last_error

        ; Named services.
        .dw     __svc_query
        .dw     _svc_register
        .dw     _svc_unregister

        ; Installable interrupt handlers.
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

        ; Native esxDOS commands.
        .dw     _exec_command

        ; Graphics.
        .dw     _gpx_create
        .dw     _gpx_destroy
        .dw     _gpx_set_page
        .dw     _gpx_width
        .dw     _gpx_height
        .dw     _gpx_clrscr
        .dw     _gpx_set_text_background
        .dw     _gpx_draw_pixel
        .dw     _gpx_draw_line
        .dw     _gpx_draw_bmp
        .dw     _gpx_show_sprite
        .dw     _gpx_hide_sprite
        .dw     _gpx_draw_rectangle
        .dw     _gpx_fill_rectangle
        .dw     _gpx_measure_text
        .dw     _gpx_draw_text
        .dw     _gpx_get_system_font
        .dw     _gpx_get_tiny_font
        .dw     _gpx_get_stock_bmp
        .dw     _gpx_draw_circle
        .dw     _gpx_fill_circle
        .dw     _gpx_draw_box
