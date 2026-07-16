#include "xcc_exec_test.h"

#define OPCODE_BASE 20

static unsigned char global_limit = 9;
static unsigned long global_bits = 8UL;
static char input_bytes[] = { 41, 73, 19, 0 };
static char output_bytes[4];
static char *input_cursor;
static char *output_cursor;

static int
label_c_first(int take)
{
    if (take)
        goto C;
    return 3;
C:
    return 7;
}

static int
label_c_second(int take)
{
    if (take)
        goto C;
    return 11;
C:
    return 13;
}

static int
label_condition_names(int take)
{
    if (take == 1)
        goto Z;
    if (take == 2)
        goto NZ;
    if (take == 3)
        goto NC;
    return 17;
Z:
    return 19;
NZ:
    return 23;
NC:
    return 29;
}

static int
case_expression(int opcode)
{
    switch (opcode) {
    case OPCODE_BASE + 1:
        return 31;
    case OPCODE_BASE + 2:
        return 37;
    case OPCODE_BASE + 9:
        return 41;
    default:
        return 43;
    }
}

static int
read_postincrement(void)
{
    return *input_cursor++;
}

static void
write_postincrement(char value)
{
    *output_cursor++ = value;
}

static int
byte_below_global(unsigned char value)
{
    return value < global_limit;
}

static int
global_bit_is_set(void)
{
    return (global_bits & 8UL) != 0;
}

static int
opcode_is_one(unsigned char opcode)
{
    return opcode == 1;
}

int
main(void)
{
    input_cursor = input_bytes;
    output_cursor = output_bytes;

    XCC_CHECK_EQ_INT_ID(1, label_c_first(0), 3);
    XCC_CHECK_EQ_INT_ID(2, label_c_first(1), 7);
    XCC_CHECK_EQ_INT_ID(3, label_c_second(0), 11);
    XCC_CHECK_EQ_INT_ID(4, label_c_second(1), 13);
    XCC_CHECK_EQ_INT_ID(5, label_condition_names(1), 19);
    XCC_CHECK_EQ_INT_ID(6, label_condition_names(2), 23);
    XCC_CHECK_EQ_INT_ID(7, label_condition_names(3), 29);

    XCC_CHECK_EQ_INT_ID(8, case_expression(21), 31);
    XCC_CHECK_EQ_INT_ID(9, case_expression(22), 37);
    XCC_CHECK_EQ_INT_ID(10, case_expression(29), 41);
    XCC_CHECK_EQ_INT_ID(11, case_expression(20), 43);

    XCC_CHECK_EQ_INT_ID(12, read_postincrement(), 41);
    XCC_CHECK_EQ_INT_ID(13, read_postincrement(), 73);
    XCC_CHECK_EQ_INT_ID(14, input_cursor - input_bytes, 2);
    write_postincrement(53);
    write_postincrement(61);
    XCC_CHECK_EQ_INT_ID(15, output_bytes[0], 53);
    XCC_CHECK_EQ_INT_ID(16, output_bytes[1], 61);
    XCC_CHECK_EQ_INT_ID(17, output_cursor - output_bytes, 2);

    XCC_CHECK_EQ_INT_ID(18, byte_below_global(8), 1);
    XCC_CHECK_EQ_INT_ID(19, byte_below_global(9), 0);
    XCC_CHECK_EQ_INT_ID(20, global_bit_is_set(), 1);
    global_bits = 4UL;
    XCC_CHECK_EQ_INT_ID(21, global_bit_is_set(), 0);
    XCC_CHECK_EQ_INT_ID(22, opcode_is_one(1), 1);
    XCC_CHECK_EQ_INT_ID(23, opcode_is_one(6), 0);
    return 0;
}
