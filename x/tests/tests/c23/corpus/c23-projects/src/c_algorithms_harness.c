#include "queue.h"

int main(void) {
    Queue *q = queue_new();
    char a = 'a';
    char b = 'b';
    char *head;
    char *tail;

    if (!q) return 1;
    if (!queue_push_head(q, &a)) return 2;
    if (!queue_push_tail(q, &b)) return 3;
    head = (char *)queue_pop_head(q);
    tail = (char *)queue_pop_tail(q);
    queue_free(q);
    return head == &a && tail == &b ? 0 : 4;
}
