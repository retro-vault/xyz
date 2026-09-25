        ; YOS ABI 6 table offsets for assembly callers.
        ;
        ; Every entry is a 16-bit pointer. Load the YOS table address, add the
        ; required YOS_OFFSET_* byte offset, fetch the word, and call it.

        .equ    YOS_ABI_VERSION,              0x06

        ; ROM-model values returned by YOS_OFFSET_ROM_MODEL.
        .equ    YOS_ROM_MODEL_48K,             0
        .equ    YOS_ROM_MODEL_128K,            1
        .equ    YOS_ROM_MODEL_NEXT,            2

        ; Kernel identity and global firmware-print hook.
        .equ    YOS_OFFSET_VERSION,           0
        .equ    YOS_OFFSET_ROM_MODEL,         2
        .equ    YOS_OFFSET_SET_PRINT_HOOK,    4

        ; Banked user memory.
        .equ    YOS_OFFSET_ALLOCATE_MEMORY,   6
        .equ    YOS_OFFSET_FREE_MEMORY,       8
        .equ    YOS_OFFSET_SHRINK_MEMORY,    10

        ; Time and critical sections.
        .equ    YOS_OFFSET_CLOCK_TICKS,      12
        .equ    YOS_OFFSET_ENTER_CRITICAL_SECTION, 14
        .equ    YOS_OFFSET_LEAVE_CRITICAL_SECTION, 16

        ; Timers and synchronization events.
        .equ    YOS_OFFSET_CREATE_TIMER,     18
        .equ    YOS_OFFSET_DESTROY_TIMER,    20
        .equ    YOS_OFFSET_CREATE_EVENT,     22
        .equ    YOS_OFFSET_DESTROY_EVENT,    24
        .equ    YOS_OFFSET_SET_EVENT,        26
        .equ    YOS_OFFSET_WAIT_EVENT,       28

        ; Threads.
        .equ    YOS_OFFSET_CREATE_THREAD,    30
        .equ    YOS_OFFSET_EXIT_THREAD,      32
        .equ    YOS_OFFSET_SUSPEND_THREAD,   34
        .equ    YOS_OFFSET_RESUME_THREAD,    36

        ; Processes and loadable libraries.
        .equ    YOS_OFFSET_CREATE_PROCESS,   38
        .equ    YOS_OFFSET_LOAD_PROCESS,     40
        .equ    YOS_OFFSET_EXIT_PROCESS,     42
        .equ    YOS_OFFSET_LOAD_LIBRARY,     44
        .equ    YOS_OFFSET_PROCESS_LOAD_ERROR, 46

        ; Named services.
        .equ    YOS_OFFSET_QUERY_SERVICE,    48
        .equ    YOS_OFFSET_REGISTER_SERVICE, 50
        .equ    YOS_OFFSET_UNREGISTER_SERVICE, 52

        ; Interrupt vectors.
        .equ    YOS_OFFSET_GET_INTERRUPT_HANDLER, 54
        .equ    YOS_OFFSET_SET_INTERRUPT_HANDLER, 56

        ; Input devices.
        .equ    YOS_OFFSET_READ_KEY,         58
        .equ    YOS_OFFSET_CALIBRATE_MOUSE,  60
        .equ    YOS_OFFSET_READ_MOUSE,       62

        ; POSIX-style esxDOS filesystem.
        .equ    YOS_OFFSET_ERROR_NUMBER,     64
        .equ    YOS_OFFSET_OPEN,             66
        .equ    YOS_OFFSET_CLOSE,            68
        .equ    YOS_OFFSET_READ,             70
        .equ    YOS_OFFSET_WRITE,            72
        .equ    YOS_OFFSET_LSEEK,            74
        .equ    YOS_OFFSET_FSYNC,            76
        .equ    YOS_OFFSET_UNLINK,           78
        .equ    YOS_OFFSET_RENAME,           80
        .equ    YOS_OFFSET_CHDIR,            82
        .equ    YOS_OFFSET_GETCWD,           84
        .equ    YOS_OFFSET_MKDIR,            86
        .equ    YOS_OFFSET_RMDIR,            88
        .equ    YOS_OFFSET_STAT,             90
        .equ    YOS_OFFSET_FSTAT,            92
        .equ    YOS_OFFSET_OPENDIR,          94
        .equ    YOS_OFFSET_READDIR,          96
        .equ    YOS_OFFSET_REWINDDIR,        98
        .equ    YOS_OFFSET_CLOSEDIR,        100
        .equ    YOS_OFFSET_ENUMERATE_DISKS, 102

        ; Native esxDOS commands.
        .equ    YOS_OFFSET_EXEC_COMMAND,    104

        ; Graphics.
        .equ    YOS_OFFSET_GPX_CREATE, 106
        .equ    YOS_OFFSET_GPX_DESTROY, 108
        .equ    YOS_OFFSET_GPX_SET_PAGE, 110
        .equ    YOS_OFFSET_GPX_WIDTH, 112
        .equ    YOS_OFFSET_GPX_HEIGHT, 114
        .equ    YOS_OFFSET_GPX_CLEAR_SCREEN, 116
        .equ    YOS_OFFSET_GPX_SET_TEXT_BACKGROUND, 118
        .equ    YOS_OFFSET_GPX_DRAW_PIXEL, 120
        .equ    YOS_OFFSET_GPX_DRAW_LINE, 122
        .equ    YOS_OFFSET_GPX_DRAW_BITMAP, 124
        .equ    YOS_OFFSET_GPX_SHOW_SPRITE, 126
        .equ    YOS_OFFSET_GPX_HIDE_SPRITE, 128
        .equ    YOS_OFFSET_GPX_DRAW_RECTANGLE, 130
        .equ    YOS_OFFSET_GPX_FILL_RECTANGLE, 132
        .equ    YOS_OFFSET_GPX_MEASURE_TEXT, 134
        .equ    YOS_OFFSET_GPX_DRAW_TEXT, 136
        .equ    YOS_OFFSET_GPX_GET_SYSTEM_FONT, 138
        .equ    YOS_OFFSET_GPX_GET_TINY_FONT, 140
        .equ    YOS_OFFSET_GPX_GET_STOCK_BITMAP, 142
        .equ    YOS_OFFSET_GPX_DRAW_CIRCLE, 144
        .equ    YOS_OFFSET_GPX_FILL_CIRCLE, 146
        .equ    YOS_OFFSET_GPX_DRAW_POLYGON, 148
        .equ    YOS_OFFSET_GPX_FILL_POLYGON, 150
        .equ    YOS_OFFSET_GPX_DRAW_BOX, 152

        .equ    YOS_TABLE_SIZE,             154
