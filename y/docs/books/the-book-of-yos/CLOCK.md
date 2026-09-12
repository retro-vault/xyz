# Clock and Timers

*Yos* provides a monotonic software clock driven by the ZX Spectrum's 50 Hz frame interrupt, and a general-purpose timer chain that the clock itself is built on.

## The Hardware Source

The ZX Spectrum ULA raises a maskable interrupt 50 times per second at the start of the vertical blank. *Yos* receives it through the IM2 vector at `0x5EFF` (see [The Boot Process](BOOT.md)); the scheduler `__thread_robin` calls `__tmr_chain` on every interrupt, and the clock driver's `__clock_tick` is one of the two timers installed by `main`:

```c
/* installed in main.s, kernel-owned, fire on every tick */
tmr_install(__clock_tick, 0, NONE);
tmr_install(__kbd_scan,   0, NONE);
```

## The Clock Counters

`drivers/_clock_state.s` keeps three `_INITIALIZED` counters:

| Symbol | Size | Meaning |
|---|---|---|
| `_clock_ticks` | 32 bits | frames since boot |
| `_clock_time` | 32 bits | whole seconds since boot |
| `_clock_sec_countdown` | 8 bits | frames left in the current second, starts at 50 |

`__clock_tick` (`drivers/_clock_tick.s`) increments `_clock_ticks`, decrements the countdown, and every 50th call resets the countdown and increments `_clock_time`. At 50 Hz the 32-bit tick counter wraps after about 2.7 years.

## `clock_ticks()`

```c
uint16_t ticks = yos->clock_ticks();
```

The public entry (`drivers/clock.s`, kernel routine `__clock`) returns the **low 16 bits** of `_clock_ticks`. There is no `<time.h>` in the ROM and no `CLOCKS_PER_SEC` macro; the rate is 50 ticks per second, and the 16-bit value wraps every `65536 / 50 ≈ 21.8` minutes. The seconds counter `_clock_time` is currently kernel-internal.

### Measuring elapsed time

```c
uint16_t start = yos->clock_ticks();
do_some_work();
uint16_t elapsed_ticks = yos->clock_ticks() - start;   /* wraps correctly */
uint16_t elapsed_seconds = elapsed_ticks / 50;
```

Unsigned subtraction wraps correctly in C, so this is safe across the 16-bit boundary as long as the interval is shorter than 21 minutes.

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

> **Note:** Spin-waiting burns the thread's entire time slice. Other threads continue to run (the scheduler still fires every tick), but nothing useful happens in this one. Polling an event that a timer callback sets is no cheaper in CPU, but it lets you wait on several sources at once.

## Timer API

Timers let you register a callback that fires at a fixed interval measured in 50 Hz ticks. The kernel routines are `tmr_install` and `tmr_uninstall`; the `yos_t` table exposes them as `create_timer` and `destroy_timer`.

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

`create_timer` registers the timer with owner `NONE`; kernel code that calls `tmr_install` directly can pass a process as the owner, in which case the timer is destroyed when the process is reaped. The return value is a handle you can use to remove the timer later, or `NULL` if `__sys_heap` is exhausted.

### Removing a timer

```c
yos->destroy_timer(t);
```

After this call, `my_callback` will no longer be invoked. The `timer_t` memory is freed. Do this from a thread, not from the callback itself (see below).

### How timers work internally

Every interrupt, `__tmr_chain` (`kernel/_tmr_chain.s`) walks the list of installed timers:

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

Because the callback fires when the countdown *reaches* zero and then reloads to `ticks`, a timer fires once every **`ticks + 1`** interrupts: `ticks = 0` fires on every interrupt, `ticks = 49` fires once a second, `ticks = 9` fires five times a second. The first firing happens `ticks + 1` interrupts after installation.

### Timer callbacks run in interrupt context

`__tmr_chain` is called from inside the scheduler while interrupts are disabled and before the next thread has been chosen. This has important consequences for callbacks:

- Keep callbacks **very short**. Every microsecond spent in a callback is a microsecond stolen from every thread and every other timer.
- The hook is called with `sdcccall(1)` and no arguments; `IX` and `IY` are saved around the whole chain, everything else may be clobbered.
- Critical sections preserve the incoming interrupt state. Short calls such as
  `set_event` and `resume_thread` are safe from a timer callback; their matching
  leave no longer enables interrupts in the middle of the scheduler.
- Do **not** execute `EI` yourself either.
- Do not create/destroy timers while the timer chain is running: it follows
  live list links after each callback. Keep allocation and expensive work in a
  thread even though public heap mutations are now protected. Never block,
  suspend/exit the current thread, or do disk I/O from a callback.
- `set_event` and single-byte flag writes are safe. The scheduler scans waiting
  threads after the chain, so an event can take effect on the same tick.
- To communicate a result back to a thread, set a flag or an event and let the thread process the result in its own context.

```c
/* Good: minimal work in the callback */
static volatile uint8_t tick_flag = 0;

void my_tick_callback(void) {
    tick_flag = 1;      /* just set a flag */
}

/* Somewhere in a thread: */
while (!tick_flag)
    ;
tick_flag = 0;
/* now do the real work here, safely */
```

## Clock Accuracy

The clock derives entirely from the 50 Hz frame interrupt. Its accuracy therefore depends on:

1. **Interrupt latency.** Holding a critical section across a frame boundary makes the interrupt arrive late but not disappear; holding it for more than a whole frame loses a tick, and every lost tick loses 20 ms from the clock permanently. Long esxDOS transfers are the most likely cause.
2. **Display timing.** The ULA generates the interrupt slightly differently on 48K and 128K machines. On PAL hardware the rate is very close to 50 Hz; NTSC clones run at 60 Hz and the clock will run fast by 20%.
3. **No real-time clock chip.** The ZX Spectrum has no battery-backed RTC. Both counters start at zero on power-up.
