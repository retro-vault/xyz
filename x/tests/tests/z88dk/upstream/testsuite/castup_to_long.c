
typedef unsigned char  type8;
typedef signed   char  type8s;
typedef unsigned int   type16;
typedef signed   int   type16s;
typedef unsigned long  type32;
typedef signed   long  type32s;

extern unsigned int __LIB__ intrinsic_swap_endian_16(unsigned int n) [[z88dk::smallc]] [[z88dk::fastcall]];


type32 pc;

type8 *effective(type32 ptr) [[z88dk::fastcall]];

void branch(type8 b) [[z88dk::fastcall]]
{
    if (b == 0) {
        pc +=  (type16s)  (intrinsic_swap_endian_16(*(type16 *)( effective(pc) ))) ;
    } else {
        pc += (type8s) b;
    }
}
