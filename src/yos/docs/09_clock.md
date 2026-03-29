# The Clock

*Yos* provides a monotonic software clock driven by the ZX Spectrum's 50 Hz screen-blank interrupt. The clock is exposed through a partial implementation of the standard C `<time.h>` header.

## The Hardware Source

The ZX Spectrum generates a hardware interrupt 50 times per second when the ULA chip finishes drawing the screen (the *vertical blank* period). *Yos* hooks this interrupt via the `RST 0x38` vector. Every tick, `_tmr_chain()` is called from within the interrupt handler, and the clock driver's `_clock_tick()` is one of the timer callbacks installed during OS startup:

```c
/* installed in main() */
tmr_install(_clock_tick, 0, NONE);   /* 0 = fire on every tick */
```

`_clock_tick` increments an internal `clock_t` counter once per interrupt. Because the interrupt fires exactly 50 times per second, the counter advances at 50 units per second.

## `clock()` and `CLOCKS_PER_SEC`

```c
#include <time.h>

clock_t clock(void);
```

`clock()` returns the current value of the tick counter. Divide by `CLOCKS_PER_SEC` to convert to seconds:

```c
#define CLOCKS_PER_SEC  50          /* 50 Hz interrupt = 50 ticks/second */
typedef unsigned int clock_t;       /* 16-bit unsigned on Z80 */
```

### Measuring elapsed time

```c
clock_t start = clock();
do_some_work();
clock_t elapsed_ticks = clock() - start;
clock_t elapsed_seconds = elapsed_ticks / CLOCKS_PER_SEC;
```

Because `clock_t` is a 16-bit unsigned integer, it overflows after `65535 / 50 ≈ 21.8 minutes`. If your program runs longer than that, you must handle the wrap-around yourself:

```c
clock_t before = clock();
/* ... long operation ... */
clock_t after  = clock();
/* Handles wrap-around correctly with unsigned subtraction */
clock_t ticks  = (clock_t)(after - before);
```

Unsigned subtraction wraps correctly in C, so this is safe even across the overflow boundary.

### Implementing a simple delay

```c
void delay_ticks(clock_t ticks) {
    clock_t end = clock() + ticks;
    while ((clock_t)(end - clock()) > 0)
        ;           /* spin-wait */
}

/* Wait approximately half a second */
delay_ticks(CLOCKS_PER_SEC / 2);
```

> **Note:** Spin-waiting consumes the full CPU for the duration of the delay. Other threads continue to run (the scheduler still fires every 50 Hz tick), but the waiting thread burns its time slot doing nothing. For longer waits, consider using `thread_suspend` driven by a timer callback instead.

## Timer API

The clock is built on top of the general-purpose timer system. Timers let you register a callback that fires at a specified interval, measured in 50 Hz ticks.

### Installing a timer

```c
#include <kernel/timer.h>

/* Fire my_callback every 10 ticks (= every 0.2 seconds) */
timer_t *t = tmr_install(my_callback, 10, owner);

/* Fire my_callback on every single tick (ticks = 0 means "always") */
timer_t *t = tmr_install(my_callback, 0, owner);
```

The return value is a handle you can use to remove the timer later.

### Removing a timer

```c
tmr_uninstall(t);
```

After this call, `my_callback` will no longer be invoked. The `timer_t` memory is freed.

### How timers work internally

Every 50 Hz tick, `_tmr_chain()` walks the list of installed timers:

```c
void _tmr_chain() {
    timer_t *t = _tmr_first;
    while (t) {
        if (t->_tick_count == 0) {
            t->_tick_count = t->ticks;  /* reload the countdown */
            t->hook();                  /* fire the callback */
        } else {
            t->_tick_count--;           /* count down */
        }
        t = t->hdr.next;
    }
}
```

A timer with `ticks = 0` fires on every tick because its `_tick_count` is always 0. A timer with `ticks = 50` fires once per second: the counter decrements from 50 down to 0, fires, then reloads to 50.

### Timer callbacks run in interrupt context

`_tmr_chain()` is called from inside the 50 Hz interrupt handler, which runs while interrupts are disabled. This has important consequences for callbacks:

- Keep callbacks **very short**. Every microsecond spent in a callback is a microsecond stolen from the thread scheduler and from other timer callbacks.
- Do **not** call `ir_disable()` inside a callback — interrupts are already disabled.
- Do **not** call functions that re-enable interrupts (like `ei` directly) inside a callback.
- Do **not** allocate or free memory inside a callback — the allocator is not interrupt-safe.
- To communicate a result back to a thread, set a flag or signal an event; let the thread process the result in its own context.

```c
/* Good: minimal work in the callback */
static volatile uint8_t tick_flag = 0;

void my_tick_callback() {
    tick_flag = 1;      /* just set a flag */
}

/* Somewhere in a thread: */
while (!tick_flag)
    ;
tick_flag = 0;
/* now do the real work here, safely */
```

## Clock Accuracy

The clock derives entirely from the 50 Hz screen-blank interrupt. The accuracy of the clock therefore depends on:

1. **Interrupt latency.** Holding `di` for more than a fraction of a millisecond can cause a tick to be missed. Each missed tick loses 20 ms from the clock permanently.
2. **Display timing accuracy.** The ULA generates the interrupt slightly differently depending on whether the machine is a 48K or 128K model and on the state of the TV border. On standard hardware the frequency is very close to exactly 50 Hz (UK/Europe) or 60 Hz (some clones for NTSC regions).
3. **No real-time clock chip.** The ZX Spectrum has no battery-backed RTC. The clock starts at zero on power-up and cannot be automatically synchronised. For applications that need wall-clock time, the value should be set by the user or synchronised over a network connection.

> If your application disables interrupts for extended periods — for example, when loading data from a tape interface — the clock will drift. For time-critical applications, periodically verify the clock against an external reference.
