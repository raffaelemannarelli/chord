#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <poll.h>
#include <time.h>
#include <unistd.h>

#include "chord_arg_parser.h"
#include "chord.h"
#include "hash.h"
#include "helper.h"

#define BACKLOG 16
#define START_PFDS 16
#define BUFFER_SIZE 1024

// global variables
// finger_table size set to bit length of SHA-1
Node *finger_table[160];
Node *predecessor;
Node **successors;
int n_successors;
uint64 node_key;

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
void create(sockaddr_in ***succ) {
  finger_table[0] = malloc(sizeof(Node));
  finger_table[0]->
  // pseudocode sets to self. Use NULL instead?
}

// TODO
// handles joining a chord to existing ring
void join(sockaddr_in ***succ, sockaddr_in *join_addr) {
  // TODO: implement find successor call
  int sock = init_socket();
  assert(connect(sock, (sockaddr*)join_addr, sizeof(struct sockaddr)) >= 0);

  ChordMessage msg = CHORD_MESSAGE__INIT;
  GetSucessorListRequest request = GET_SUCCESSOR_LIST_REQUEST__INIT;
  msg.msg_case = &request;

  int len = chord_message__get_packet_size(&msg);
  char buf[BUFFER_SIZE];

  chord_message__pack(&msg,buf);

  send(sock, buf, len, 0);
  int msg_len = recv(sock, buf, BUFFER_SIZE, 0);

  msg = chord_message__unpack(NULL, msg_len, buf);

  GetSucessorListResponse *response = msg.msg_case;
  
  for (int i = 0)
  
  chord_message__free_unpacked(msg, NULL);
    
  close(sock);
}

// TODO; no idea if any of this is correct
void handle_message(int fd, ChordMessage *msg) {
  char buf[BUFFER_SIZE];
  ChordMessage msg = CHORD_MESSAGE__INIT;
  // TODO: need to generalize this for all cases
  GetSuccessorListResponse response;
  if (msg->msg_case == CHORD_MESSAGE__MSG_NOTIFY_REQUEST) {
    // notify
  } else if (msg->msg_case == CHORD_MESSAGE__MSG_FIND_SUCCESSOR_REQUEST) {
    // finds and returns successor for requesting node
    fprintf(stderr, "find successor request received\n");
    
  } else if (msg->msg_case == CHORD_MESSAGE__MSG_GET_PREDECESSOR_REQUEST) {
    // get predecessor
    fprintf(stderr, "get predecessor request received\n");
  } else if (msg->msg_case == CHORD_MESSAGE__MSG_CHECK_PREDECESSOR_REQUEST) {
    // check predecessor
    fprintf(stderr, "check predecessor request received\n");
  } else if (msg->msg_case == CHORD_MESSAGE__MSG_GET_SUCCESSOR_LIST_REQUEST) {
    // this sends own successor list???
    // TODO: this
    fprintf(stderr, "successor list request received\n");
    response = GET_SUCCESSOR_LIST_RESPONSE__INIT;
    response.n_successors = n_successors;
    Node **node = malloc(sizeof(Node*)n_successors);
    for (int i = 0; i < n_successors; i++) {
      node[i] = malloc(sizeof(struct Node));
      (*node[i]) = NODE__INIT;
      // TODO: is key parameter unused for a host node???
      node[i]->address = successor[i].sin_addr.s_addr;
      node[i]->port = successor[i].sin_port;
    }
    msg.msg_case = &response;    
  } else if (msg->msg_case == CHORD_MESSAGE__MSG_R_FIND_SUCC_REQ) {
    // r find successor
    fprintf(stderr, "r find successor request received\n");
  }
  // pack and send message
  chord_message__pack(&msg,buf);
  int s = send(fd, buf, len, 0);
}

// calls update functions if time interval has passed
void update_chord(struct chord_arguments *args,
		  struct timespec *curr_time, struct timespec *last_stab,
		  struct timespec *last_ff, struct timespec *last_cp) {
  if (time_diff(last_stab,curr_time) >
      deci_to_sec(args->stabilize_period)) {
    stabilize();
    clock_gettime(CLOCK_REALTIME, last_stab);
  }
  if (time_diff(last_stab,curr_time) >
      deci_to_sec(args->fix_fingers_period)) {
    fix_fingers();
    clock_gettime(CLOCK_REALTIME, last_ff);
  }
  if (time_diff(last_cp,curr_time) >
      deci_to_sec(args->check_predecessor_period)) {
    check_predecessor();
    clock_gettime(CLOCK_REALTIME, last_cp);
  }    
}

// hashes address based on ip address and port
int hash_addr(struct sockaddr_in *addr) {
  // helpful variables
  uint8_t checksum[20];
  struct sha1sum_ctx *ctx = sha1sum_create(NULL, 0);
  assert(ctx != NULL);
  // copy ip and port to hash
  unsigned char	port_and_addr[6];
  memcpy(port_and_addr, addr->sin_addr, 4);
  memcpy(port_and_addr+4, addr->sin_port, 2);
  // call hash and return truncated value
  sha1sum_finish(ctx, port_and_addr, 6, checksum);
  sha1sum_destroy(ctx);
  return sha1sum_truncated_head(checksum);
}

int main(int argc, char *argv[]) {
  // arguments from parser  
  struct chord_arguments args = chord_parseopt(argc,argv);

  // obtain node_key hash
  node_key = hash_addr(&args.my_address);
  
  // book-keeping for surrounding nodes
  predecessor = NULL;
  successors = malloc(sizeof(Node *)*args.num_successors);
  for (int i = 0; i < args.num_sucessors; i++)
    successors[i] = NULL;
  n_successors = 0;
  
  // create and bind listening socket for incoming connections
  int listenfd = init_socket();
  assert(bind(listenfd,(struct sockaddr*)&(args.my_address),
	      sizeof(struct sockaddr_in) >= 0));
  assert(listen(listenfd, BACKLOG) >= 0);

  if (args.join_address.sin_port == 0) {
    // TODO: check this is a correct way to distinguish no join address
    // no join address was given    
    create(&sucessors);
  } else {
    join(&sucessors, &(args.join_address));
    // TODO: need to get all sucessor nodes after node obtained during join
  }
    
  // pfds table and book-keeping to manage all connections
  int p_size = START_PFDS, p_cons = 1;
  struct pollfd *pfds = malloc(sizeof(struct pollfd)*p_size);
  pfds[0].fd = listenfd, pfds[0].events = POLLIN;

  // timespecs for tracking regular intervals
  struct timespec curr_time, last_stab, last_ff, last_cp;
  clock_gettime(CLOCK_REALTIME, &last_stab);
  clock_gettime(CLOCK_REALTIME, &last_ff);
  clock_gettime(CLOCK_REALTIME, &last_cp);
  
  char buf[BUFFER_SIZE];
  struct sockaddr_in client_addr;
  socklen_t addr_size = sizeof(client_addr);
  // main loop
  while (1) {
    int p = poll(pfds, p_cons, 10);    
    clock_gettime(CLOCK_REALTIME, &curr_time);

    // calls update functions at appropriate intervals
    update_chord(&args, &curr_time, &last_stab, &last_ff, &last_cp);

    // TODO: handling incoming messages
    // handling packets
    for (int i = 0; i < p_cons && p != 0; i++) {
      if (pfds[i].revents & POLLIN) {
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
