struct node {
    struct node *next;
    unsigned int value;
};

static struct node nodes[7];

static __attribute__((noinline)) unsigned int
chase_and_update(struct node *head)
{
    unsigned int sum = 0;
    unsigned int repetition;

    for (repetition = 0; repetition < 10u; ++repetition) {
        struct node *cursor = head;

        while (cursor != (struct node *)0) {
            sum += cursor->value;
            cursor = cursor->next;
        }

        cursor = head;
        while (cursor != (struct node *)0) {
            cursor->value += 3u;
            cursor = cursor->next;
        }
    }
    return sum;
}

int
main(void)
{
    unsigned int index;

    for (index = 0; index < 7u; ++index) {
        nodes[index].next =
            index + 1u < 7u ? &nodes[index + 1u] : (struct node *)0;
        nodes[index].value = index + 1u;
    }

    if (chase_and_update(&nodes[0]) != 1225u)
        return 1;
    for (index = 0; index < 7u; ++index) {
        if (nodes[index].value != index + 31u)
            return 2;
    }
    return 0;
}
