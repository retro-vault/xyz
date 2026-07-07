# z88dk Benchmark Kernels

This suite mirrors the compiler-comparison kernels discussed on the z88dk
forum:

- `search`
- `sieve`
- `sort`
- `queen`
- `rle`
- `switch`
- `intbench`
- `ptr`
- `crc`
- `char`

The upstream sources use the z88dk `test.h` framework and `printf`-based
reporting. These copies keep the benchmark kernels and host-verified expected
values, but use a libc-free `main()` that returns zero on success. The benchmark
runner links only the test startup and runtime helper archive, not libc.
