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

// REQUESTS
void notify_request(ChordMessage *to_return,
                    Node *send_to, Node *own_node);
void find_successor_request(ChordMessage *to_return,
                         Node *send_to, Node *own_node);
void get_predecessor_request(ChordMessage *to_return,
                          Node *send_to);
// RESPONSES
void notify_response(Node *to_send);
void find_successor_response(Node *to_send, Node *node);
void get_predecessor_response(Node *to_send, Node *node);

// HELPER FUNCTIONS
void pack_and_send(Node *to_send, ChordMessage *msg);
void send_and_return(ChordMessage *to_return,
		     ChordMessage *msg, Node *to_send);
int socket_and_connect(Node *node);
void node_init(Node *node, struct sockaddr_in *addr);
