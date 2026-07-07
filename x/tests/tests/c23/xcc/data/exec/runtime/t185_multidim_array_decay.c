#include "xcc_exec_test.h"

static int mat[4][3];

static void fill_matrix(void) {
    int i;
    int j;
    int v = 1;

    for (i = 0; i < 4; ++i) {
        for (j = 0; j < 3; ++j)
            mat[i][j] = v++;
    }
}

static int sum_param(int m[4][3]) {
    int i;
    int j;
    int sum = 0;

    for (i = 0; i < 4; ++i) {
        for (j = 0; j < 3; ++j)
            sum += m[i][j];
    }
    return sum;
}

static int sum_global(void) {
    int i;
    int j;
    int sum = 0;

    for (i = 0; i < 4; ++i) {
        for (j = 0; j < 3; ++j)
            sum += mat[i][j];
    }
    return sum;
}

int main(void) {
    int (*p)[3];

    fill_matrix();
    XCC_CHECK_EQ_INT_ID(1, mat[0][0], 1);
    XCC_CHECK_EQ_INT_ID(2, mat[3][2], 12);
    XCC_CHECK_EQ_INT_ID(3, sum_param(mat), 78);
    XCC_CHECK_EQ_INT_ID(4, sum_global(), 78);

    p = mat;
    p[2][1] = 99;
    XCC_CHECK_EQ_INT_ID(5, mat[2][1], 99);
    XCC_CHECK_EQ_INT_ID(6, sum_param(mat), 169);
    return 0;
}
