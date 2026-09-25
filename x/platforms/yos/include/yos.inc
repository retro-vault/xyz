        ; YOS ABI 1 table offsets for assembly callers.
        ;
        ; Every entry is a 16-bit pointer. Load the YOS table address, add the
        ; required YOS_OFFSET_* byte offset, fetch the word, and call it.

        .equ    YOS_ABI_VERSION,              0x01

        ; ROM-model values returned by YOS_OFFSET_ROM_MODEL.
        .equ    YOS_ROM_MODEL_48K,             0
        .equ    YOS_ROM_MODEL_128K,            1
        .equ    YOS_ROM_MODEL_NEXT,            2

        ; Kernel identity and global firmware-print hook.
        .equ    YOS_OFFSET_VERSION,           0
        .equ    YOS_OFFSET_ROM_MODEL,         2
        .equ    YOS_OFFSET_GET_SYS_INFO,      4
        .equ    YOS_OFFSET_SET_PRINT_HOOK,    6

        ; Banked user memory.
        .equ    YOS_OFFSET_ALLOCATE_MEMORY,   8
        .equ    YOS_OFFSET_FREE_MEMORY,      10
        .equ    YOS_OFFSET_SHRINK_MEMORY,    12

        ; Time and critical sections.
        .equ    YOS_OFFSET_CLOCK_TICKS,      14
        .equ    YOS_OFFSET_ENTER_CRITICAL_SECTION, 16
        .equ    YOS_OFFSET_LEAVE_CRITICAL_SECTION, 18

        ; Timers and synchronization events.
        .equ    YOS_OFFSET_CREATE_TIMER,     20
        .equ    YOS_OFFSET_DESTROY_TIMER,    22
        .equ    YOS_OFFSET_CREATE_EVENT,     24
        .equ    YOS_OFFSET_DESTROY_EVENT,    26
        .equ    YOS_OFFSET_SET_EVENT,        28
        .equ    YOS_OFFSET_WAIT_EVENT,       30

        ; Threads.
        .equ    YOS_OFFSET_CREATE_THREAD,    32
        .equ    YOS_OFFSET_EXIT_THREAD,      34
        .equ    YOS_OFFSET_SUSPEND_THREAD,   36
        .equ    YOS_OFFSET_RESUME_THREAD,    38

        ; Processes and loadable libraries.
        .equ    YOS_OFFSET_CREATE_PROCESS,   40
        .equ    YOS_OFFSET_LOAD_PROCESS,     42
        .equ    YOS_OFFSET_EXIT_PROCESS,     44
        .equ    YOS_OFFSET_LOAD_LIBRARY,     46
        .equ    YOS_OFFSET_PROCESS_LOAD_ERROR, 48

        ; Named services.
        .equ    YOS_OFFSET_QUERY_SERVICE,    50
        .equ    YOS_OFFSET_REGISTER_SERVICE, 52
        .equ    YOS_OFFSET_UNREGISTER_SERVICE, 54

        ; Interrupt vectors.
        .equ    YOS_OFFSET_GET_INTERRUPT_HANDLER, 56
        .equ    YOS_OFFSET_SET_INTERRUPT_HANDLER, 58

        ; Input devices.
        .equ    YOS_OFFSET_READ_KEY,         60
        .equ    YOS_OFFSET_CALIBRATE_MOUSE,  62
        .equ    YOS_OFFSET_READ_MOUSE,       64

        ; POSIX-style esxDOS filesystem.
        .equ    YOS_OFFSET_ERROR_NUMBER,     66
        .equ    YOS_OFFSET_OPEN,             68
        .equ    YOS_OFFSET_CLOSE,            70
        .equ    YOS_OFFSET_READ,             72
        .equ    YOS_OFFSET_WRITE,            74
        .equ    YOS_OFFSET_LSEEK,            76
        .equ    YOS_OFFSET_FSYNC,            78
        .equ    YOS_OFFSET_UNLINK,           80
        .equ    YOS_OFFSET_RENAME,           82
        .equ    YOS_OFFSET_CHDIR,            84
        .equ    YOS_OFFSET_GETCWD,           86
        .equ    YOS_OFFSET_MKDIR,            88
        .equ    YOS_OFFSET_RMDIR,            90
        .equ    YOS_OFFSET_STAT,             92
        .equ    YOS_OFFSET_FSTAT,            94
        .equ    YOS_OFFSET_OPENDIR,          96
        .equ    YOS_OFFSET_READDIR,          98
        .equ    YOS_OFFSET_REWINDDIR,        100
        .equ    YOS_OFFSET_CLOSEDIR,         102
        .equ    YOS_OFFSET_ENUMERATE_DISKS, 104

        ; Native esxDOS commands.
        .equ    YOS_OFFSET_EXEC_COMMAND,    106

        ; Graphics.
        .equ    YOS_OFFSET_GPX_CREATE, 108
        .equ    YOS_OFFSET_GPX_DESTROY, 110
        .equ    YOS_OFFSET_GPX_SET_PAGE, 112
        .equ    YOS_OFFSET_GPX_WIDTH, 114
        .equ    YOS_OFFSET_GPX_HEIGHT, 116
        .equ    YOS_OFFSET_GPX_CLEAR_SCREEN, 118
        .equ    YOS_OFFSET_GPX_SET_TEXT_BACKGROUND, 120
        .equ    YOS_OFFSET_GPX_DRAW_PIXEL, 122
        .equ    YOS_OFFSET_GPX_DRAW_LINE, 124
        .equ    YOS_OFFSET_GPX_DRAW_BITMAP, 126
        .equ    YOS_OFFSET_GPX_SHOW_SPRITE, 128
        .equ    YOS_OFFSET_GPX_HIDE_SPRITE, 130
        .equ    YOS_OFFSET_GPX_DRAW_RECTANGLE, 132
        .equ    YOS_OFFSET_GPX_FILL_RECTANGLE, 134
        .equ    YOS_OFFSET_GPX_MEASURE_TEXT, 136
        .equ    YOS_OFFSET_GPX_DRAW_TEXT, 138
        .equ    YOS_OFFSET_GPX_GET_SYSTEM_FONT, 140
        .equ    YOS_OFFSET_GPX_GET_TINY_FONT, 142
        .equ    YOS_OFFSET_GPX_GET_STOCK_BITMAP, 144
        .equ    YOS_OFFSET_GPX_DRAW_CIRCLE, 146
        .equ    YOS_OFFSET_GPX_FILL_CIRCLE, 148
        .equ    YOS_OFFSET_GPX_DRAW_BOX, 150

        .equ    YOS_TABLE_SIZE,             152
