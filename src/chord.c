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
#include "message.h"
#include "helper.h"


#define START_PFDS 16
#define BUFFER_SIZE 1024
#define COMMAND_MAX 100 // IDK what this value should be rn

// global variables
// finger_table size set to bit length of SHA-1
// NEED TO INITIALIZE ALL NODES IN ORDER TO PACK
Node own_node;
Node *finger_table[160];
Node *predecessor;
Node **successors;
int n_successors;
uint64_t node_key;
char command[COMMAND_MAX]; // buffer for commands

// TODO: FIGURE OUT IF query_id in msg IS IMPORTANT

void printKey(uint64_t key) {
  printf("%" PRIu64, key);
}

// TODO
void stabilize() {

}

// TODO
void fix_fingers() {

}

// TODO
void check_predecessor() {

}

// handles creating ring from first chord
void create() {

  // its own predeccessor 
  predecessor = &own_node; 

  // fill out finger table, one entry, itself
  finger_table[0] = emalloc(sizeof(struct Node))
  memcpy(finger_table[0]->id, own_node.id, 20);
  finger_table[0]->address = own_node.addr;
  finger_table[0]->buf = emalloc(BUFFER_SIZE);
  
  // deal with successors, itself is the successor
  successors[0] = &own_node;
}

// TODO
// handles joining a chord to existing ring
void join(struct sockaddr_in *join_addr) {
  int fd = socket_and_assert();
  int c = connect(fd, (struct sockaddr*)join_addr, sizeof(struct sockaddr));
  assert(c >= 0);

  ChordMessage response;
  find_successor_request(&response, fd, &own_node);

  // put received node into sucessors list
  successors[0] = malloc(sizeof(Node));
  memcpy(successors[0],response->find_successor_response->node,sizeof(Node));
  
  close(sock);
}

// TODO; no idea if any of this is correct
void handle_message(int fd, ChordMessage *msg) {



  if (msg->msg_case == CHORD_MESSAGE__MSG_NOTIFY_REQUEST) {
    // notify
  } else if (msg->msg_case == CHORD_MESSAGE__MSG_FIND_SUCCESSOR_REQUEST) {
    // finds and returns successor for requesting node
    fprintf(stderr, "find successor request received\n");

    // TODO: follow algorithm to get node
    Node successor;
    send_successor_request(fd, &successor);
  } else if (msg->msg_case == CHORD_MESSAGE__MSG_GET_PREDECESSOR_REQUEST) {
    // get predecessor
    fprintf(stderr, "get predecessor request received\n");
  } else if (msg->msg_case == CHORD_MESSAGE__MSG_CHECK_PREDECESSOR_REQUEST) {
    // check predecessor
    fprintf(stderr, "check predecessor request received\n");
  } else if (msg->msg_case == CHORD_MESSAGE__MSG_GET_SUCCESSOR_LIST_REQUEST) {
    // get sucessor list

  } else if (msg->msg_case == CHORD_MESSAGE__MSG_R_FIND_SUCC_REQ) {
    // r find successor
    fprintf(stderr, "r find successor request received\n");
  }
}

handleCommand(){
  fgets(command, COMMAND_MAX, stdin);
  // process command
  // print stuff
  return;
}

int main(int argc, char *argv[]) {
  // arguments from parser  
  struct chord_arguments args = chord_parseopt(argc,argv);
  // initializes own_node
  node_init(&own_node, &args.my_address);
  
  // book-keeping for surrounding nodes
  predecessor = NULL;
  successors = malloc(sizeof(Node *)*args.num_successors);
  for (int i = 0; i < args.num_successors; i++)
    successors[i] = NULL;
  n_successors = 0;
  
  // create and bind listening socket for incoming connections
  int listenfd = socket_and_assert();
  bind_and_assert(listenfd,(struct sockaddr*)&args.my_address);
  listen_and_assert(listenfd);
  
  if (args.join_address.sin_port == 0) {
    fprintf(stderr, "creating ring\n");
    // TODO: ensure correct way to distinguish no join address
    // no join address was given    
    create();

  } else {
    fprintf(stderr, "joining ring\n");
    join(&(args.join_address));
    // TODO: need to get all sucessor nodes after node obtained during join
  }
    
  // pfds table and book-keeping to manage all connections
  int p_size = START_PFDS, p_cons = 1;
  struct pollfd *pfds = malloc(sizeof(struct pollfd)*(1 + p_size));

  //command file descriptor
  pfds[0].fd = listenfd
  pfds[0].events = POLLIN;

  // init the rest of pfds
  for (int i = 0; i < p_size; i++) ufds[i+1].fd = -1;

  // timespecs for tracking regular intervals
  struct timespec curr_time, last_stab, last_ff, last_cp;
  clock_gettime(CLOCK_REALTIME, &last_stab);
  clock_gettime(CLOCK_REALTIME, &last_ff);
  clock_gettime(CLOCK_REALTIME, &last_cp);
  
  char buf[BUFFER_SIZE];
  struct sockaddr_in client_addr;
  socklen_t addr_size = sizeof(client_addr);

  // main loop
  fprintf(stderr, "entering listening loop\n");
  while (1) {
    int p = poll(pfds, 1+ p_cons, 100);    
    clock_gettime(CLOCK_REALTIME, &curr_time);

    // handles calling update functions
    update_chord(&args, &curr_time, &last_stab, &last_ff, &last_cp);

    // TODO: handling incoming messages

    // Handling incoming command on std io, this fd dedicated to commands
    if (pfds[0].revents & POLLIN) handleCommand();

    // handling packets
    for (int i = 0; i < p_cons && p != 0; i++) {
      if (pfds[i].revents & POLLIN) {
	fprintf(stderr, "got message\n");
	if (pfds[i].fd == listenfd) {
	  // socket that listen for incoming connections
	  fprintf(stderr, "detected and adding new connection\n");
	  int fd = accept(listenfd,(struct sockaddr*)&client_addr,&addr_size);
	  assert(fd >= 0);	
	  add_connection(&pfds,fd,&p_cons,&p_size);
	} else {
	  // socket for an existing chord connection
	  int msg_len = recv(pfds[i].fd, buf, BUFFER_SIZE, 0);	

	  if (msg_len < 0) {
	    // error case
	    fprintf(stderr, "error\n");
	  } else if (msg_len == 0) {
	    // connection closed case
	    remove_connection(&pfds, pfds[i].fd, &p_cons);
	  } else {
	    // normal message case
	    ChordMessage *msg = chord_message__unpack(NULL, msg_len, buf);
	    assert(msg != NULL);
	    handle_message(pfds[i].fd, msg);
	    chord_message__free_unpacked(msg, NULL);
	  }
	}
      }
    }
  }      
  return 0;
}
