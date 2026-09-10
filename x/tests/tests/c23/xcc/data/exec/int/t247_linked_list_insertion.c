#define NULL ((void *)0)
struct Node { int data; struct Node *next; };
__attribute__((noinline)) struct Node *sort_links(struct Node *list)
{
    struct Node *k, *nodeList;
    /* need at least two items to sort */
    if (list == NULL || list->next == NULL)
    {
        return list;
    }

    nodeList = list;
    k = list->next;
    nodeList->next = NULL; /* 1st node is new list */
    while (k != NULL)
    {
        struct Node *ptr;
        /* check if insert before first */
        if (nodeList->data > k->data)
        {
            struct Node *tmp;
            tmp = k;
            k = k->next;  // important for the while
            tmp->next = nodeList;
            nodeList = tmp;
            continue;
        }

        // from begin up to end
        // finds [i] > [i+1]
        for (ptr = nodeList; ptr->next != NULL; ptr = ptr->next)
        {
            if (ptr->next->data > k->data)
                break;
        }

        // if found (above)
        if (ptr->next != NULL)
        {
            struct Node *tmp;
            tmp = k;
            k = k->next;  // important for the while
            tmp->next = ptr->next;
            ptr->next = tmp;
            continue;
        }
        else
        {
            ptr->next = k;
            k = k->next;  // important for the while
            ptr->next->next = NULL;
            continue;
        }
    }
    return nodeList;
}


int main(void)
{
    struct Node nodes[6];
    unsigned seed, i, seen, count;
    for (seed = 0; seed < 32; ++seed) {
        for (i = 0; i < 6; ++i) {
            nodes[i].data = (int)((seed * 13u + i * 7u) % 17u) - 8;
            nodes[i].next = i == 5 ? NULL : &nodes[i + 1];
        }
        struct Node *head = sort_links(nodes);
        struct Node *current = head;
        int previous = -32767;
        seen = 0;
        count = 0;
        while (current != NULL && count < 6) {
            if (current->data < previous) return 1;
            previous = current->data;
            for (i = 0; i < 6; ++i) {
                if (current == &nodes[i]) {
                    if (seen & (1u << i)) return 2;
                    seen |= 1u << i;
                    break;
                }
            }
            if (i == 6) return 3;
            ++count;
            current = current->next;
        }
        if (count != 6 || current != NULL || seen != 63u) return 4;
    }
    return 0;
}
