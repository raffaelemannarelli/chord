#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <poll.h>
#include <time.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include "hash.h"
#include "helper.h"
#include "message.h"

// hopefully this is large enough
#define BUFFER_SIZE 256

// WARNING: MAY NOT BE PROPERLY COPYING RESPONSE TO INCLUDE
//          POINTER FIELD: CHECK THIS!!!!


// sends notify_request
void notify_request(ChordMessage *to_return,
		    Node *send_to, Node *own_node) {
  ChordMessage msg = CHORD_MESSAGE__INIT;
  NotifyRequest request = NOTIFY_REQUEST__INIT;
  request.node = own_node;
  msg.msg_case = CHORD_MESSAGE__MSG_NOTIFY_REQUEST;
  msg.notify_request = &request;

  send_and_return(to_return, &msg, send_to);
}

// sends find_successor_request
void find_successor_request(ChordMessage *to_return,
			    Node *send_to, Node *own_node) {
  ChordMessage msg = CHORD_MESSAGE__INIT;
  FindSuccessorRequest request = FIND_SUCCESSOR_REQUEST__INIT;
  request.key = own_node->key;
  msg.msg_case = CHORD_MESSAGE__MSG_FIND_SUCCESSOR_REQUEST;
  msg.find_successor_request = &request;
  
  send_and_return(to_return, &msg, send_to);
}

/*void r_find_succ_request(ChordMessage *to_return, Node *send_to,
			 int key, Node *requester) {
  ChordMessage msg = CHORD_MESSAGE__INIT;
  
  }*/

// sends get_predecessor_request
void get_predecessor_request(ChordMessage *to_return,
			     Node *send_to) {
  ChordMessage msg = CHORD_MESSAGE__INIT;
  GetPredecessorRequest request = GET_PREDECESSOR_REQUEST__INIT;
  msg.msg_case = CHORD_MESSAGE__MSG_GET_PREDECESSOR_REQUEST;
  msg.get_predecessor_request = &request;

  send_and_return(to_return, &msg, send_to);
}

void notify_response(int fd) {
  ChordMessage msg = CHORD_MESSAGE__INIT;
  NotifyResponse response = NOTIFY_RESPONSE__INIT;
  msg.msg_case = CHORD_MESSAGE__MSG_NOTIFY_RESPONSE;
  msg.notify_response = &response;
  pack_and_send(fd, &msg);
}

// sends find_sucessor_response
void find_successor_response(int fd, Node *node) {
  ChordMessage msg = CHORD_MESSAGE__INIT;
  FindSuccessorResponse response = FIND_SUCCESSOR_RESPONSE__INIT;
  response.node = node;
  msg.msg_case = CHORD_MESSAGE__MSG_FIND_SUCCESSOR_RESPONSE;
  msg.find_successor_response = &response;
  pack_and_send(fd, &msg);
}

// sends get_predecessor_response
void get_predecessor_response(int fd, Node *node) {
  ChordMessage msg = CHORD_MESSAGE__INIT;
  GetPredecessorResponse response = GET_PREDECESSOR_RESPONSE__INIT;
  response.node = node;
  msg.msg_case = CHORD_MESSAGE__MSG_GET_PREDECESSOR_RESPONSE;
  msg.get_predecessor_response = &response;
  pack_and_send(fd, &msg);
}

/*******************************************/
/*            HELPER FUNCTIONS             */
/*******************************************/

// pack and send message
void pack_and_send(int fd, ChordMessage *msg) {
  uint8_t buf[BUFFER_SIZE];
  int msg_len = chord_message__get_packed_size(msg);
  chord_message__pack(msg, buf);
  int s = send(fd, buf, msg_len, 0);
  assert(s >= 0);
  close(fd);
}

// sends message, gets response, and copies to to_return
void send_and_return(ChordMessage *to_return,
			      ChordMessage *msg, Node *to_send) {
  uint8_t buf[BUFFER_SIZE];
  int send_len = chord_message__get_packed_size(msg);
  chord_message__pack(msg, buf);

  int fd = socket_and_connect(to_send);
  send(fd, buf, send_len, 0);
  int recv_len = recv(fd, buf, BUFFER_SIZE, 0);
  close(fd);

  ChordMessage *response = chord_message__unpack(NULL, recv_len,
						 buf);
  memcpy(to_return,response,sizeof(ChordMessage));
  chord_message__free_unpacked(response, NULL);
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

// TODO: FIGURE OUT IF query_id in msg IS IMPORTANT
void node_init(Node *node, struct sockaddr_in *addr) {
  node__init(node);
  node->key = hash_addr(addr);
  node->address = addr->sin_addr.s_addr;
  node->port = addr->sin_port;
}
