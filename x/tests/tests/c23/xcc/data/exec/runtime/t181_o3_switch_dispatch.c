enum {
    OP_HALT = 0,
    OP_PUSHI,
    OP_LOAD,
    OP_STORE,
    OP_ADD,
    OP_SUB,
    OP_JNZ,
    OP_JMP,
    OP_MUL,
    OP_DUP,
    OP_DROP,
    OP_LT
};

#define K      200
#define REPS   18
#define RESULT 20100u
#define CHK    ((RESULT * REPS) & 0xffffu)

static const int prog[] = {
    OP_PUSHI, K,
    OP_STORE, 0,
    OP_PUSHI, 0,
    OP_STORE, 1,
    OP_LOAD,  1,
    OP_LOAD,  0,
    OP_ADD,
    OP_STORE, 1,
    OP_LOAD,  0,
    OP_PUSHI, 1,
    OP_SUB,
    OP_STORE, 0,
    OP_LOAD,  0,
    OP_JNZ,   8,
    OP_HALT
};

static int stk[32];
static int mem[4];

static int
vm_run(void)
{
    int pc = 0;
    int sp = -1;
    int op;
    int a;

    for (;;) {
        op = prog[pc++];
        switch (op) {
        case OP_PUSHI:
            stk[++sp] = prog[pc++];
            break;
        case OP_LOAD:
            stk[++sp] = mem[prog[pc++]];
            break;
        case OP_STORE:
            mem[prog[pc++]] = stk[sp--];
            break;
        case OP_ADD:
            stk[sp - 1] += stk[sp];
            sp--;
            break;
        case OP_SUB:
            stk[sp - 1] -= stk[sp];
            sp--;
            break;
        case OP_MUL:
            stk[sp - 1] *= stk[sp];
            sp--;
            break;
        case OP_LT:
            a = (stk[sp - 1] < stk[sp]);
            sp--;
            stk[sp] = a;
            break;
        case OP_DUP:
            stk[sp + 1] = stk[sp];
            sp++;
            break;
        case OP_DROP:
            sp--;
            break;
        case OP_JNZ:
            a = prog[pc++];
            if (stk[sp--])
                pc = a;
            break;
        case OP_JMP:
            pc = prog[pc];
            break;
        case OP_HALT:
            return mem[1];
        default:
            return -1;
        }
    }
}

int
main(void)
{
    unsigned int chk = 0;
    int r;
    int res = 0;

    for (r = 0; r < REPS; r++) {
        res = vm_run();
        chk = (unsigned int)((chk + (unsigned int)res) & 0xffffu);
    }

    if (res != (int)RESULT)
        return 1;
    if (chk != CHK)
        return 2;
    return 0;
}
