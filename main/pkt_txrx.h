#ifndef PKT_TXRX_H
#define PKT_TXRX_H

#include <assert.h>
#include <math.h>
#include <pthread.h>
#include <semaphore.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>

#ifdef __cplusplus
extern "C" {
#endif

int pkt_recv_multi_no_ack(int sock_fd, FILE *fd);

#ifdef __cplusplus
}
#endif

#endif
