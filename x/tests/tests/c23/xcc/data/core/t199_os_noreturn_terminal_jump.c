_Noreturn void stop_reg(unsigned value);
_Noreturn void stop_stack(unsigned first, unsigned second, unsigned third);
_Noreturn void stop_critical(unsigned value);

void terminal_register_call(unsigned value)
{
    volatile unsigned adjusted = value + 1u;
    stop_reg(adjusted);
}

void terminal_stack_call(unsigned value)
{
    volatile unsigned adjusted = value + 1u;
    stop_stack(adjusted, value, value);
}

[[sdcc::critical]] void terminal_critical_call(unsigned value)
{
    stop_critical(value);
}
