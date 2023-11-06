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


#include "load_config.h"
#include "time_stamp.h"

extern bool go_exit;
extern ngscope_dci_sink_CA_t dci_CA_buf;

// connect server
void *dci_sink_client_thread(void *p) {
  /*******  DCI server configuration ********/
  char dci_serv_IP[40] = "127.0.0.1";
  int dci_serv_port = 6767;
  char recvBuf[1500];
  char sendBuf[1500];
  struct sockaddr_in cliaddr;
  // connect to the DCI server
  int sockfd = sock_connectServer_w_config_udp(serv_IP, serv_port);
  /*******  END of DCI server configuration ********/
  
  serv_cli_config_t* config = (serv_cli_config_t *)p;
  int sock_fd;
  struct sockaddr_in remote_addr;
  remote_addr = sock_create_serv_addr(config.remote_IP, config.remote_port);
  // basically notify the remote about us 
  // very important if the server want to send something back to us
  connection_starter(sock_fd, remote_addr);

  int last_ca_header;

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
      // if the CA header is updated
      int new_header = ngscope_dciSink_ringBuf_header_update(&dci_CA_buf, last_ca_header);
      if(new_header){
        // example: send dci to the remote 
        // TODO Yuxin: change it to transmit ur trans-buffer or anything
        ue_dci_t ue_dci = ngscope_dciSink_ringBuf_fetch_dci(&dci_CA_buf, 0, new_header);
        pkt_header_t pkt_header;
        pkt_header.sequence_number = 0;
        pkt_header.sent_timestamp = timestamp_us();
        pkt_header.recv_timestamp = 0;
        pkt_header.pkt_type    = 3; // 1 DATA 2 ACK 3 DCI
        int pkt_size = packet_generate(sendBuf, &pkt_header, (void *)&ue_dci, sizeof(ue_dci_t));

        // send the dci packet to the remote
        sock_pkt_send_single(sock_fd, remote_addr, sendBuf, pkt_size);
      }
    }
  }
  // close the connection with server
  sock_close_and_notify_udp(sockfd);

  printf("hell world\n");
  return NULL;
}
