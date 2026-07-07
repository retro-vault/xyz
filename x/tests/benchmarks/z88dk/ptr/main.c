#define BUF_LEN     1024
#define ARR_LEN     256
#define MAT_DIM     16
#define STRUCT_LEN  64
#define REPS        63

static unsigned char buffer[BUF_LEN];

struct rec {
    int a;
    int b;
    int c;
    int d;
};

static int arr[ARR_LEN];
static int mat[MAT_DIM][MAT_DIM];
static struct rec recs[STRUCT_LEN];

static int
index_walk_int(int *a, int n)
{
    int i;
    int sum = 0;

    for (i = 0; i < n; i++)
        sum += a[i];
    return sum;
}

static int
matrix_walk(int m[MAT_DIM][MAT_DIM])
{
    int i;
    int j;
    int sum = 0;

    for (i = 0; i < MAT_DIM; i++) {
        for (j = 0; j < MAT_DIM; j++)
            sum += m[i][j];
    }

    return sum;
}

static int
matrix_walk_global(void)
{
    int i;
    int j;
    int sum = 0;

    for (i = 0; i < MAT_DIM; i++) {
        for (j = 0; j < MAT_DIM; j++)
            sum += mat[i][j];
    }

    return sum;
}

static int
struct_field_sum(struct rec *p, int n)
{
    int i;
    int sum = 0;

    for (i = 0; i < n; i++)
        sum += p[i].a + p[i].b + p[i].c + p[i].d;
    return sum;
}

static int
multi_deref(struct rec *p)
{
    return p->a + p->b + p->c + p->d;
}

static int
multi_deref_sum(struct rec *p, int n)
{
    int i;
    int sum = 0;

    for (i = 0; i < n; i++)
        sum += multi_deref(&p[i]);
    return sum;
}

static void
init_data(void)
{
    unsigned int i;
    unsigned int seed = 0xC001u;

    for (i = 0; i < BUF_LEN; i++) {
        buffer[i] = (unsigned char)(seed & 0xFFu);
        seed = (unsigned int)(seed * 25173u + 13849u);
    }
}

static void
init_shapes(void)
{
    int i;
    int j;
    int k = 0;

    for (i = 0; i < ARR_LEN; i++)
        arr[i] = ((int)buffer[i * 2] | ((int)buffer[i * 2 + 1] << 8));

    for (i = 0; i < MAT_DIM; i++) {
        for (j = 0; j < MAT_DIM; j++) {
            mat[i][j] = arr[k];
            k++;
        }
    }

    k = 0;
    for (i = 0; i < STRUCT_LEN; i++) {
        recs[i].a = arr[k++];
        recs[i].b = arr[k++];
        recs[i].c = arr[k++];
        recs[i].d = arr[k++];
    }
}

static int
repeat_sum_index(void)
{
    int r;
    int sum = 0;

    for (r = 0; r < REPS; r++)
        sum += index_walk_int(arr, ARR_LEN);
    return sum;
}

static int
repeat_sum_matrix(void)
{
    int r;
    int sum = 0;

    for (r = 0; r < REPS; r++)
        sum += matrix_walk(mat);
    return sum;
}

static int
repeat_sum_matrix_global(void)
{
    int r;
    int sum = 0;

    for (r = 0; r < REPS; r++)
        sum += matrix_walk_global();
    return sum;
}

static int
repeat_sum_struct(void)
{
    int r;
    int sum = 0;

    for (r = 0; r < REPS; r++)
        sum += struct_field_sum(recs, STRUCT_LEN);
    return sum;
}

static int
repeat_sum_multi_deref(void)
{
    int r;
    int sum = 0;

    for (r = 0; r < REPS; r++)
        sum += multi_deref_sum(recs, STRUCT_LEN);
    return sum;
}

int
main(void)
{
    int sum;

    init_data();
    init_shapes();

    sum = repeat_sum_index();
    if ((unsigned int)sum != 0x8000u)
        return 1;
    sum = repeat_sum_matrix();
    if ((unsigned int)sum != 0x8000u)
        return 2;
    sum = repeat_sum_matrix_global();
    if ((unsigned int)sum != 0x8000u)
        return 3;
    sum = repeat_sum_struct();
    if ((unsigned int)sum != 0x8000u)
        return 4;
    sum = repeat_sum_multi_deref();
    if ((unsigned int)sum != 0x8000u)
        return 5;
    return 0;
}
