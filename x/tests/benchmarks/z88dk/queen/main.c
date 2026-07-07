#define NQ   8
#define REPS 1
#define NSOL 92

static int board[NQ];
static unsigned int solutions;

static int
safe(int col, int row)
{
    int i;
    int d;

    for (i = 0; i < col; i++) {
        int ri = board[i];

        if (ri == row)
            return 0;
        d = ri - row;
        if (d < 0)
            d = -d;
        if (d == col - i)
            return 0;
    }

    return 1;
}

static void
place(int col)
{
    int row;

    if (col == NQ) {
        solutions++;
        return;
    }

    for (row = 0; row < NQ; row++) {
        if (safe(col, row)) {
            board[col] = row;
            place(col + 1);
        }
    }
}

int
main(void)
{
    unsigned int total = 0;
    int r;

    for (r = 0; r < REPS; r++) {
        solutions = 0;
        place(0);
        total += solutions;
    }

    if (solutions != NSOL)
        return 1;
    if (total != (unsigned int)(NSOL * REPS))
        return 2;
    return 0;
}
