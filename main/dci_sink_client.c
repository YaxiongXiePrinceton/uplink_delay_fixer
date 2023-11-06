#include <arpa/inet.h>
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
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

// #include "srsran/srsran.h"
#include "dci_sink_client.h"
#include "dci_sink_dci_recv.h"
#include "dci_sink_ring_buffer.h"
#include "dci_sink_sock.h"

extern bool go_exit;
extern ngscope_dci_sink_CA_t dci_CA_buf;

// connect server
void *dci_sink_client_thread(void *p) {
  char serv_IP[40] = "127.0.0.1";
  int serv_port = 6767;
  char recvBuf[1400];
  struct sockaddr_in cliaddr;

  // connect the server
  int sockfd = sock_connectServer_w_config_udp(serv_IP, serv_port);
  socklen_t len;
  while (!go_exit) {
    if (go_exit)
      break;
    int buf_idx = 0;
    int recvLen = 0;

    recvLen = recvfrom(sockfd, (char *)recvBuf, 1400, MSG_WAITALL,
                       (struct sockaddr *)&cliaddr, &len);
    if (recvLen > 0) {
      while (!go_exit) {
        int ret = ngscope_dci_sink_recv_buffer(&dci_CA_buf, recvBuf, buf_idx,
                                               recvLen);
        if (ret < 0) {
          // ignore the buffer
          printf("recvLen: %d buf_idx: %d \n\n", recvLen, buf_idx);
          buf_idx = recvLen;
          break;
        } else if (ret == recvLen) {
          break;
        } else {
          buf_idx = ret;
        }
      }
      // offset = shift_recv_buffer(recvBuf, buf_idx, recvLen);
      // printf("recvLen: %d buf_idx: %d \n\n", recvLen, buf_idx);
    }
  }
  // close the connection with server
  sock_close_and_notify_udp(sockfd);

  printf("hell world\n");
  return NULL;
}
