#pragma once

/* https://www.embecosm.com/appnotes/ean4/embecosm-howto-rsp-server-ean4-issue-2.html#id3059472 */

/* Standard package layout is              */
/* +---+----------------+---+------------+ */
/* | $ | <message data> | # | <checksum> | */
/* +---+----------------+---+------------+ */
/* Checksum is 1 byte in hex i.e. F7       */
/* If byte 0x03 is detected, it is Ctrl+C  */
/* escape character is 0x7d (}) follwed by */
/* original char xor 20. Escapes are       */
/* required for $ # and }                  */
/* last packed + = ack, - = fail           */

enum cmd_e {
    read_registers  = 0 /* g ... Reads all registers */
    ,write_registers    /* G ... Writes all registers */
    ,read_memory        /* m ... Reads memory at address */
    ,write_memory       /* M ... yes Writes memory at address */
    ,last_signal        /* ? ... Get last signal: S */
    ,step               /* s ... Step the program */
    ,cont               /* c ... Continues program execution */
};

/*  values for cmd_e i.e. gGmM?sc */
extern char **cmd_sym;

typedef struct msg_s {
    enum cmd_e cmd;

} msg_t;

extern msg_t* msg_from_string(char *s);
char* msg_to_string(msg_t* msg);
extern int msg_checksum(char *s);