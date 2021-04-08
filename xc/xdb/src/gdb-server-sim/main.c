/* https://jameshfisher.com/2017/04/05/set_socket_nonblocking/ */
/* https://github.com/mafintosh/echo-servers.c/blob/master/tcp-non-blocking-echo-server.c */
#include <stdio.h>

#include "server.h"

int main() {
    server_run();
    return 0;
}