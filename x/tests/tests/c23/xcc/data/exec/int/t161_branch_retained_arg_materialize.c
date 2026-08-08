#include "xcc_exec_test.h"

struct link {
    struct link *previous;
    struct link *next;
    int value;
};

static struct link new_link;

static struct link *
make_link(int value)
{
    new_link.previous = &new_link;
    new_link.next = &new_link;
    new_link.value = value;
    return &new_link;
}

static struct link *
replace_head(struct link *head, int value)
{
    struct link *replacement;

    if (head == 0)
        return make_link(value);
    replacement = make_link(value);
    replacement->next = head;
    return replacement;
}

int
main(void)
{
    struct link existing;
    struct link *result;

    existing.previous = &existing;
    existing.next = &existing;
    existing.value = 12;
    result = replace_head(&existing, 34);
    XCC_CHECK_EQ_UINT_ID(1, result == &new_link, 1u);
    XCC_CHECK_EQ_UINT_ID(2, result->next == &existing, 1u);
    XCC_CHECK_EQ_INT_ID(3, result->next->value, 12);
    return 0;
}
