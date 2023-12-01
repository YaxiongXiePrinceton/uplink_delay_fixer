#include <arpa/inet.h>
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <math.h>
#include <pthread.h>
#include <semaphore.h>
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

#include "connection.h"
#include "dci_sink_client.h"
#include "load_config.h"
#include "nat_punch.h"
#include "packet.h"
#include "sock.h"
#include "sock_cmd.h"
#include "sock_pkt_txrx.h"

#include "dci_sink_ring_buffer.h"
#include "dci_ul_reTx.h"

bool go_exit;
ngscope_dci_sink_CA_t dci_CA_buf;
ngscope_ul_trans_buf_t dci_reTx_buf;
ngscope_ul_trans_buf_t dci_ueSh_buf;

int main(int argc, char **argv) {
  int c;
  serv_cli_config_t config;
  char *config_path = NULL;

  /* Parsing command line arguments */
  while ((c = getopt(argc, argv, "c:s:o:h")) != -1) {
    switch (c) {
    case 'c':
      config_path = optarg;
      break;
    case 'h':
      return 0;
    case '?':
      if (optopt == 'c')
        fprintf(stderr, "Option -%c requires an argument.\n", optopt);
      else {
        return 1;
      }
    default:
      return 1;
    }
  }

  FILE *fd = fopen("pkt_log", "w+");

  packetIO_read_config(&config, config_path);

  bool separate_txrx = config.separate_txrx;
  bool pkt_send = config.pkt_sent_config.send_pkt;
  bool send_port = config.pkt_sent_config.send_port;
  bool pkt_recv = config.pkt_recv_config.recv_pkt;
  bool recv_port = config.pkt_recv_config.recv_port;

  pkt_tx_config_t pkt_tx_config;
  pkt_tx_config.pkt_size = config.pkt_sent_config.pkt_size;
  pkt_tx_config.pkt_num = config.pkt_sent_config.pkt_send_num;
  pkt_tx_config.pkt_interval = config.pkt_sent_config.pkt_send_int_us;

  printf("SEND %d PACKETS!\n", pkt_tx_config.pkt_size);

  // we only care about the uplink now
  pthread_t dci_thd;
  // pthread_create(&dci_thd, NULL, dci_sink_client_thread,
  //                  (serv_cli_config_t *)config);
  // pthread_create(&dci_thd, NULL, dci_sink_client_thread,
  //                  (void *)config.remote_IP);

  if (config.sender) {
    // pthread_t dci_thd;
    pthread_create(&dci_thd, NULL, dci_sink_client_thread,
                   (void *)config.remote_IP);
    // pthread_join(dci_thd, NULL);
    // sleep(5);
  }

  int sock_fd = 0;
  sock_fd = sock_create_udp_socket(config.local_IP, config.local_port);

  struct sockaddr_in remote_addr;
  // now we need to figure out the remote addr
  if (config.connect_starter) {
    remote_addr = sock_create_serv_addr(config.remote_IP, config.remote_port);
    connection_starter(sock_fd, remote_addr);
  } else {
    remote_addr = connection_responder(sock_fd);
  }
  if (config.sender) {
    // let's send the packets
    sock_pkt_send_multi_w_config(sock_fd, remote_addr, pkt_tx_config, fd);

    // after sending the packets, let's close the connection.
    sock_cmd_sent_w_type(sock_fd, remote_addr, CON_CLOSE);
  } else {
    // receiver
    // sock_pkt_recv_multi_no_ack(sock_fd, fd);
    sock_pkt_recv_para_no_ack(sock_fd, fd);
  }

  if (config.sender) {
    pthread_join(dci_thd, NULL);
  }

  fclose(fd);
  return 0;
}
