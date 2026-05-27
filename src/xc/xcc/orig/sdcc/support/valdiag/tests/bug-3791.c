/* bug-3890.c

   Initalization of extern variable at block scope was treated as assignment.
 */

#ifdef TEST1
#ifdef __SDCC_z80
#define __IO_VDP_DATA_OUT   #0xbe
#define __IO_VDP_COMMAND    #0xbf
static volatile __sfr __at __IO_VDP_COMMAND VDPControlPort; /* ERROR(SDCC_z80) */
static volatile __sfr __at __IO_VDP_DATA_OUT VDPDataPortOut; /* ERROR(SDCC_z80) */
#endif
void f(void);
#endif

