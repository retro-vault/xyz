struct node {
    int value;
    struct node *next;
};

int
main(void)
{
    struct node first = {1, 0};
    struct node second = {2, 0};
    struct node third = {3, 0};

    first.next = &second;
    second.next = &third;

    if (first.next->value != 2)
        return 1;
    if (first.next->next != &third)
        return 2;
    if (first.next->next->value != 3)
        return 3;
    if (first.next->next->next != 0)
        return 4;
    return 0;
}
