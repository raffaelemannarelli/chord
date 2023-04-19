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

// WARNING: MAY NOT BE PROPERLY COPYING RESPONSE TO INCLUDE
//          POINTER FIELD: CHECK THIS!!!!

void get_predecessor_request(ChordMessage *to_return,
			     Node *send_to, Node *own_node) {
  ChordMessage msg = CHORD_MESSAGE__INIT;
  GetPredecessorRequest request = GET_PREDECESSOR_REQUEST__INIT;
  msg.msg_case = CHORD_MESSAGE__MSG_GET_PREDECESSOR_REQUEST;
  msg.get_predecessor_request = &request;
  
  int send_len = chord_message__get_packet_size(&msg);
  char buf[BUFFER];
  chord_message__pack(&msg, buf);

  int fd = socket_and_connect(send_to);
  send(fd, buf, send_len, 0);
  int recv_len = recv(fd, buf, BUFFER_SIZE, 0);
  close(fd);

  ChordMessage *response = chord_message__unpack(NULL, recv_len, buf);
  // WARNING IS SIZEOF CHORDMESSAGE CONSISTENT???
  memcpy(to_return, response, sizeof(ChordMessage));
  chord_message__free_unpacked(response, NULL);
}

// returns response to a find_successor_request call
void find_successor_request(ChordMessage *to_return,
			    Node *send_to, Node *own_node) {
  ChordMessage msg = CHORD_MESSAGE__INIT;
  FindSuccessorRequest request = FIND_SUCCESSOR_REQUEST__INIT;
  request.key = own_node->key;
  msg.msg_case = CHORD_MESSAGE__MSG_FIND_SUCCESSOR_REQUEST;
  msg.find_successor_request = &request;

  int send_len = chord_message__get_packed_size(&msg);
  char buf[BUFFER_SIZE];
  chord_message__pack(&msg,buf);

  int fd = socket_and_connect(send_to);
  send(fd, buf, send_len, 0);
  int recv_len = recv(sock, buf, BUFFER_SIZE, 0);
  close(fd);
  
  ChordMessage *response = chord_message__unpack(NULL, recv_len, buf);
    // WARNING IS SIZEOF CHORDMESSAGE CONSISTENT???
  memcpy(to_return,response,sizeof(ChordMessage));
  chord_message__free_unpacked(response, NULL);
}

void get_predecessor_response(Node *to_send, Node *node) {
  ChordMessage msg = CHORD_MESSAGE_INIT;
  FindSuccessorResponse response = GET_PREDECESSOR_RESPONSE__INIT;
  response.node = node;
  msg.msg_case = CHORD_MESSAGE__MSG_GET_PREDECESSOR_RESPONSE;
  msg.get_predecessor_response = &response;
  pack_and_send(to_send, &msg);
}

// sends sucessor request
void find_successor_response(Node *to_send, Node *node) {
  ChordMessage msg = CHORD_MESSAGE_INIT;
  FindSuccessorResponse response = FIND_SUCCESSOR_RESPONSE__INIT;
  response.node = node;
  msg.msg_case = CHORD_MESSAGE__MSG_FIND_SUCCESSOR_RESPONSE;
  msg.find_successor_response = &response;
  pack_and_send(to_send, &msg);
}

// pack and send message
void pack_and_send(Node *to_send, ChordMessage *msg) {
  char buf[BUFFER_SIZE];
  int msg_len = chord_message__get_packed_size(&msg);
  chord_message__pack(&msg,buf);
  int fd = socket_and_connect(to_send);
  int s = send(fd, buf, msg_len, 0);
  close(fd);
}

// makes socket and connection from given node
int socket_and_connect(Node *node) {
  int fd = socket(AF_INET, SOCK_STREAM, 0);
  assert(fd >= 0);
  struct sockaddr_in addr;
  addr_from_node(&addr,node);
  int c = connect(fd, (struct sockaddr*)&(addr),
		  sizeof(struct sockaddr));
  assert(c >= 0);
  return fd;
}
