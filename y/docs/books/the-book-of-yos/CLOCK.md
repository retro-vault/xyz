# Clock and Timers

*Yos* provides a monotonic software clock driven by the ZX Spectrum's
50 Hz frame interrupt, plus a general-purpose timer chain that the clock
itself is built on.

## The hardware source

The ZX Spectrum ULA raises a maskable interrupt 50 times per second, at
the start of the vertical blank. *Yos* receives it through the IM2 vector
at `0x5EFF` (see [The Boot Process](BOOT.md)); the scheduler,
`__thread_robin`, calls `__tmr_chain` on every interrupt. `main` installs
three kernel callbacks on that chain:

```c
/* installed in main.s, kernel-owned, fire on every tick */
tmr_install(__clock_tick, 0, NONE);
tmr_install(__kbd_scan,   0, NONE);
tmr_install(__mouse_scan, 0, NONE);
```

The keyboard scanner queues transitions. The mouse scanner converts
changes in the Kempston hardware counters into bounded absolute screen
coordinates, and accumulates button-change bits until `read_mouse`
consumes them.

## The clock counters

`drivers/_clock_state.s` keeps three `_INITIALIZED` counters:

| Symbol | Size | Meaning |
|---|---|---|
| `_clock_ticks` | 32 bits | frames since boot |
| `_clock_time` | 32 bits | whole seconds since boot |
| `_clock_sec_countdown` | 8 bits | frames left in the current second, starts at 50 |

`__clock_tick` (`drivers/_clock_tick.s`) increments `_clock_ticks`,
decrements the countdown, and every 50th call resets the countdown and
increments `_clock_time`. At 50 Hz, the 32-bit tick counter wraps after
roughly 2.7 years.

## `clock_ticks()`

```c
uint16_t ticks = yos->clock_ticks();
```

The public entry (`drivers/clock.s`, kernel routine `__clock`) returns
the **low 16 bits** of `_clock_ticks`. There is no `<time.h>` in the ROM
and no `CLOCKS_PER_SEC` macro — the rate is simply 50 ticks per second,
and the 16-bit value wraps every `65536 / 50 ≈ 21.8` minutes. The seconds
counter, `_clock_time`, is currently kernel-internal only.

### Measuring elapsed time

```c
uint16_t start = yos->clock_ticks();
do_some_work();
uint16_t elapsed_ticks = yos->clock_ticks() - start;   /* wraps correctly */
uint16_t elapsed_seconds = elapsed_ticks / 50;
```

Unsigned subtraction wraps correctly in C, so this stays safe across the
16-bit boundary as long as the interval itself is shorter than 21
minutes.

### Implementing a simple delay

```c
void delay_ticks(uint16_t ticks) {
    uint16_t end = yos->clock_ticks() + ticks;
    while ((int16_t)(end - yos->clock_ticks()) > 0)
        ;           /* spin-wait */
}

/* Wait approximately half a second */
delay_ticks(25);
```

> **Note.** Spin-waiting burns the thread's entire time slice. Other
> threads keep running — the scheduler still fires on every tick — but
> nothing useful happens in this one. Prefer ABI 2's `wait_event` with an
> event set by a timer instead: the scheduler removes the caller from the
> runnable queue entirely until it is signalled.

## Timer API

Timers let you register a callback that fires at a fixed interval,
measured in 50 Hz ticks. The kernel routines are `tmr_install` and
`tmr_uninstall`; the `yos_t` table exposes them as `create_timer` and
`destroy_timer`.

### The timer object

A timer is a 10-byte system object on `__sys_heap`:

```c
typedef struct timer_s {
    sysobj_t hdr;           /* 0: list link + owner */
    void (*hook)(void);     /* 4: callback */
    uint16_t ticks;         /* 6: reload value */
    uint16_t _tick_count;   /* 8: countdown */
} timer_t;
```

### Installing a timer

```c
/* Fire my_callback every 10th interrupt (= 5 times a second) */
yos_timer_t *t = yos->create_timer(my_callback, 9);

/* Fire my_callback on every single interrupt */
yos_timer_t *t = yos->create_timer(my_callback, 0);
```

`create_timer` registers the timer with owner `NONE`. Kernel code that
calls `tmr_install` directly can instead pass a process as the owner, in
which case the timer is destroyed automatically when that process is
reaped. The return value is a handle you can use to remove the timer
later, or `NULL` if `__sys_heap` is exhausted.

### Removing a timer

```c
yos->destroy_timer(t);
```

After this call, `my_callback` is never invoked again, and the
`timer_t` memory is freed. Do this from a thread, not from the callback
itself — see below.

### How timers work internally

On every interrupt, `__tmr_chain` (`kernel/_tmr_chain.s`) walks the list
of installed timers:

```c
void _tmr_chain(void) {
    timer_t *t = _tmr_first;
    while (t) {
        if (t->_tick_count == 0) {
            t->_tick_count = t->ticks;  /* reload the countdown */
            t->hook();                  /* fire the callback */
        } else {
            t->_tick_count--;           /* count down */
        }
        t = t->hdr.next;                /* read after the hook: never free t inside it */
    }
}
```

Because the callback fires when the countdown *reaches* zero and then
reloads to `ticks`, a timer fires once every **`ticks + 1`** interrupts:
`ticks = 0` fires on every interrupt, `ticks = 49` fires once a second,
and `ticks = 9` fires five times a second. The first firing happens
`ticks + 1` interrupts after installation.

### Timer callbacks run in interrupt context

`__tmr_chain` is called from inside the scheduler while interrupts are
disabled and before the next thread has been chosen. That has several
consequences for callbacks:

- Keep callbacks **very short**. Every microsecond spent inside one is a
  microsecond stolen from every thread and every other timer.
- The hook is called with `sdcccall(1)` and no arguments; `IX` and `IY`
  are saved around the whole chain, but everything else may be clobbered.
- Critical sections preserve the incoming interrupt state, so short calls
  like `set_event` and `resume_thread` are safe from inside a timer
  callback — their matching leave no longer re-enables interrupts in the
  middle of the scheduler.
- Do **not** execute `EI` yourself either.
- Do not create or destroy timers while the timer chain is running — it
  follows live list links after each callback. Keep allocation and
  expensive work in a thread, even though public heap mutations are now
  protected. Never block, suspend or exit the current thread, or perform
  disk I/O, from a callback.
- `set_event` and single-byte flag writes are safe. The scheduler scans
  waiting threads after the chain finishes, so an event set here can
  still take effect on the same tick.
- To hand a result back to a thread, set a flag or an event and let the
  thread process that result in its own context, outside the interrupt.

```c
static yos_t *yos;
static yos_event_t *tick_event;

void my_tick_callback(void) {
    yos->set_event(tick_event, YOS_EVENT_SET);
}

/* In the process thread, after acquiring the YOS table: */
tick_event = yos->create_event(NULL);
yos_timer_t *timer = yos->create_timer(my_tick_callback, 4);
while (work_remains()) {
    yos->wait_event(tick_event);  /* no scheduled CPU time while waiting */
    do_work();                  /* outside the interrupt handler */
}
yos->destroy_timer(timer);      /* stop signals before freeing the event */
yos->destroy_event(tick_event);
```

## Clock accuracy

The clock derives entirely from the 50 Hz frame interrupt, so its
accuracy depends on three things:

1. **Interrupt latency.** Holding a critical section across a frame
   boundary delays the interrupt but does not drop it; holding one for
   more than a whole frame loses a tick outright, and every lost tick
   costs the clock 20 ms permanently. Long esxDOS transfers are the most
   likely cause.
2. **Display timing.** The ULA generates the interrupt slightly
   differently on 48K and 128K machines. PAL hardware runs very close to
   50 Hz; NTSC clones run at 60 Hz instead, so the clock will run about
   20% fast on one.
3. **No real-time clock chip.** The ZX Spectrum has no battery-backed
   RTC. Both counters simply start at zero on power-up.
