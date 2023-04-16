#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <poll.h>
#include <time.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include "chord_arg_parser.h"
#include "chord.h"
#include "hash.h"
#include "helper.h"

// TODO: FIGURE OUT IF query_id in msg IS IMPORTANT
void node_init(Node *node, struct sockaddr_in *addr) {
  node__init(node);
  node->key = hash_addr(addr);
  node->address = addr->sin_addr.s_addr;
  node->port = addr->sin_port;
}

// returns response to a find_successor_request call
void find_successor_request(ChordMessage *to_return,
			    int fd, Node *own_node) {
  ChordMessage msg = CHORD_MESSAGE__INIT;
  FindSuccessorRequest request = FIND_SUCCESSOR_REQUEST__INIT;
  request.key = own_node->key;
  msg.msg_case = CHORD_MESSAGE__MSG_FIND_SUCCESSOR_REQUEST;
  msg.find_successor_request = &request;

  int len = chord_message__get_packed_size(&msg);
  char buf[BUFFER_SIZE];

  chord_message__pack(&msg,buf);

  send(sock, buf, len, 0);
  int msg_len = recv(sock, buf, BUFFER_SIZE, 0);
  ChordMessage *response = chord_message__unpack(NULL, msg_len, buf);
  memcpy(to_return,response,sizeof(ChordMessage));
  chord_message__free_unpacked(response, NULL);
}

// sends sucessor request
void send_successor_request(int fd, Node *node) {
  ChordMessage msg = CHORD_MESSAGE_INIT;
  FindSuccessorResponse response = FIND_SUCCESSOR_RESPONSE__INIT;
  response.node = node;
  msg.msg_case = CHORD_MESSAGE__MSG_FIND_SUCCESSOR_RESPONSE;
  msg.find_successor_response = &response;
  pack_and_send(fd, &msg);
}

// pack and send message
void pack_and_send(int fd, ChordMessage *msg) {
  char buf[BUFFER_SIZE];
  int msg_len = chord_message__get_packed_size(&msg);
  chord_message__pack(&msg,buf);
  int s = send(fd, buf, msg_len, 0);
}
