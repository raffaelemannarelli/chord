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

// REQUEST FUNCTIONS
void notify_request(ChordMessage *to_return,
		    Node *send_to, Node *own_node) {
  ChordMessage msg = CHORD_MESSAGE__INIT;
  NotifyRequest request = NOTIFY_REQUEST__INIT;
  request.node = own_node;
  msg.msg_case = CHORD_MESSAGE__MSG_NOTIFY_REQUEST;
  msg.notify_request = &request;

  send_and_return(to_return, &msg, send_to);
}

void find_successor_request(ChordMessage *to_return,
			    Node *send_to, uint64_t key) {
  ChordMessage msg = CHORD_MESSAGE__INIT;
  FindSuccessorRequest request = FIND_SUCCESSOR_REQUEST__INIT;
  request.key = key;
  msg.msg_case = CHORD_MESSAGE__MSG_FIND_SUCCESSOR_REQUEST;
  msg.find_successor_request = &request;

  send_and_return(to_return, &msg, send_to);
}

void r_find_succ_request(ChordMessage *to_return, Node *send_to,
			 int key, Node *requester) {
  ChordMessage msg = CHORD_MESSAGE__INIT;
  RFindSuccReq request = R_FIND_SUCC_REQ__INIT;
  request.key = key;
  request.requester = requester;
  msg.msg_case = CHORD_MESSAGE__MSG_R_FIND_SUCC_REQ;
  msg.r_find_succ_req = &request;

  send_and_return(to_return, &msg, send_to);  
}

void get_predecessor_request(ChordMessage *to_return,
			     Node *send_to) {
  ChordMessage msg = CHORD_MESSAGE__INIT;
  GetPredecessorRequest request = GET_PREDECESSOR_REQUEST__INIT;
  msg.msg_case = CHORD_MESSAGE__MSG_GET_PREDECESSOR_REQUEST;
  msg.get_predecessor_request = &request;

  send_and_return(to_return, &msg, send_to);
}

void check_predecessor_request(ChordMessage *to_return,
			     Node *send_to) {
  ChordMessage msg = CHORD_MESSAGE__INIT;
  CheckPredecessorRequest request = CHECK_PREDECESSOR_REQUEST__INIT;
  msg.msg_case = CHORD_MESSAGE__MSG_CHECK_PREDECESSOR_REQUEST;
  msg.check_predecessor_request = &request;

  send_and_return(to_return, &msg, send_to);
}

ChordMessage* get_successor_list_request(ChordMessage *to_return,
				Node *send_to) {
  ChordMessage msg = CHORD_MESSAGE__INIT;
  GetSuccessorListRequest request = GET_SUCCESSOR_LIST_REQUEST__INIT;
  msg.msg_case = CHORD_MESSAGE__MSG_GET_SUCCESSOR_LIST_REQUEST;
  msg.get_successor_list_request = &request;

  return send_and_return(to_return, &msg, send_to);
}

// RESPONSE FUNCTIONS
void notify_response(int fd) {
  ChordMessage msg = CHORD_MESSAGE__INIT;
  NotifyResponse response = NOTIFY_RESPONSE__INIT;
  msg.msg_case = CHORD_MESSAGE__MSG_NOTIFY_RESPONSE;
  msg.notify_response = &response;
  pack_and_send(fd, &msg);
}

void find_successor_response(int fd, Node *node) {
  ChordMessage msg = CHORD_MESSAGE__INIT;
  FindSuccessorResponse response = FIND_SUCCESSOR_RESPONSE__INIT;
  response.node = node;
  msg.msg_case = CHORD_MESSAGE__MSG_FIND_SUCCESSOR_RESPONSE;
  msg.find_successor_response = &response;
  pack_and_send(fd, &msg);
}

void rFindSuccResp(int fd, int key, Node *node) {
  ChordMessage msg = CHORD_MESSAGE__INIT;
  RFindSuccResp response = R_FIND_SUCC_RESP__INIT;
  response.key = key;
  response.node = node;
  msg.msg_case = CHORD_MESSAGE__MSG_R_FIND_SUCC_RESP;
  msg.r_find_succ_resp = &response;
  pack_and_send(fd, &msg);  
}

void get_predecessor_response(int fd, Node *node) {
  ChordMessage msg = CHORD_MESSAGE__INIT;
  GetPredecessorResponse response = GET_PREDECESSOR_RESPONSE__INIT;
  response.node = node;
  msg.msg_case = CHORD_MESSAGE__MSG_GET_PREDECESSOR_RESPONSE;
  msg.get_predecessor_response = &response;
  pack_and_send(fd, &msg);
}

void check_predecessor_response(int fd) {
  ChordMessage msg = CHORD_MESSAGE__INIT;
  CheckPredecessorResponse response = CHECK_PREDECESSOR_RESPONSE__INIT;
  msg.msg_case = CHORD_MESSAGE__MSG_CHECK_PREDECESSOR_RESPONSE;
  msg.check_predecessor_response = &response;
  pack_and_send(fd, &msg);
}

// TODO: ensure this is the correct method of handling
void get_successor_list_response(int fd, Node **successors, int num) {
  ChordMessage msg = CHORD_MESSAGE__INIT;
  GetSuccessorListResponse response = GET_SUCCESSOR_LIST_RESPONSE__INIT;
  Node **nodes = malloc(sizeof(Node*)*num);
  fprintf(stderr, "setting\n");
  for (int i = 0; i < num; i++) {
    nodes[i] = malloc(sizeof(Node));
    memcpy(nodes[i], successors[i], sizeof(Node));
  }

  fprintf(stderr, "last prep\n");
  response.n_successors = num;
  response.successors = nodes;
  msg.msg_case = CHORD_MESSAGE__MSG_GET_SUCCESSOR_LIST_RESPONSE;
  msg.get_successor_list_response = &response;
  fprintf(stderr, "packing and sending\n");
  pack_and_send(fd, &msg);
  for (int i = 0; i < num; i++)
    free(nodes[i]);
  free(nodes);
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
}

// sends message, gets response, and copies to to_return
ChordMessage* send_and_return(ChordMessage *to_return,
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
  if (response->msg_case == CHORD_MESSAGE__MSG_GET_SUCCESSOR_LIST_RESPONSE) {
    return response;
  }
  
  memcpy(to_return,response,sizeof(ChordMessage));
  chord_message__free_unpacked(response, NULL);
  return NULL;
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
