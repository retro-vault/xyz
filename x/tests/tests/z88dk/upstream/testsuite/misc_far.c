
typedef union {
  int a;
  long b;
} un_t;

struct x {
        long c;
};

typedef struct {
  char c;
  un_t pt;
  char arr[10];
  struct x *extra;
  struct x * [[xcc::far]] extraf;
} st_t;

typedef st_t * [[xcc::far]] far_st_ptr;

void callit();

void func(far_st_ptr far_p) {
  callit(far_p);
}

void func0(far_st_ptr far_p) {
  callit(&(far_p->c));
}
void func1(far_st_ptr far_p) {
  callit(&(far_p->pt));
}
void func2(far_st_ptr far_p) {
  callit(&(far_p->pt.b));
}
void func2a(far_st_ptr far_p) {
  callit((far_p->pt.a));
}
void func2b(far_st_ptr far_p) {
  callit((far_p->pt.b));
}

void func3(far_st_ptr far_p) {
  far_p->arr[2] = 10;
}
void func4(far_st_ptr far_p) {
  callit(&far_p->arr[5]);
}


void func5(far_st_ptr far_p) {
  callit(&far_p->extra->c);
}

void func5a(far_st_ptr far_p) {
  callit(far_p->extra->c);
}

void func6(far_st_ptr far_p) {
  callit(&far_p->extraf->c);
}

void func6a(far_st_ptr far_p) {
  callit(far_p->extraf->c);
}
