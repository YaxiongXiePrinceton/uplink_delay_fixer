#include <arpa/inet.h>
#include <assert.h>
#include <errno.h>
#include <poll.h>
#include <signal.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>

#include "log.h"
#include "packet.h"
#include "sock_cmd.h"
#include "sock_pkt_txrx.h"
#include "time_stamp.h"

#include "dci_sink_ring_buffer.h"

int pkt_recv_multi_no_ack(int sock_fd, FILE *fd) {
  char recvBuf[1500];
  struct sockaddr_in remote_addr;
  /* wait for incoming packet OR expiry of timer */
  struct pollfd poll_fds[1];
  poll_fds[0].fd = sock_fd;
  poll_fds[0].events = POLLIN;

  struct timespec timeout;
  timeout.tv_sec = 1;
  timeout.tv_nsec = 0;

  sock_cmd_type_t pkt_type;
  int recvLen = 0;
  while (true) {
    fflush(NULL);
    // printf("WHILE LOOPING! pkt_rcv:%d nof_pkt:%d\n", _pkt_received,
    // _nof_pkt);
    ppoll(poll_fds, 1, &timeout, NULL);
    if (poll_fds[0].revents & POLLIN) {
      recvLen = sock_pkt_recv_single(sock_fd, &remote_addr, recvBuf);
      if (recvLen > 0) {
        pkt_type = sock_cmd_identify_pkt_type(recvBuf);
        if (pkt_type == DATA) {
          pkt_header_t pkt_header;
          packet_extract_header(recvBuf, recvLen, &pkt_header);
          pkt_header.recv_timestamp = timestamp_us();
          if (pkt_header.pkt_type == 1) {
            printf("DATA packets!\n");
          } else if (pkt_header.pkt_type == 3) {
            ue_dci_t ue_dci;
            packet_decompose(recvBuf, recvLen, &pkt_header, &ue_dci);
            printf("UE-DCI : TTI:%d RNTI:%d\n", ue_dci.tti, ue_dci.rnti);
          }
          log_pkt_header(pkt_header, fd);
        } else if (pkt_type == CON_CLOSE) {
          /* we receive the command to close the connection */
          printf("CONNECTION CLOSE received, close the connection!\n");
          printf(" Good Bye!\n");
          break;
        } else {
          continue;
        }
      }
    }
  }
}
