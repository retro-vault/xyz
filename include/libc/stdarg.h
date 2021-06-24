/*
 * stdarg.h
 *
 * standard C header file
 * 
 * MIT License (see: LICENSE)
 * copyright (c) 2021 tomaz stih
 *
 * 03.05.2021   tstih
 *
 */
#ifndef __STDARG_H__
#define __STDARG_H__

/* Standard C var arg macros */
#define va_list                 unsigned char *
#define va_start(marker, last)  { marker = (va_list)&last + sizeof(last); }
#define va_arg(marker, type)    *((type *)((marker += sizeof(type)) - sizeof(type)))
#define va_end(marker)          marker = (va_list) 0;

#endif /* __STDARG_H__ */
