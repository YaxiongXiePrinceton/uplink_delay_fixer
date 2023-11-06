#ifndef DCI_SINK_CLIENT_H
#define DCI_SINK_CLIENT_H

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

void *dci_sink_client_thread(void *p);

#ifdef __cplusplus
}
#endif

#endif
