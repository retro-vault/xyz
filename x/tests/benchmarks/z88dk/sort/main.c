#define QN    100
#define IN    64
#define QREPS 16
#define IREPS 24

#define QSUM  23215u
#define ISUM  43825u

static int qa[QN];
static int ia[IN];
static unsigned int lcg_state;

static int
next_val(void)
{
    lcg_state = (unsigned int)((lcg_state * 181u + 17u) & 0xffffu);
    return (int)(lcg_state & 0x7fffu);
}

static int
cmp_int(const void *a, const void *b)
{
    int x = *(const int *)a;
    int y = *(const int *)b;

    if (x < y)
        return -1;
    if (x > y)
        return 1;
    return 0;
}

static void
qsort_rec(int *v, int lo, int hi, int (*cmp)(const void *, const void *))
{
    int i;
    int j;
    int pivot;
    int tmp;

    if (lo >= hi)
        return;

    pivot = v[hi];
    i = lo;
    for (j = lo; j < hi; j++) {
        if (cmp(&v[j], &pivot) < 0) {
            tmp = v[i];
            v[i] = v[j];
            v[j] = tmp;
            i++;
        }
    }

    tmp = v[i];
    v[i] = v[hi];
    v[hi] = tmp;
    qsort_rec(v, lo, i - 1, cmp);
    qsort_rec(v, i + 1, hi, cmp);
}

static void
ins_sort(int *v, int n)
{
    int i;
    int j;
    int key;

    for (i = 1; i < n; i++) {
        key = v[i];
        j = i - 1;
        while (j >= 0 && v[j] > key) {
            v[j + 1] = v[j];
            j--;
        }
        v[j + 1] = key;
    }
}

static unsigned int
checksum(const int *v, int n)
{
    unsigned int c = 0;
    int k;

    for (k = 0; k < n; k++)
        c = (unsigned int)((c + (unsigned int)v[k] * (unsigned int)(k + 1)) & 0xffffu);

    return c;
}

static int
is_sorted(const int *v, int n)
{
    int k;

    for (k = 1; k < n; k++) {
        if (v[k - 1] > v[k])
            return 0;
    }
    return 1;
}

static int
qsort_run(void)
{
    unsigned int chk = 0;
    int ok = 1;
    int r;
    int i;

    for (r = 0; r < QREPS; r++) {
        lcg_state = (unsigned int)(0xBEEFu + (unsigned int)r);
        for (i = 0; i < QN; i++)
            qa[i] = next_val();
        qsort_rec(qa, 0, QN - 1, cmp_int);
        ok &= is_sorted(qa, QN);
        chk = (unsigned int)((chk + checksum(qa, QN)) & 0xffffu);
    }

    if (ok != 1)
        return 1;
    if (chk != QSUM)
        return 2;
    return 0;
}

static int
ins_run(void)
{
    unsigned int chk = 0;
    int ok = 1;
    int r;
    int i;

    for (r = 0; r < IREPS; r++) {
        lcg_state = (unsigned int)(0x1234u + (unsigned int)r);
        for (i = 0; i < IN; i++)
            ia[i] = next_val();
        ins_sort(ia, IN);
        ok &= is_sorted(ia, IN);
        chk = (unsigned int)((chk + checksum(ia, IN)) & 0xffffu);
    }

    if (ok != 1)
        return 3;
    if (chk != ISUM)
        return 4;
    return 0;
}

int
main(void)
{
    int rc;

    rc = qsort_run();
    if (rc != 0)
        return rc;
    return ins_run();
}
