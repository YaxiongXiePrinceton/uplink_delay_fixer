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

#include "connection.h"
#include "load_config.h"
#include "packet.h"
#include "sock.h"
#include "sock_pkt_txrx.h"
#include "time_stamp.h"
#include "packet.h"
#include "sock.h"

#include "dci_ul_reTx.h"
#include "connection.h"
#include "sock_pkt_txrx.h"
#include "dci_ul_ueSh.h"

extern bool go_exit;
extern ngscope_dci_sink_CA_t dci_CA_buf;
extern ngscope_ul_trans_buf_t dci_reTx_buf;
extern ngscope_ul_trans_buf_t dci_ueSh_buf;

// connect server
void *dci_sink_client_thread(void *p) {
  printf("create client thread\n");
  /*******  DCI server configuration ********/
  char dci_serv_IP[40] = "127.0.0.1";
  // char dci_serv_IP[40] = "0.0.0.0";
  // char dci_serv_IP[40] = "192.168.42.104";
  int dci_serv_port = 6767;
  char recvBuf[1500];
  char sendBuf[1500];
  struct sockaddr_in cliaddr;
  // connect to the DCI server
  int sockfd = sock_connectServer_w_config_udp(dci_serv_IP, dci_serv_port);
  /*******  END of DCI server configuration ********/

  serv_cli_config_t *config = (serv_cli_config_t *)p;
  int sock_fd;
  struct sockaddr_in remote_addr;

  char temp_remote_IP[40] = "3.22.79.149";
  int temp_remote_port = 9002;
  remote_addr = sock_create_serv_addr(temp_remote_IP, temp_remote_port);
  // remote_addr = sock_create_serv_addr(config->remote_IP, config->remote_port);
  // basically notify the remote about us 
  // very important if the server want to send something back to us
  connection_starter(sock_fd, remote_addr);

  int last_ca_header;
  ngscope_trans_init_buf(&dci_reTx_buf);
  ngscope_trans_init_buf(&dci_ueSh_buf);

  FILE* fd_reTx_s       = fopen("./data/send_reTx", "w+");
  FILE* fd_ueSh_s       = fopen("./data/send_ueSh", "w+");

  int socket = 0;
  char local_IP[40] = "192.168.42.104";
  int local_port = 8888;
  socket = sock_create_udp_socket(local_IP, local_port);

  socklen_t len;
  while (!go_exit) {
    if (go_exit)
      break;
    int buf_idx = 0;
    int recvLen = 0;
    // printf("in client loop\n");

    recvLen = recvfrom(sockfd, (char *)recvBuf, 1400, MSG_WAITALL,
                       (struct sockaddr *)&cliaddr, &len);
    if (recvLen > 0) {
      while (!go_exit) {
        int ret = ngscope_dci_sink_recv_buffer(&dci_CA_buf, recvBuf, buf_idx,
                                               recvLen);
        if (ret < 0) {
          // ignore the buffer
          printf("error recvLen: %d buf_idx: %d \n\n", recvLen, buf_idx);
          buf_idx = recvLen;
          break;
        } else if (ret == recvLen) {
          break;
        } else {
          buf_idx = ret;
        }
      }
      // /************ transmit ueSh-buffer **************/
      // ngscope_ul_trans_ueSh_buffer(&dci_CA_buf, &dci_ueSh_buf);

      // if(dci_ueSh_buf.buf_active){
      //   // printf("*******************find ueSh, start %d end %d\n", dci_ueSh_buf.buf_start_tti, dci_ueSh_buf.buf_last_tti);
      //   ngscope_ul_update_ueSh_buffer(&dci_CA_buf, &dci_ueSh_buf);

      //   ngscope_trans_reset_buf(&dci_ueSh_buf);
      // }


      // if the CA header is updated
      int new_header = ngscope_dciSink_ringBuf_header_updated(&dci_CA_buf, last_ca_header);

      if(new_header != -1){
        /************ example: send dci to the remote **************/
        // ue_dci_t ue_dci = ngscope_dciSink_ringBuf_fetch_dci(&dci_CA_buf, 0, new_header);
        // pkt_header_t pkt_header;
        // pkt_header.sequence_number = 0;
        // pkt_header.sent_timestamp = timestamp_us();
        // pkt_header.recv_timestamp = 0;
        // pkt_header.pkt_type    = 3; // 1 DATA 2 ACK 3 DCI
        // int pkt_size = packet_generate(sendBuf, &pkt_header, (void *)&ue_dci, sizeof(ue_dci_t));

        // // send the dci packet to the remote
        // sock_pkt_send_single(sock_fd, remote_addr, sendBuf, pkt_size);

        /************ transmit ueSh-buffer **************/
        ngscope_ul_trans_ueSh_buffer(&dci_CA_buf, new_header, &dci_ueSh_buf);

        /************ transmit reTx-buffer **************/
        ngscope_ul_trans_reTx_buffer(&dci_CA_buf, new_header, &dci_reTx_buf);

        if (dci_reTx_buf.buf_active) {
          pkt_header_t pkt_header;
          pkt_header.sequence_number = 0;
          pkt_header.sent_timestamp = timestamp_us();
          pkt_header.recv_timestamp = 0;
          pkt_header.pkt_type    = 4; // 1 DATA 2 ACK 3 DCI 4 trans_buf
          int pkt_size = packet_generate(sendBuf, &pkt_header, (void *)&dci_reTx_buf, sizeof(ngscope_ul_trans_buf_t));
          // printf("\nsize %d\n", pkt_size);

          sock_pkt_send_single(socket, remote_addr, sendBuf, pkt_size);
        }

        if (dci_ueSh_buf.buf_active) {
          pkt_header_t pkt_header;
          pkt_header.sequence_number = 0;
          pkt_header.sent_timestamp = timestamp_us();
          pkt_header.recv_timestamp = 0;
          pkt_header.pkt_type    = 5; // 1 DATA 2 ACK 3 DCI 4 reTx_buf 5 ueSh_buf
          int pkt_size = packet_generate(sendBuf, &pkt_header, (void *)&dci_reTx_buf, sizeof(ngscope_ul_trans_buf_t));

          sock_pkt_send_single(socket, remote_addr, sendBuf, pkt_size);
        }

        /************ update last_ca_header **************/
        last_ca_header = new_header;
      }
    }
  }
  // close the connection with server
  sock_close_and_notify_udp(sockfd);
  fclose(fd_reTx_s);
  fclose(fd_ueSh_s);

  printf("hell world\n");
  return NULL;
}
