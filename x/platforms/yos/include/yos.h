/* Public YOS ABI 9 kernel and filesystem interface. */
#ifndef _YOS_H
#define _YOS_H

#include <stddef.h>
#include <stdint.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>

#define YOS_VERSION 0x09

enum yos_process_load_error {
    YOS_PROCESS_LOAD_OK = 0,
    YOS_PROCESS_LOAD_NOT_FOUND = 1,
    YOS_PROCESS_LOAD_NO_MEMORY = 2,
    YOS_PROCESS_LOAD_READ_ERROR = 3,
    YOS_PROCESS_LOAD_INVALID_IMAGE = 4,
    YOS_PROCESS_LOAD_START_ERROR = 5,
    YOS_PROCESS_LOAD_NOT_PROCESS = 6,
    YOS_PROCESS_LOAD_REQUIRES_NEWER_OS = 7,
    YOS_PROCESS_LOAD_BAD_CHECKSUM = 8,
    YOS_PROCESS_LOAD_BUSY = 9,
    YOS_PROCESS_LOAD_NO_PROCESS = 10,
    YOS_PROCESS_LOAD_INIT_ERROR = 11
};

/* Shared instances are keyed by full XPRG name and image ABI. */
#define YOS_LIBRARY_PRIVATE 0
#define YOS_LIBRARY_SHARED 1

#define YOS_KEY_DOWN 0x40
#define YOS_KEY_CODE 0x3f

enum yos_vector {
    YOS_VECTOR_RST18 = 2,
    YOS_VECTOR_RST20 = 3,
    YOS_VECTOR_RST28 = 4,
    YOS_VECTOR_RST30 = 5,
    YOS_VECTOR_RST38 = 6
};

enum yos_event_state {
    YOS_EVENT_RESET = 0,
    YOS_EVENT_SET = 1
};

typedef struct yos_event yos_event_t;
typedef struct yos_process yos_process_t;
typedef struct yos_service yos_service_t;
typedef struct yos_thread yos_thread_t;
typedef struct yos_timer yos_timer_t;

typedef void (*yos_entry_t)(void);
typedef void (*yos_handler_t)(void);
typedef void (*yos_putchar_hook_t)(char character);

typedef struct yos_mouse_state {
    uint8_t x;
    uint8_t y;
    uint8_t buttons;
    uint8_t changed_buttons;
} yos_mouse_state_t;

typedef struct yos_disk_info {
    uint8_t device;
    uint8_t flags;
    uint32_t blocks;
} yos_disk_info_t;

typedef struct yos_s {
    uint16_t (*version)(void);
    void *(*allocate_memory)(size_t size);
    void (*free_memory)(void *memory);
    uint16_t (*clock_ticks)(void);
    void (*enter_critical_section)(void);
    void (*leave_critical_section)(void);
    yos_timer_t *(*create_timer)(yos_handler_t handler, uint16_t ticks);
    void (*destroy_timer)(yos_timer_t *timer);
    yos_event_t *(*create_event)(void *owner);
    void (*destroy_event)(yos_event_t *event);
    yos_event_t *(*set_event)(yos_event_t *event,
                              enum yos_event_state state);
    yos_thread_t *(*create_thread)(yos_entry_t entry, uint16_t stack_size,
                                   yos_process_t *process);
    void (*exit_thread)(yos_thread_t *thread);
    void (*suspend_thread)(yos_thread_t *thread);
    void (*resume_thread)(yos_thread_t *thread);
    yos_process_t *(*create_process)(const char *name, yos_entry_t entry,
                                     size_t stack_size);
    void (*exit_process)(void);
    void *(*query_service)(const char *name);
    yos_service_t *(*register_service)(const char *name, void *interface);
    void (*unregister_service)(yos_service_t *service);
    yos_handler_t (*get_interrupt_handler)(uint8_t vector);
    void (*set_interrupt_handler)(yos_handler_t handler, uint8_t vector);
    uint8_t (*read_key)(void);
    void (*calibrate_mouse)(uint8_t x, uint8_t y);
    void (*read_mouse)(yos_mouse_state_t *state);
    int *error_number;
    int (*open)(const char *path, int flags);
    int (*close)(int fd);
    ssize_t (*read)(int fd, void *buffer, size_t count);
    ssize_t (*write)(int fd, const void *buffer, size_t count);
    off_t (*lseek)(int fd, off_t offset, int whence);
    int (*fsync)(int fd);
    int (*unlink)(const char *path);
    int (*rename)(const char *old_path, const char *new_path);
    int (*chdir)(const char *path);
    char *(*getcwd)(char *buffer, size_t size);
    int (*mkdir)(const char *path, mode_t mode);
    int (*rmdir)(const char *path);
    int (*stat)(const char *path, struct stat *status);
    int (*fstat)(int fd, struct stat *status);
    DIR *(*opendir)(const char *path);
    struct dirent *(*readdir)(DIR *directory);
    void (*rewinddir)(DIR *directory);
    int (*closedir)(DIR *directory);
    int (*enumerate_disks)(yos_disk_info_t *disks, size_t capacity);
    yos_process_t *(*load_process)(const char *path);
    uint8_t *process_load_error;
    /* ABI 9: returns a direct function-pointer table. Each successful
     * acquisition is retained until the caller's last thread exits.
     * Errors use process_load_error; code 6 means the wrong image kind.
     * query_service returns borrowed pointers and does not acquire.
     */
    void *(*load_library)(const char *path, uint16_t flags);
} yos_t;

void *query_service(const char *name);
yos_t *yos_get_api(void);
yos_putchar_hook_t yos_set_putchar_hook(yos_putchar_hook_t hook);
int enumerate_disks(yos_disk_info_t *disks, size_t capacity);

#endif /* _YOS_H */
