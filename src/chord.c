#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "chord_arg_parser.h"
#include "chord.h"
#include "hash.h"

#define BACKLOG 16
#define START_PFDS 16

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
  //int p_size = START_PFDS;
  //int p_cons = 1;
  //struct pollfd *pfds = malloc(sizeof(struct pollfd)*p_size);
  //pfds[0].fd = listenfd, pfds[0].events = POLLIN;

  // timespecs for tracking regular intervals
  struct timespec curr_time, last_stab, last_ff, last_cp;
  clock_gettime(CLOCK_REALTIME, &last_stab);
  clock_gettime(CLOCK_REALTIME, &last_ff);
  clock_gettime(CLOCK_REALTIME, &last_cp);

  // main loop
  while (1) {
    int p = poll(pfds, p_cons, 100);    
    clock_gettime(CLOCK_REALTIME, &curr_time);

    // calls update functions at appropriate intervals
    void update_chord(args, &curr_time, &last_stab, &last_ff, &last_cp);

    // TODO: handling incoming messages
            
  }
      
  return 0;
}



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

double time_diff(struct timespec *t1, struct timespec *t2) {
  double result = (double) (t2->tv_sec - t1->tv_sec);
  result += ((double)(t2->tv_nsec - t1->tv_nsec))
    /1000000000;
  return result;
}

double deci_to_sec(int time) {
  return (double time)/10;
}
