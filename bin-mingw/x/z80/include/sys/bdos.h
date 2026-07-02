/*
 * sys/bdos.h
 *
 * CP/M BDOS call declarations for target programs.
 *
 * MIT License (see: LICENSE)
 * Copyright (C) 2026 tomaz stih
 */
#ifndef XCC_SYS_BDOS_H
#define XCC_SYS_BDOS_H

#define BDOS_SUCCESS 0
#define BDOS_FAILURE 0xff

#define P_TERMCPM     0
#define P_CODE        108

#define C_READ        1
#define C_WRITE       2
#define C_RAWIO       6
#define C_DELIMIT     110
#define C_WRITEBLK    111

#define DRV_SET       14
#define DRV_LOGINVEC  24
#define DRV_GET       25
#define DRV_DPB       31

#define T_SET         104
#define T_GET         105

#define F_OPEN        15
#define F_CLOSE       16
#define F_SEARCHFIRST 17
#define F_SEARCHNEXT  18
#define F_DELETE      19
#define F_READ        20
#define F_WRITE       21
#define F_MAKE        22
#define F_DMAOFF      26
#define F_USERNUM     32
#define F_READRAND    33
#define F_WRITERAND   34
#define F_SIZE        35
#define F_RANDREC     36
#define F_TRUNCATE    99
#define F_PARSE       152

typedef struct bdos_ret_s {
    unsigned char reta;
    unsigned char retb;
    unsigned short rethl;
} bdos_ret_t;

#if defined(__SDCC_VERSION_MAJOR) && defined(__SDCC_VERSION_MINOR) && \
    (__SDCC_VERSION_MAJOR == 4 && __SDCC_VERSION_MINOR == 0)
#define __sdcccall(a)
#endif

#ifndef __sdcccall
#define __sdcccall(a)
#endif

extern unsigned char bdos(unsigned char fn, unsigned short param) __sdcccall(1);
extern void bdosret(unsigned char fn, unsigned short param, bdos_ret_t *p) __sdcccall(1);

#endif /* XCC_SYS_BDOS_H */
