

typedef void (*fcallee)(int x) [[z88dk::callee]];
typedef void (*fcall)(int x) [[z88dk::fastcall]];

typedef void (*call)(int x);

void func_fcall(int) [[z88dk::fastcall]];
void func(int);
void func_callee(int) [[z88dk::callee]];


int test1() {
   fcall fc_ptr;
   fc_ptr = func_fcall;

   fc_ptr(1);
}

int test2() {
   call c_ptr;
   c_ptr = func;

   c_ptr(1);
}

int test3() {
   fcallee c_ptr;
   c_ptr = func_callee;

   c_ptr(1);
}
