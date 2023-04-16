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

// TODO
// handles creating ring from first chord
void create() {

}

// TODO
// handles joining a chord to existing ring
void join() {
  //int joinfd = init_socket(&(args.join_address));
  //assert(connect(joinfd,(struct sockaddr*)&(args.join_address),
  //		 sizeof(struct sockaddr_in)) >= 0);
  
}

// TODO; no idea if any of this is correct
void handle_message(ChordMessage *msg) {
  if (msg->msg_case == CHORD_MESSAGE__MSG_NOTIFY_REQUEST) {
    // notify
    fprintf(stderr, "notify request received\n");
  } else if (msg->msg_case == CHORD_MESSAGE__MSG_FIND_SUCCESSOR_REQUEST) {
    // find successor
    fprintf(stderr, "find successor request received\n");
  } else if (msg->msg_case == CHORD_MESSAGE__MSG_GET_PREDECESSOR_REQUEST) {
    // get predecessor
    fprintf(stderr, "get predecessor request received\n");
  } else if (msg->msg_case == CHORD_MESSAGE__MSG_CHECK_PREDECESSOR_REQUEST) {
    // check predecessor
    fprintf(stderr, "check predecessor request received\n");
  } else if (msg->msg_case == CHORD_MESSAGE__MSG_GET_SUCCESSOR_LIST_REQUEST) {
    // get successor list
    fprintf(stderr, "successor list request received\n");
  } else if (msg->msg_case == CHORD_MESSAGE__MSG_R_FIND_SUCC_REQ) {
    // r find successor
    fprintf(stderr, "r find successor request received\n");
  }
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

int main(int argc, char *argv[]) {
  // arguments from parser  
  struct chord_arguments args = chord_parseopt(argc,argv);

  // array for holding r known successors sockets
  int successors[args.num_successors];

  // create and bind listening socket for incoming connections
  int listenfd = init_socket();
  assert(bind(listenfd,(struct sockaddr*)&(args.my_address),
	      sizeof(struct sockaddr_in) >= 0));
  assert(listen(listenfd, BACKLOG) >= 0);

  if (args.join_address.sin_port == 0) {
    // TODO: check this is a correct way to distinguish no join address
    // no join address was given    
    create();
  } else {
    join();
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
	    handle_message(msg);
	    chord_message__free_unpacked(msg, NULL);
	  }
	}
      }
    }
  }      
  return 0;
}
