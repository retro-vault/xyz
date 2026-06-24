#include <stdio.h>
#include "utlist.h"

struct List {
    int value;
    struct List *prev;
    struct List *next;
};

struct List dummy = {9, NULL, NULL};

static void dump_els(struct List *list1, struct List *list2, struct List *els, int n) {
    for (int i=0; i < n; ++i) {
        printf("%d (prev %d next %d)\n", els[i].value,
            els[i].prev ? els[i].prev->value : -1,
            els[i].next ? els[i].next->value : -1);
    }
}

static void test_ll_concat_case(int a, int b) {
    struct List els[] = {
        {1, &dummy, &dummy},
        {2, &dummy, &dummy},
        {3, &dummy, &dummy},
        {4, &dummy, &dummy},
        {5, &dummy, &dummy},
        {6, &dummy, &dummy},
    };
    struct List *list1 = NULL;
    struct List *list2 = NULL;

    for (int i = 0; i < a; ++i) {
        LL_APPEND(list1, &els[i]);
    }
    for (int i = 0; i < b; ++i) {
        LL_APPEND(list2, &els[i+3]);
    }
    LL_CONCAT(list1, list2);

    printf("### %d %d\n", a, b);
    printf("list1 = %d, list2 = %d\n", list1 ? list1->value : -1, list2 ? list2->value : -1);
    dump_els(list1, list2, els, 6);
}

static void test_ll_concat() {
    printf("## test_ll_concat\n");
    for (int a = 0; a <= 2; ++a) {
        for (int b = 0; b <= 2; ++b) {
            test_ll_concat_case(a, b);
        }
    }
}

static void test_dl_concat_case(int a, int b) {
    struct List els[] = {
        {1, &dummy, &dummy},
        {2, &dummy, &dummy},
        {3, &dummy, &dummy},
        {4, &dummy, &dummy},
        {5, &dummy, &dummy},
        {6, &dummy, &dummy},
    };
    struct List *list1 = NULL;
    struct List *list2 = NULL;

    for (int i = 0; i < a; ++i) {
        DL_APPEND(list1, &els[i]);
    }
    for (int i = 0; i < b; ++i) {
        DL_APPEND(list2, &els[i+3]);
    }
    DL_CONCAT(list1, list2);

    printf("### %d %d\n", a, b);
    printf("list1 = %d, list2 = %d\n", list1 ? list1->value : -1, list2 ? list2->value : -1);
    dump_els(list1, list2, els, 6);
}

static void test_dl_concat() {
    printf("## test_dl_concat\n");
    for (int a = 0; a <= 3; ++a) {
        for (int b = 0; b <= 3; ++b) {
            test_dl_concat_case(a, b);
        }
    }
}

static void test_cdl_concat_case(int a, int b) {
    struct List els[] = {
        {1, &dummy, &dummy},
        {2, &dummy, &dummy},
        {3, &dummy, &dummy},
        {4, &dummy, &dummy},
        {5, &dummy, &dummy},
        {6, &dummy, &dummy},
    };
    struct List *list1 = NULL;
    struct List *list2 = NULL;

    for (int i = 0; i < a; ++i) {
        CDL_APPEND(list1, &els[i]);
    }
    for (int i = 0; i < b; ++i) {
        CDL_APPEND(list2, &els[i+3]);
    }
    CDL_CONCAT(list1, list2);

    printf("### %d %d\n", a, b);
    printf("list1 = %d, list2 = %d\n", list1 ? list1->value : -1, list2 ? list2->value : -1);
    dump_els(list1, list2, els, 6);
}

static void test_cdl_concat() {
    printf("## test_cdl_concat\n");
    for (int a = 0; a <= 3; ++a) {
        for (int b = 0; b <= 3; ++b) {
            test_cdl_concat_case(a, b);
        }
    }
}

int main()
{
    test_ll_concat();
    test_dl_concat();
    test_cdl_concat();

    return 0;
}
