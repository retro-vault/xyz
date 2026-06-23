float sqrtf(float x);
float fabsf(float x);
int __libc_isfinitef(float x);

float root_abs(float x) {
    return sqrtf(fabsf(x));
}

int finite(float x) {
    return __libc_isfinitef(x);
}
