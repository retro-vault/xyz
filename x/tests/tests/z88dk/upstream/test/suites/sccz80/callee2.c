
#include "test.h"
#include <stdio.h>
#include <stdlib.h>


[[z88dk::callee]] int int_callee(int a,int b) {
    assertEqual(3,a);
    assertEqual(5,b);
    return 22;
}

void test_int_callee() {
   int ret;
   ret = int_callee(3,5);
   assertEqual(22,ret);
}

void test_int_callee_ptr() {
   [[z88dk::callee]] int (*func)(int x, int y) = int_callee;

   int ret = (func)(3,5);
   assertEqual(22,ret);
}

[[z88dk::callee]] void long_callee(long a,long b) {
    assertEqual(3,a);
    assertEqual(5,b);
}

void test_long_callee() {
   long_callee(3,5);
}

void test_long_callee_ptr() {
   [[z88dk::callee]] void (*func)(long x, long y) = long_callee;

   (func)(3,5);
}

#ifndef __8080
  #ifndef __GBZ80
[[z88dk::callee]] long long int_callee_ret_longlong(int a,int b) {
    assertEqual(3,a);
    assertEqual(5,b);
    return a;
}

void test_int_callee_ret_longlong() {
   long long ret = int_callee_ret_longlong(3,5);
   assertEqual(3, ret);
}

void test_int_callee_ret_longlong_ptr() {
   [[z88dk::callee]] long long (*func)(int x, int y) = int_callee_ret_longlong;
   long long ret = func(3,5);
   assertEqual(3, ret);
}

[[z88dk::callee]] long long longlong_callee_ret_longlong(long long a,long long b) {
    assertEqual(3,a);
    assertEqual(5,b);
    return a;
}

void test_longlong_callee_ret_longlong() {
   long long ret = longlong_callee_ret_longlong(3,5);
   assertEqual(3, ret);
}

void test_longlong_callee_ret_longlong_ptr() {
   [[z88dk::callee]] long long (*func)(long long x, long long y) = longlong_callee_ret_longlong;
   long long ret = func(3,5);
   assertEqual(3, ret);
}
  #endif
#endif


int suite_callee()
{
    suite_setup("Callee Function Pointer Tests");
    suite_add_test(test_int_callee);
    suite_add_test(test_int_callee_ptr);
    suite_add_test(test_long_callee);
    suite_add_test(test_long_callee_ptr);
#ifndef __8080
  #ifndef __GBZ80
    suite_add_test(test_int_callee_ret_longlong);
    suite_add_test(test_int_callee_ret_longlong_ptr);
    suite_add_test(test_longlong_callee_ret_longlong);
    suite_add_test(test_longlong_callee_ret_longlong_ptr);
  #endif
#endif

    return suite_run();
}


int main(int argc, char *argv[])
{
    int  res = 0;

    res += suite_callee();

    exit(res);
}
