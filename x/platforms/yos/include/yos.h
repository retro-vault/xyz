/*
 * Public YOS kernel and filesystem interface.
 *
 * MIT License (see: LICENSE)
 * Copyright (C) 2026 tomaz stih
 */
#ifndef _YOS_H
#define _YOS_H

#include <stddef.h>
#include <stdint.h>
#include <sys/stat.h>
#include <sys/types.h>

/* ABI version returned by yos_s::version(). */
#define YOS_VERSION 0x01

enum yos_process_load_error {
    YOS_PROCESS_LOAD_OK = 0,
    YOS_PROCESS_LOAD_NOT_FOUND = 1,
    YOS_PROCESS_LOAD_NO_MEMORY = 2,
    YOS_PROCESS_LOAD_READ_ERROR = 3,
    YOS_PROCESS_LOAD_INVALID_IMAGE = 4,
    YOS_PROCESS_LOAD_START_ERROR = 5,
    YOS_PROCESS_LOAD_NOT_PROCESS = 6,
    YOS_PROCESS_LOAD_INCOMPATIBLE_OS = 7,
    YOS_PROCESS_LOAD_REQUIRES_NEWER_OS = 7,
    YOS_PROCESS_LOAD_BAD_CHECKSUM = 8,
    YOS_PROCESS_LOAD_BUSY = 9,
    YOS_PROCESS_LOAD_NO_PROCESS = 10,
    YOS_PROCESS_LOAD_INIT_ERROR = 11
};

/* Shared instances are keyed by full XPRG name and image ABI. */
#define YOS_LIBRARY_PRIVATE 0
#define YOS_LIBRARY_SHARED 1

/* Raw keyboard-event encoding returned by read_key(). */
#define YOS_KEY_DOWN 0x40
#define YOS_KEY_CODE 0x3f

/* Writable YOS vector-table indexes. */
enum yos_vector {
    YOS_VECTOR_RST18 = 2,
    YOS_VECTOR_RST20 = 3,
    YOS_VECTOR_RST28 = 4,
    YOS_VECTOR_RST30 = 5,
    YOS_VECTOR_RST38 = 6
};

/* Synchronization-event states. */
enum yos_event_state {
    YOS_EVENT_RESET = 0,
    YOS_EVENT_SET = 1
};

/* Spectrum hardware model detected while the ROM starts. */
enum yos_rom_model {
    YOS_ROM_MODEL_48K = 0,
    YOS_ROM_MODEL_128K = 1,
    YOS_ROM_MODEL_NEXT = 2
};

/* Kernel objects are opaque outside YOS. */
typedef struct yos_event yos_event_t;
typedef struct yos_directory yos_directory_t;
typedef struct yos_library_reference yos_library_reference_t;
typedef struct yos_memory_block yos_memory_block_t;
typedef struct yos_process yos_process_t;
typedef struct yos_service yos_service_t;
typedef struct yos_thread yos_thread_t;
typedef struct yos_timer yos_timer_t;

/* Read-only kernel topology returned by get_sys_info(). List fields point to
 * live fixed-memory head variables; dereference each field to obtain the
 * current first object. Banked heaps all begin at banked_heap_address. */
typedef struct yos_sys_info {
    const yos_memory_block_t *os_heap;
    uint16_t banked_heap_address;
    const uint8_t *bank_count;
    yos_process_t *volatile *processes;
    yos_thread_t *volatile *current_thread;
    yos_thread_t *volatile *suspended_threads;
    yos_thread_t *volatile *running_threads;
    yos_thread_t *volatile *waiting_threads;
    yos_thread_t *volatile *terminated_threads;
    yos_timer_t *volatile *timers;
    yos_event_t *volatile *events;
    yos_service_t *volatile *services;
    yos_service_t *volatile *private_services;
    yos_library_reference_t *volatile *library_references;
} yos_sys_info_t;

typedef void (*yos_entry_t)(void);
typedef void (*yos_handler_t)(void);

/* Read-only views of live kernel objects. System-list owners are packed far
 * pointers in bank,address-low,address-high order. Heap-block owners remain
 * fixed-memory process/thread addresses. */
typedef struct yos_far_owner {
    uint8_t bank;
    uint16_t address;
} yos_far_owner_t;

typedef struct yos_system_object {
    void *next;
    yos_far_owner_t owner;
} yos_system_object_t;

struct yos_memory_block {
    yos_memory_block_t *next;
    void *owner;
    uint8_t allocated;
    uint16_t size;
};

struct yos_process {
    yos_process_t *next;
    yos_far_owner_t owner;
    uint8_t flags;
    char name[8];
    yos_thread_t *main_thread;
    uint8_t bank;
};

struct yos_thread {
    yos_thread_t *next;
    yos_far_owner_t owner;
    uint16_t stack_pointer;
    uint8_t startup[9];
    uint8_t load_error;
    yos_event_t **wait;
    uint8_t wait_count;
    uint8_t state;
    int16_t error_number;
    yos_process_t *process;
    uint8_t bank;
    uint8_t call_depth;
    uint8_t call_frames[12];
};

struct yos_timer {
    yos_timer_t *next;
    yos_far_owner_t owner;
    yos_handler_t handler;
    uint16_t period;
    uint16_t remaining;
};

struct yos_event {
    yos_event_t *next;
    yos_far_owner_t owner;
    uint8_t state;
};

struct yos_service {
    yos_service_t *next;
    yos_far_owner_t owner;
    char name[16];
    void *interface;
};

struct yos_library_reference {
    yos_library_reference_t *next;
    yos_far_owner_t owner;
    yos_process_t *library;
};
/* Raw user allocations live in 0xC000-0xFFFF of one bank. A nonzero request
 * must not exceed 16377 bytes. Public allocation searches every configured
 * bank; preserve this packed far pointer when passing the block to
 * free_memory or shrink_memory. */
typedef void * [[xcc::far]] yos_user_ptr_t;

typedef struct yos_mouse_state {
    uint8_t x;
    uint8_t y;
    uint8_t buttons;
    uint8_t changed_buttons;
} yos_mouse_state_t;

/* One physical esxDOS block-device or partition descriptor. */
typedef struct yos_disk_info {
    uint8_t device;
    uint8_t flags;
    uint32_t blocks;
} yos_disk_info_t;

/* Directory-entry kinds returned by yos_t::readdir(). */
#define YOS_DIRECTORY_TYPE_UNKNOWN 0
#define YOS_DIRECTORY_TYPE_DIR     4
#define YOS_DIRECTORY_TYPE_REG     8
#define YOS_DIRECTORY_NAME_MAX     12

/* One short-name directory record owned by a yos_directory_t stream. */
typedef struct yos_directory_entry {
    ino_t d_ino;
    off_t d_size;
    uint8_t d_type;
    uint8_t d_attributes;
    char d_name[YOS_DIRECTORY_NAME_MAX + 1];
} yos_directory_entry_t;

/* libgpx public formats and constants, from upstream commit 0ef6f070.
 * GPL-2.0, Copyright (C) 2026 tomaz stih. */
/* Signed screen coordinate and unsigned screen dimension. */
typedef int16_t coord;
typedef uint16_t dim;

#define CO_BACK 0x00
#define CO_FORE 0x01
typedef uint8_t color;

#define BM_CPY 0x00
#define BM_XOR 0x01
#define BM_OR  0x02
typedef uint8_t bmode;

#define GPX_LP_SOLID        0xff
#define GPX_LP_DOTTED       0xaa
#define GPX_LP_DASHED       0xf0
#define GPX_LP_DASHED_SHORT 0xcc

#define GPX_TEXT_BG_OPAQUE      0x00
#define GPX_TEXT_BG_TRANSPARENT 0x01
typedef uint8_t textbg;

typedef struct rect_s {
    coord x0;
    coord y0;
    coord x1;
    coord y1;
} rect_t;

#define BMP_ENC_1BPP      0x0
#define BMP_ENC_1BPP_MASK 0x1
#define BMP_ENC_TINY      0x2
#define BMP_ENC_TINY_MASK 0x3
#define BMP_SIG(enc) ((uint8_t)(((enc) & 0x0f) << 4))
#define BMP_ENC(sig) ((uint8_t)(((sig) >> 4) & 0x0f))
#define BMP_STRIDE_ENC(stride) ((uint8_t)(((stride) - 1) & 0x0f))
#define BMP_STRIDE(sig) ((uint8_t)(((sig) & 0x0f) + 1))
#define BMP_SIG_STRIDE(enc, stride) \
    ((uint8_t)(BMP_SIG(enc) | BMP_STRIDE_ENC(stride)))
#define S_BMP BMP_SIG(BMP_ENC_1BPP)

typedef struct bmp_s {
    uint8_t signature;
    uint8_t w;
    uint8_t h;
    uint16_t size;
    uint8_t bitmap[];
} bmp_t;

typedef struct font_s {
    uint8_t flags;
    uint8_t first_ascii;
    uint8_t last_ascii;
    uint8_t empty_width;
    uint8_t max_glyph_width;
    uint8_t glyph_height;
    uint8_t advance;
    uint8_t descent;
    uint8_t data[];
} font_t;

#define FONT_FLAG_PROPORTIONAL 0x01
#define FONT_FLAG_OFFSETS_BE   0x02
#define FONT_FLAG_VECTOR       0x04

typedef struct gpx_s {
    dim width;
    dim height;
    uint8_t pages;
    textbg text_background;
} gpx_t;

#define GPX_SPRITE_BG_HEADER_SIZE  5
#define GPX_SPRITE_BG_PAYLOAD_SIZE 32
#define GPX_SPRITE_BG_SIZE \
    (GPX_SPRITE_BG_HEADER_SIZE + GPX_SPRITE_BG_PAYLOAD_SIZE)

typedef struct sprite_s {
    coord x;
    coord y;
    bmp_t *bitmap;
    bmp_t *background;
    const rect_t *clip;
} sprite_t;

#define GPXM_DEFAULT        0
#define GPXM_CPC_640X200    0
#define GPXM_CPC_320X200    1
#define GPXM_RASTA_1920X1080 0
typedef uint8_t gmode;

#define PG_DISPLAY 0x01
#define PG_WRITE   0x02

#define GPXSB_CURSOR_CLASSIC   0
#define GPXSB_CURSOR_STD       1
#define GPXSB_CURSOR_HOURGLASS 2
#define GPXSB_CURSOR_CARET     3
#define GPXSB_CURSOR_HAND      4
#define GPXSB_CURSOR_RESIZE    5

#define GPX_EDGE_LEFT   0x01
#define GPX_EDGE_TOP    0x02
#define GPX_EDGE_RIGHT  0x04
#define GPX_EDGE_BOTTOM 0x08
#define GPX_EDGE_ALL    0x0f

/*
 * Kernel interface returned by query_service("yos").
 *
 * ABI 1 groups related entries. Every entry occupies one 16-bit table slot;
 * yos.inc publishes the matching byte offsets for assembly code.
 */
typedef struct yos_s {
    /* Kernel identity and the global firmware-print hook. */
    /* Return the implemented YOS ABI version. */
    uint16_t (*version)(void);
    /* Return the Spectrum model detected while this ROM started. */
    enum yos_rom_model (*rom_model)(void);
    /* Inspect live kernel lists and heap roots. */
    const yos_sys_info_t *(*get_sys_info)(void);
    /* Install a RAM print sink and return the previously installed handler. */
    yos_handler_t (*set_print_hook)(yos_handler_t sink);

    /* Banked user memory. Kernel objects use the private fixed OS heap. */
    /* Allocate a process-owned block from any configured user bank. */
    yos_user_ptr_t (*allocate_memory)(size_t size);
    /* Release a block returned by allocate_memory(). */
    void (*free_memory)(yos_user_ptr_t memory);
    /* Shrink a live user block in place and release its unused tail. */
    yos_user_ptr_t (*shrink_memory)(yos_user_ptr_t memory, size_t size);

    /* Time and critical sections. */
    /* Return the wrapping 16-bit count of 50 Hz clock ticks. */
    uint16_t (*clock_ticks)(void);
    /* Enter a nestable kernel critical section. */
    void (*enter_critical_section)(void);
    /* Leave one level of a kernel critical section. */
    void (*leave_critical_section)(void);

    /* Timers and synchronization events. */
    /* Create a periodic timer that invokes handler every ticks + 1 ticks. */
    yos_timer_t *(*create_timer)(yos_handler_t handler, uint16_t ticks);
    /* Destroy a timer and stop future callbacks. */
    void (*destroy_timer)(yos_timer_t *timer);
    /* Create a binary synchronization event owned by owner. */
    yos_event_t *(*create_event)(void *owner);
    /* Destroy a synchronization event. */
    void (*destroy_event)(yos_event_t *event);
    /* Set or reset an event and return the same event on success. */
    yos_event_t *(*set_event)(yos_event_t *event, enum yos_event_state state);
    /* Block the calling thread until the event is set and consumed. */
    void (*wait_event)(yos_event_t *event);

    /* Threads. */
    /* Create a suspended thread in process with a fixed-memory stack. */
    yos_thread_t *(*create_thread)(yos_entry_t entry, uint16_t stack_size, yos_process_t *process);
    /* Terminate a thread and schedule its deferred cleanup. */
    void (*exit_thread)(yos_thread_t *thread);
    /* Move a runnable thread to the suspended queue. */
    void (*suspend_thread)(yos_thread_t *thread);
    /* Move a suspended thread to the runnable queue. */
    void (*resume_thread)(yos_thread_t *thread);

    /* Processes and loadable libraries. */
    /* Create a process around an already resident entry point. */
    yos_process_t *(*create_process)(const char *name, yos_entry_t entry, size_t stack_size);
    /* Load, relocate, and start an XPRG process image. */
    yos_process_t *(*load_process)(const char *path);
    /* Terminate the current process. */
    void (*exit_process)(void);
    /* Load or acquire an XPRG library and return its export table. */
    void *(*load_library)(const char *path, uint16_t flags);
    /* Point to the calling thread's most recent process/library load status. */
    uint8_t *process_load_error;

    /* Named services. */
    /* Return a borrowed interface pointer for a registered service. */
    void *(*query_service)(const char *name);
    /* Register an interface under name and return its service handle. */
    yos_service_t *(*register_service)(const char *name, void *interface);
    /* Unregister and destroy a service handle. */
    void (*unregister_service)(yos_service_t *service);

    /* Interrupt vectors. */
    /* Return the handler currently installed at a writable vector index. */
    yos_handler_t (*get_interrupt_handler)(uint8_t vector);
    /* Install handler at a writable vector index. */
    void (*set_interrupt_handler)(yos_handler_t handler, uint8_t vector);

    /* Input devices. */
    /* Return one encoded keyboard transition, or zero when none is queued. */
    uint8_t (*read_key)(void);
    /* Calibrate the absolute mouse cursor against current hardware counters. */
    void (*calibrate_mouse)(uint8_t x, uint8_t y);
    /* Copy sampled mouse state and consume its changed-buttons mask. */
    void (*read_mouse)(yos_mouse_state_t *state);

    /* POSIX-style esxDOS filesystem. */
    /* Point to the scheduler-virtualized kernel errno cell. */
    int *error_number;
    /* Open a file and return its descriptor, or -1 on error. */
    int (*open)(const char *path, int flags);
    /* Close an open file descriptor. */
    int (*close)(int fd);
    /* Read up to count bytes from a file descriptor. */
    ssize_t (*read)(int fd, void *buffer, size_t count);
    /* Write up to count bytes to a file descriptor. */
    ssize_t (*write)(int fd, const void *buffer, size_t count);
    /* Reposition a file descriptor and return its new offset. */
    off_t (*lseek)(int fd, off_t offset, int whence);
    /* Flush pending data for a file descriptor. */
    int (*fsync)(int fd);
    /* Remove a filesystem entry. */
    int (*unlink)(const char *path);
    /* Rename a filesystem entry. */
    int (*rename)(const char *old_path, const char *new_path);
    /* Change the current working directory. */
    int (*chdir)(const char *path);
    /* Copy the current working directory into buffer. */
    char *(*getcwd)(char *buffer, size_t size);
    /* Create a directory; mode is accepted for POSIX compatibility. */
    int (*mkdir)(const char *path, mode_t mode);
    /* Remove an empty directory. */
    int (*rmdir)(const char *path);
    /* Read metadata for a filesystem path. */
    int (*stat)(const char *path, struct stat *status);
    /* Read metadata for an open file descriptor. */
    int (*fstat)(int fd, struct stat *status);
    /* Open a directory stream. */
    yos_directory_t *(*opendir)(const char *path);
    /* Return the next entry from a directory stream. */
    yos_directory_entry_t *(*readdir)(yos_directory_t *directory);
    /* Rewind a directory stream to its first entry. */
    void (*rewinddir)(yos_directory_t *directory);
    /* Close a directory stream. */
    int (*closedir)(yos_directory_t *directory);
    /* Enumerate at most capacity available disks or partitions. */
    int (*enumerate_disks)(yos_disk_info_t *disks, size_t capacity);

    /* Native esxDOS commands. */
    /* Execute an esxDOS dot command and return zero or 0x100 + native error. */
    int (*exec_command)(const char *commandline);

    /* Graphics. */

    /* Create a process-owned graphics context for mode. */
    gpx_t *(*gpx_create)(gmode mode);
    /* Destroy a graphics context. */
    void (*gpx_destroy)(gpx_t *gpx);
    /* Select display or drawing pages where supported. */
    void (*gpx_set_page)(uint8_t operation, uint8_t page);
    /* Return the display width in pixels. */
    dim (*gpx_width)(void);
    /* Return the display height in pixels. */
    dim (*gpx_height)(void);
    /* Clear the current display. */
    void (*gpx_clear_screen)(void);
    /* Set opaque or transparent text rendering. */
    void (*gpx_set_text_background)(gpx_t *gpx, textbg background);

    /* Draw one clipped pixel. */
    void (*gpx_draw_pixel)(gpx_t *gpx, coord x, coord y,
                       color c, bmode mode, const rect_t *clip);
    /* Draw one clipped patterned line. */
    uint8_t (*gpx_draw_line)(gpx_t *gpx, coord x0, coord y0,
                         coord x1, coord y1, color c, bmode mode,
                         uint8_t pattern, const rect_t *clip);
    /* Draw one clipped bitmap. */
    void (*gpx_draw_bitmap)(gpx_t *gpx, coord x, coord y,
                        bmp_t *bitmap, const rect_t *clip);
    /* Save the background and show a sprite. */
    void (*gpx_show_sprite)(gpx_t *gpx, sprite_t *sprite);
    /* Restore the background behind a sprite. */
    void (*gpx_hide_sprite)(gpx_t *gpx, sprite_t *sprite);
    /* Draw a patterned rectangle outline. */
    void (*gpx_draw_rectangle)(gpx_t *gpx, rect_t *rectangle,
                           color c, bmode mode, uint8_t pattern,
                           const rect_t *clip);
    /* Fill a rectangle with a repeating pattern. */
    void (*gpx_fill_rectangle)(gpx_t *gpx, rect_t *rectangle,
                           color c, bmode mode, uint8_t *pattern,
                           uint8_t pattern_length, const rect_t *clip);

    /* Measure text width for a font. */
    coord (*gpx_measure_text)(const char *text, const font_t *font);
    /* Draw clipped text with a font. */
    void (*gpx_draw_text)(gpx_t *gpx, coord x, coord y, const char *text,
                      const font_t *font, color c, bmode mode,
                      const rect_t *clip);
    /* Return the system font. */
    const font_t *(*gpx_get_system_font)(void);
    /* Return the compact font. */
    const font_t *(*gpx_get_tiny_font)(void);
    /* Return a stock bitmap by identifier. */
    bmp_t *(*gpx_get_stock_bitmap)(uint8_t which);

    /* Draw a clipped circle outline. */
    void (*gpx_draw_circle)(gpx_t *gpx, coord x, coord y, coord radius,
                        color c, bmode mode, const rect_t *clip);
    /* Fill a clipped circle with a repeating pattern. */
    void (*gpx_fill_circle)(gpx_t *gpx, coord x, coord y, coord radius,
                        color c, bmode mode, uint8_t *pattern,
                        uint8_t pattern_length, const rect_t *clip);
    /* Draw selected edges of a clipped box. */
    uint8_t (*gpx_draw_box)(gpx_t *gpx, const rect_t *rectangle,
                        uint8_t edges, color c, bmode mode,
                        uint8_t pattern, const rect_t *clip);
} yos_t;

/* Resolve a named service through the application's RST 18 stub. */
void *query_service(const char *name);

#endif /* _YOS_H */
