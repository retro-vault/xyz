// sudo apt-get install libreadline-dev 
extern "C" {
#include <stdio.h>
#include <readline/readline.h>
#include <readline/history.h>
}
#include <iostream>

int main(int argc, char **argv) {

    //std::cout << std::endl;
    //std::cout << "Z80 GDB 0.1 (c) 2020 Tomaz Stih (MIT license)" << std::endl;
    //std::cout << "ready?" << std::endl;

    //std::cout << argc;
    //for(int i=0;i<argc;i++)
        //std::cout << argv[i] << std::endl;

    char *cmd_raw;
    bool quit=false;

    while (!quit) {
        cmd_raw=readline("(gdb) ");
        std::string cmd=cmd_raw;
        if(cmd=="list") {
            std::cout << "1\t#include <iostream>" << std::endl;
            std::cout << "2" << std::endl; 
            std::cout << "3\tint main(int argc, char**argv)" << std::endl;
            std::cout << "4\t\treturn 0;" << std::endl;
            std::cout << "5\t}" << std::endl; 
        } else if (cmd=="info registers") {
            std::cout << "de  0x00ff     255" << std::endl;
            std::cout << "hl  0x1000    4096" << std::endl;
            std::cout << "a   0x01         1" << std::endl;
        } else if (cmd=="info line") {
            std::cout << "Line 3 of \"/home/tomaz/Work/z80-gdb/z80-gdb.cpp\" is at address 0x12cc <main(int, char**)+35> but contains no code.\n" << std::endl;
            std::cout << "\032\032/home/tomaz/Work/z80-gdb/z80-gdb.cpp:10:191:beg:0x12cc\n" << std::endl; 
        } else if (cmd=="disassemble 0x12cc,0x13cc") {
            std::cout << "Dump of assembler code from 0x12cc to 0x13cc:\n" << std::endl; 
            std::cout << "   0x00000000000012cc <main(int, char**)+35>:\tmovb   $0x0,-0x49(%rbp)\n" << std::endl; 
            std::cout << "   0x00000000000012d0 <main(int, char**)+39>:\tcmpb   $0x0,-0x49(%rbp)\n" << std::endl; 
            std::cout << "   0x00000000000012d4 <main(int, char**)+43>:\tjne    0x14be <main(int, char**)+533>\n" << std::endl; 
            std::cout << "   0x00000000000012da <main(int, char**)+49>:\tlea    0xd28(%rip),%rdi        # 0x2009\n" << std::endl; 
            std::cout << "   0x00000000000012e1 <main(int, char**)+56>:\tcallq  0x1130 <readline@plt>\n" << std::endl; 
            std::cout << "   0x00000000000012e6 <main(int, char**)+61>:\tmov    %rax,-0x48(%rbp)\n" << std::endl; 
            std::cout << "   0x00000000000012ea <main(int, char**)+65>:\tlea    -0x4a(%rbp),%rax\n" << std::endl; 
        } else if (cmd=="info files") {
            std::cout << "Symbols from \"/home/tomaz/Work/z80-gdb/z80-gdb\".\n" << std::endl;
            std::cout << "Local exec file:\n" << std::endl;
            std::cout << "\t`/home/tomaz/Work/z80-gdb/z80-gdb\', file type elf64-x86-64.\n" << std::endl;
            std::cout << "\tEntry point: 0x11c0\n" << std::endl;
            std::cout << "\t0x0000000000003d28 - 0x0000000000003d30 is .fini_array\n"<< std::endl;
            std::cout << "\t0x0000000000003d30 - 0x0000000000003f50 is .dynamic\n"<< std::endl;
            std::cout << "\t0x0000000000003f50 - 0x0000000000004000 is .got\n"<< std::endl;
            std::cout << "\t0x0000000000004000 - 0x0000000000004018 is .data\n"<< std::endl;
            std::cout << "\t0x0000000000004040 - 0x0000000000004158 is .bss\n"<< std::endl;
        } 
        else if (cmd=="quit") 
            quit=true;
    }

    return 0;
}