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
#include "chord_arg_parser.h"
#include "chord.h"

// REQUESTS
void notify_request(ChordMessage *to_return,
                    Node *send_to, Node *own_node);
void find_successor_request(ChordMessage *to_return,
                         Node *send_to, uint64_t key);
void r_find_succ_request(ChordMessage *to_return,
			 Node *send_to, int key,
			 Node *requester);
void get_predecessor_request(ChordMessage *to_return,
                          Node *send_to);
void check_predecessor_request(ChordMessage *to_return,
			       Node *send_to);
void get_successor_list_request(ChordMessage *to_return,
				Node *send_to);

// RESPONSES
void notify_response(int fd);
void find_successor_response(int fd, Node *node);
void rFindSuccResp(int fd, int key, Node *node);
void get_predecessor_response(int fd, Node *node);
void check_predecessor_response(int fd);
void get_successor_list_response(int fd,
				 Node **successors,
				 int num);

// HELPER FUNCTIONS
void pack_and_send(int fd, ChordMessage *msg);
void send_and_return(ChordMessage *to_return,
		     ChordMessage *msg, Node *to_send);
int socket_and_connect(Node *node);
void node_init(Node *node, struct sockaddr_in *addr);
