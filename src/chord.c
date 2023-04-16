#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "chord_arg_parser.h"
#include "chord.h"
#include "hash.h"

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
void join() {
  
}

// TODO; no idea if any of this is correct
void handle_message(ChordMessage *msg) {
  if (msg->oneof_name_case == CHORDMESSAGE__ONEOF_MSG_NOTIFYREQUEST) {
    // notify
    fprintf(stderr, "notify request received\n");
  } else if (msg->oneof_name_case == CHORDMESSAGE__ONEOF_MSG_FINDSUCCESSORREQUEST) {
    // find successor
    fprintf(stderr, "find successor request received\n");
  } else if (msg->oneof_name_case == CHORDMESSAGE__ONEOF_MSG_GETPREDECESSORREQUEST) {
    // get predecessor
    fprintf(stderr, "get predecessor request received\n");
  } else if (msg->oneof_name_case == CHORDMESSAGE__ONEOF_MSG_CHECKPREDECESSORREQUEST) {
    // check predecessor
    fprintf(stderr, "check predecessor request received\n");
  } else if (msg->oneof_name_case == CHORDMESSAGE__ONEOF_MSG_GETSUCCESSORLISTREQUEST) {
    // get successor list
    fprintf(stderr, "successor list request received\n");
  } else if (msg->oneof_name_case == CHORDMESSAGE__ONEOF_MSG_RFINDSECCREQ) {
    // r find successor
    fprintf(stderr, "r find successor request received\n");
  }
}

// creates socket from sockaddr_in pointer
int init_socket(struct sockaddr_in *addr) {
  int sockfd = socket(AF_INET, SOCK_STREAM, 0);
  assert(sockfd >= 0);
  return sockfd;
}

int main(int argc, char *argv[]) {
  // arguments from parser  
  struct chord_arguments *args = chord_parseopt(argc,argv);

  // array for holding r known successors sockets
  int successors[args->num_successors];

  // create listening socket for incoming connections
  int listenfd = init_socket(&(args->my_address));
  assert(bind(sockfd,(struct sockaddr*)&(args->my_address),
	      sizeof(struct sockaddr_in) >= 0));
  assert(listen(sockfd, BACKLOG) >= 0);

  // socket for successor
  int joinfd = init_socket(&(args->join_address));
  assert(connect(joinfd,(struct sockaddr*)&(args->join_address),
		 sizeof(struct sockaddr_in)) >= 0);

  // TODO: implement joining procedure
  join();

  // TODO: will need a way to grow TCP connections
  //       unclear how managing connections should be done
  int p_size = START_PFDS;
  int p_cons = 1;
  struct pollfd *pfds = malloc(sizeof(struct pollfd)*p_size);
  pfds[0].fd = listenfd, pfds[0].events = POLLIN;

  // timespecs for tracking regular intervals
  struct timespec curr_time, last_stab, last_ff, last_cp;
  clock_gettime(CLOCK_REALTIME, &last_stab);
  clock_gettime(CLOCK_REALTIME, &last_ff);
  clock_gettime(CLOCK_REALTIME, &last_cp);

  char buf[BUFFER_SIZE];
  // main loop
  while (1) {
    int p = poll(pfds, p_cons, 10);    
    clock_gettime(CLOCK_REALTIME, &curr_time);

    // calls update functions at appropriate intervals
    void update_chord(args, &curr_time, &last_stab, &last_ff, &last_cp);

    // TODO: handling incoming messages
    // handling packets
    for (int i = 0; i < p_cons && p != 0; i++) {
      if (pfds[i].revents & POLLIN) {
	if (pfds[i].fd == listenfd) {
	  // socket that listen for incoming connections
	  fprintf(stderr, "detected and adding new connection\n");
	  int fd = accept(sock, (struct sockaddr*)
			  &client_addr,&addr_size);
	  assert(fd >= 0);	
	  add_connection(&pfds,fd,&p_cons,&p_size);
	} else {
	  // socket for an existing chord connection
	  int msg_len = recv(pfds[i].fd, buf, BUFFER_SIZE, 0);	

	  if (r < 0) {
	    // error case
	    fprintf(stderr, "error\n");
	  } else if (r == 0) {
	    // connection closed case
	    remove_connection(&pfds,pfds[i],&p_cons);
	  } else {
	    // normal message case
	    // TODO: handle message appropriately
	    // TODO: check format on these functions
	    ChordMessage *msg = chordmesssage__unpack(NULL, msg_len, buf);
	    assert(msg != NULL);
	    handle_message(msg);	    
	    chordmessage__free_unpacked(msg, NULL);
	  }
	}
      }
    }
  }      
  return 0;
}

// adds connection to pollfd array; handles p_cons, p_size, ect.
void add_connection(struct pollfd **pfds, int fd,
		    int *p_cons, int *p_size) {
  if (*p_cons >= *p_size) {
    *p_size = 2*(*p_size);
    *pfds = realloc(*pfds, sizeof(struct pollfd)*(*p_size));
  }

  (*pfds)[*p_cons].fd = fd;
  (*pfds)[*p_cons].events = POLLIN;
  (*p_cons)++;
}

// removes connection from pollfd array; handles p_cons, p_size, ect.
void remove_connection(struct pollfd **pfds, int fd, int* p_cons) {
  close(fd);
  int i = 0;
  while ((*pfds)[i].fd != fd)
    i++;
  while (i-1 < *p_cons) {
    (*pfds)[i] = (*pfds)[i+1];
    i++;
  }

  *p_cons = (*p_cons)-1;
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
    check_predecessors();
    clock_gettime(CLOCK_REALTIME, last_cp);
  }    
}

// returns double of t2 - t1
double time_diff(struct timespec *t1, struct timespec *t2) {
  double result = (double) (t2->tv_sec - t1->tv_sec);
  result += ((double)(t2->tv_nsec - t1->tv_nsec))
    /1000000000;
  return result;
}

// converts deciseconds to seconds
double deci_to_sec(int time) {
  return (double time)/10;
}
