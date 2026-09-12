# YOS API Reference

This is the complete ABI 1 application reference for `y/include/yos.h` and
the YOS XCC platform helpers. All kernel calls use the `sdcccall(1)` ABI used
by the default XCC mode. Include `<yos.h>` and obtain the cached table once:

```c
yos_t *yos = yos_get_api();
if (!yos || yos->version() < YOS_VERSION)
    return 1;
```

Opaque handles (`yos_timer_t`, `yos_event_t`, `yos_thread_t`,
`yos_process_t`, and `yos_service_t`) must only be passed back to the API that
created them. A returned null pointer means failure unless stated otherwise.

## Service bootstrap and platform helpers

### `void *query_service(const char *name)`

Executes the RST `0x18` named-service lookup. It returns the service interface
or `NULL`. This is the fundamental operation that gives an application access
to `yos.h` tables.

```c
yos_t *direct = (yos_t *)query_service("yos");
gpx_api_t *graphics = (gpx_api_t *)query_service("gpx");
```

### `yos_t *yos_get_api(void)`

Returns the `"yos"` table cached by the YOS CRT during startup.

```c
yos_t *yos = yos_get_api();
```

### `yos_putchar_hook_t yos_set_putchar_hook(yos_putchar_hook_t hook)`

Installs the process-local sink used by standard character output and returns
the previous hook. `NULL` restores silent output.

```c
static void sink(char ch) { queue_for_alto(ch); }
yos_putchar_hook_t old = yos_set_putchar_hook(sink);
puts("sent through sink");
yos_set_putchar_hook(old);
```

### `int enumerate_disks(yos_disk_info_t *disks, size_t capacity)`

POSIX-style convenience wrapper for the table member. It returns the number
of records written, or `-1` and sets `errno`.

```c
yos_disk_info_t disks[4];
int count = enumerate_disks(disks, 4);
```

## Identity and memory

### `uint16_t version(void)`

Returns the kernel ABI version.

```c
if (yos->version() < YOS_VERSION) return 1;
```

### `void *allocate_memory(size_t size)`

Allocates a raw block from the user heap. The public adapter records the
current process as owner when one is running. Use `free_memory`, not `free`,
for this pointer.

```c
void *raw = yos->allocate_memory(64);
```

### `void free_memory(void *memory)`

Returns a raw YOS allocation. Passing `NULL` is harmless.

```c
yos->free_memory(raw);
```

For normal C code prefer `malloc`, `calloc`, `realloc`, `aligned_alloc`, and
`free`; the platform implementation builds their metadata on these two calls.

## Clock and critical sections

### `uint16_t clock_ticks(void)`

Returns the low 16 bits of the 50 Hz monotonic clock.

```c
uint16_t start = yos->clock_ticks();
```

### `void enter_critical_section(void)`

On the outermost entry, records whether maskable interrupts were enabled,
disables them, and increments the nesting depth. Nested calls are supported to
a maximum depth of 127. Registers and flags are preserved.

```c
yos->enter_critical_section();
```

### `void leave_critical_section(void)`

Decrements the nesting depth. The final matching leave restores the outer
caller's interrupt state; it does not blindly enable interrupts. An unmatched
leave is a harmless no-op, but callers should still balance every path.

```c
shared_value = 7;
yos->leave_critical_section();
```

Always balance the pair and keep the protected region short.

## Timers

### `yos_timer_t *create_timer(yos_handler_t handler, uint16_t ticks)`

Creates a periodic timer. The first invocation occurs after `ticks + 1`
50 Hz frames. Publication in the timer chain is atomic. The callback has no
arguments and runs in scheduler context.

```c
static void pulse(void) { ++pulses; }
yos_timer_t *timer = yos->create_timer(pulse, 49);
```

### `void destroy_timer(yos_timer_t *timer)`

Unlinks and frees a timer. Public ABI 1 timers are kernel-owned, so explicitly
destroy every successful timer. Removal is atomic with respect to threads,
but do not destroy a timer from a callback while the active chain is walking
it.

```c
if (timer) yos->destroy_timer(timer);
```

## Events

### `yos_event_t *create_event(void *owner)`

Creates a reset event. Application code normally passes `NULL`; kernel-aware
launchers may pass a valid owner object.

```c
yos_event_t *event = yos->create_event(NULL);
```

### `void destroy_event(yos_event_t *event)`

Unlinks and frees a registered event.

```c
if (event) yos->destroy_event(event);
```

Destruction is atomically removed from the kernel list, but the caller must
ensure no other thread will subsequently use the handle.

### `yos_event_t *set_event(yos_event_t *event, enum yos_event_state state)`

Sets `YOS_EVENT_SET` or `YOS_EVENT_RESET`. Returns `event` if it remains a
registered object, otherwise `NULL`. The validation and state change are one
protected operation; this short call is safe from a timer callback.

```c
if (!yos->set_event(event, YOS_EVENT_SET)) handle_stale_event();
```

ABI 1 has no public event-wait call.

## Threads

### `yos_thread_t *create_thread(yos_entry_t entry, uint16_t stack_size, yos_process_t *process)`

Creates a suspended thread owned by `process` with its own stack.

```c
static void worker(void) { for (;;) { } }
yos_thread_t *thread = yos->create_thread(worker, 256, process);
```

### `void exit_thread(yos_thread_t *thread)`

Moves a runnable thread to the terminated queue for scheduler cleanup.

```c
yos->exit_thread(thread);
```

### `void suspend_thread(yos_thread_t *thread)`

Moves a runnable thread to the suspended queue. Suspending the current thread
yields to another runnable thread.

```c
yos->suspend_thread(thread);
```

### `void resume_thread(yos_thread_t *thread)`

Moves a suspended thread to the runnable queue.

```c
yos->resume_thread(thread);
```

There is no join operation or public current-thread/current-process getter.

## Processes and XPRG loading

### `yos_process_t *create_process(const char *name, yos_entry_t entry, size_t stack_size)`

Creates a process and a runnable initial thread around code that is already
resident.

```c
static void child(void) { yos_get_api()->exit_process(); }
yos_process_t *process = yos->create_process("child", child, 256);
```

The process object keeps at most seven name characters plus NUL. The returned
process has no parent/creator field, even when this call is made by a process.

### `void exit_process(void)`

Terminates the calling thread; the process survives while another member
thread exists. Cleanup occurs in
the scheduler and the call does not normally return.

```c
yos->exit_process();
```

The platform CRT uses this when `main` returns or `exit` is called. ABI 1 has
no exit-status channel, process parent, or wait operation.

### `yos_process_t *load_process(const char *path)`

Reads an XPRG v1 process, validates its kind, minimum OS ABI, CRC, XL records,
allocates and relocates it, creates its declared stack, and schedules it.

```c
yos_process_t *loaded = yos->load_process("EDITOR.SYS");
```

The caller does not become a parent. Once created, the loaded process has no
stored relationship to the process that loaded it.

### `uint8_t *process_load_error`

Points to the last loader error byte. It is data, not a function pointer.
The scheduler saves and restores its value per thread. Loading is synchronous,
so read it after `load_process` or `load_library` returns `NULL`:

```c
loaded = yos->load_process("EDITOR.SYS");
if (!loaded) {
    enum yos_process_load_error why =
        (enum yos_process_load_error)*yos->process_load_error;
}
```

Values are `YOS_PROCESS_LOAD_OK`, `NOT_FOUND`, `NO_MEMORY`, `READ_ERROR`,
`INVALID_IMAGE`, `START_ERROR`, `NOT_PROCESS`, `REQUIRES_NEWER_OS`, and
`BAD_CHECKSUM` (0 through 8). Code 6 means wrong image kind for the
selected loading API. The current ABI also defines `BUSY` (9), `NO_PROCESS`
(10), and `INIT_ERROR` (11).

A competing or recursive loader call does not wait: it returns `NULL` with
`BUSY`. Its status cannot overwrite the interrupted thread's saved status.

### `void *load_library(const char *path, uint16_t flags)`

Loads a relocatable XPRG service and returns its relocated function-pointer
table. `YOS_LIBRARY_PRIVATE` always creates a private instance;
`YOS_LIBRARY_SHARED` reuses the same full name and image ABI. Initialization
and self-registration run once, after relocation. Each successful call
retains a reference until the acquiring process's last thread exits.
There is no explicit unload call. `query_service` does not retain a library.
The call is synchronous and normally preemptible; only short shared-state
commits and nested firmware/descriptor transactions mask interrupts. Do not
asynchronously terminate a thread while it is loading or initializing code.

```c
shelllib_api_t *library = yos->load_library(
    "shelllib.svc", YOS_LIBRARY_SHARED);
if (library) library->probe();
```

See [Loadable Libraries](../the-book-of-yos/LIBRARIES.md) and the complete
`y/tests/shell-yos/shelllib.s` / `shelllib.h` fixture for the initializer
contract, staged registration, ownership and supported image limits.

## Named services

### `void *query_service(const char *name)` table member

Performs the same lookup as the global RST stub. Use the global spelling for
bootstrap and either spelling afterwards.

```c
void *interface = yos->query_service("counter");
```

### `yos_service_t *register_service(const char *name, void *interface)`

Copies a case-sensitive name and publishes the resident interface pointer.
Keep names to 15 characters and keep the table alive while registered.

```c
yos_service_t *service = yos->register_service("counter", &counter_api);
```

### `void unregister_service(yos_service_t *service)`

Removes and frees a registration; clients must no longer use its pointer.

```c
if (service) yos->unregister_service(service);
```

ABI 1 records current-process ownership, so ordinary registrations are
reaped on process exit. During library initialization registration is
library-owned and staged until success. Never manually unregister a
loader-managed library service. Registration, lookup, and removal are atomic,
but invoking the returned interface is not: mutable service state needs its
own synchronization, and unregistering requires coordination with borrowers.

## Restart handlers

### `yos_handler_t get_interrupt_handler(uint8_t vector)`

Returns the handler currently installed in a writable restart slot. Valid
public indexes are `YOS_VECTOR_RST18`, `RST20`, `RST28`, `RST30`, and `RST38`.

```c
yos_handler_t old = yos->get_interrupt_handler(YOS_VECTOR_RST20);
```

### `void set_interrupt_handler(yos_handler_t handler, uint8_t vector)`

Atomically replaces a writable restart slot.

```c
yos->set_interrupt_handler(my_rst20, YOS_VECTOR_RST20);
```

Do not replace RST 18 unless you also preserve named-service lookup. Handlers
must obey the register/return contract of their restart and remain resident.

## Keyboard and mouse

### `uint8_t read_key(void)`

Returns zero when the transition queue is empty. Otherwise bits 0–5 contain a
one-based raw matrix key and `YOS_KEY_DOWN` distinguishes press from release.

```c
uint8_t key_event = yos->read_key();
if (key_event & YOS_KEY_DOWN) key_pressed(key_event & YOS_KEY_CODE);
```

### `void calibrate_mouse(uint8_t x, uint8_t y)`

Sets the logical cursor position used for subsequent Kempston deltas.

```c
yos->calibrate_mouse(128, 96);
```

### `void read_mouse(yos_mouse_state_t *state)`

Polls the hardware and fills `{x, y, buttons, changed_buttons}`.

```c
yos_mouse_state_t mouse;
yos->read_mouse(&mouse);
```

## Filesystem error cell

### `int *error_number`

Points to the kernel filesystem error cell. Direct table calls update it. The
libc wrappers copy it into their process-local `errno` when they fail.
The kernel cell is saved/restored per thread, but libc's copy is not; see
[Concurrency](MEMORY-TIME-AND-CONCURRENCY.md) before sharing libc calls.

```c
int fd = yos->open("DATA.BIN", O_RDONLY);
if (fd < 0) direct_error = *yos->error_number;
```

## Raw file calls

The members in this section mirror the functions from `<fcntl.h>`,
`<unistd.h>`, `<sys/stat.h>`, and `<dirent.h>`. Prefer those standard wrappers;
the direct form is shown because every `yos_t` entry is part of the ABI.

Descriptors and the esxDOS current directory are system-wide. Each descriptor
call is serialized from validation through native I/O and state commit, so one
call cannot corrupt kernel bookkeeping; append seek plus write is atomic.
Sequences of calls are not transactions: coordinate `chdir` plus `open`, and
do not close a descriptor or free a buffer while another thread uses it.
`readdir` reuses storage in its `DIR`, so copy a record before another read on
that stream.

### `int open(const char *path, int flags)`

Opens an 8.3 path and returns a descriptor or `-1`.

```c
int fd = yos->open("DATA.BIN", O_RDWR | O_CREAT);
```

### `int close(int fd)`

Closes a filesystem descriptor.

```c
if (fd >= 0) yos->close(fd);
```

### `ssize_t read(int fd, void *buffer, size_t count)`

Reads up to `count` raw bytes; zero is end of file.

```c
ssize_t got = yos->read(fd, buffer, sizeof buffer);
```

### `ssize_t write(int fd, const void *buffer, size_t count)`

Writes raw bytes and returns the count written.

```c
ssize_t put = yos->write(fd, buffer, length);
```

### `off_t lseek(int fd, off_t offset, int whence)`

Moves the 32-bit file position and returns the new position.

```c
off_t end = yos->lseek(fd, 0, SEEK_END);
```

### `int fsync(int fd)`

Requests that buffered file data be committed.

```c
if (yos->fsync(fd) < 0) save_failed();
```

### `int unlink(const char *path)`

Deletes a file.

```c
yos->unlink("OLD.TMP");
```

### `int rename(const char *old_path, const char *new_path)`

Renames a path on the backing filesystem.

```c
yos->rename("DRAFT.TXT", "FINAL.TXT");
```

### `int chdir(const char *path)`

Changes the process-visible current directory maintained by esxDOS.

```c
yos->chdir("/APPS");
```

### `char *getcwd(char *buffer, size_t size)`

Writes the current path and returns `buffer`, or `NULL`.

```c
char cwd[64];
if (yos->getcwd(cwd, sizeof cwd)) use_path(cwd);
```

### `int mkdir(const char *path, mode_t mode)`

Creates a directory. The Unix permission bits are accepted but ignored.

```c
yos->mkdir("SAVES", S_IRUSR | S_IWUSR);
```

### `int rmdir(const char *path)`

Removes an empty directory.

```c
yos->rmdir("EMPTY");
```

### `int stat(const char *path, struct stat *status)`

Reads path metadata.

```c
struct stat status;
if (yos->stat("DATA.BIN", &status) == 0) size = status.st_size;
```

### `int fstat(int fd, struct stat *status)`

Reads metadata for an open descriptor.

```c
if (yos->fstat(fd, &status) == 0 && S_ISREG(status.st_mode)) use_file();
```

### `DIR *opendir(const char *path)`

Allocates and opens a directory stream.

```c
DIR *directory = yos->opendir(".");
```

### `struct dirent *readdir(DIR *directory)`

Returns the next reused directory record, or `NULL` at end/error.

```c
struct dirent *entry = yos->readdir(directory);
```

### `void rewinddir(DIR *directory)`

Returns a stream to its first entry.

```c
yos->rewinddir(directory);
```

### `int closedir(DIR *directory)`

Closes and frees a directory stream.

```c
if (directory) yos->closedir(directory);
```

### `int enumerate_disks(yos_disk_info_t *disks, size_t capacity)`

Writes at most `capacity` records and returns the number written or `-1`.

```c
yos_disk_info_t disk[2];
int present = yos->enumerate_disks(disk, 2);
```

## POSIX wrapper reference

The YOS backend exports the standard spellings below. Each delegates to the
corresponding raw member, copies kernel errors to `errno`, and returns the
usual success/failure result:

```c
int fd = open("DATA.BIN", O_RDONLY);                /* open */
ssize_t n = read(fd, buffer, sizeof buffer);         /* read */
n = write(fd, buffer, length);                       /* write */
off_t at = lseek(fd, 0, SEEK_SET);                   /* lseek */
int ok = fsync(fd);                                  /* fsync */
ok = close(fd);                                      /* close */
ok = rename("A.TXT", "B.TXT");                     /* rename */
ok = unlink("B.TXT");                              /* unlink */
ok = chdir("/APPS");                               /* chdir */
char *here = getcwd(buffer, sizeof buffer);           /* getcwd */
ok = mkdir("NEW", S_IRUSR | S_IWUSR);              /* mkdir */
ok = rmdir("NEW");                                 /* rmdir */
ok = stat("DATA.BIN", &status);                     /* stat */
ok = fstat(fd, &status);                             /* fstat */
DIR *d = opendir(".");                              /* opendir */
struct dirent *e = readdir(d);                       /* readdir */
rewinddir(d);                                        /* rewinddir */
ok = closedir(d);                                    /* closedir */
int disks_found = enumerate_disks(disks, capacity);  /* enumerate_disks */
```

The kernel error cell is per-thread; linked libc `errno` is only process-local
in ABI 1. Multithreaded code should use the raw entry plus
`*yos->error_number`, or protect a wrapper call and its immediate `errno` read.

For descriptors 1 and 2, `write` routes each byte to the installed output
hook and succeeds even when no hook exists. Descriptor 0 reads as immediate
end of file. Closing descriptors 0–2 succeeds without a kernel call.
