#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <poll.h>
#include <time.h>
#include <assert.h>
#include <unistd.h>
#include <sys/socket.h>
#include <stdint.h>

#include "chord_arg_parser.h"
#include "chord.h"
#include "hash.h"

#define BACKLOG 16

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

// returns double of t2 - t1
double time_diff(struct timespec *t1, struct timespec *t2) {
  double result = (double) (t2->tv_sec - t1->tv_sec);
  result += ((double)(t2->tv_nsec - t1->tv_nsec))
    /1000000000;
  return result;
}

// converts deciseconds to seconds
double deci_to_sec(int time) {
  return ((double) time)/10;
}

// creates socket from sockaddr_in pointer and asserts valid
int socket_and_assert() {
  int sockfd = socket(AF_INET, SOCK_STREAM, 0);
  assert(sockfd >= 0);
  return sockfd;
}

void bind_and_assert(int sock, struct sockaddr* addr) {
  int b = bind(sock, addr, sizeof(*addr));
  assert(b >= 0);
}

void listen_and_assert(int sock) {
  int l = listen(sock, BACKLOG);
  assert(l >= 0);
}

// hashes address based on ip address and port
uint64_t hash_addr(struct sockaddr_in *addr) {
  // helpful variables
  uint8_t checksum[20];
  struct sha1sum_ctx *ctx = sha1sum_create(NULL, 0);
  assert(ctx != NULL);
  // copy ip and port to hash
  unsigned char port_and_addr[6];
  memcpy(port_and_addr, &(addr->sin_addr), 4);
  memcpy(port_and_addr+4, &(addr->sin_port), 2);
  // call hash and return truncated value
  sha1sum_finish(ctx, port_and_addr, 6, checksum);
  sha1sum_destroy(ctx);
  return sha1sum_truncated_head(checksum);
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

void addr_from_node(struct sockaddr_in *addr, Node *node) {
  addr->sin_family = AF_INET;
  addr->sin_addr.sin_addr = node->address;
  addr->sin_port = htons(node->port);
}

int in_bounds(int x, int a, int b) {
  if (a <= b) {
    if (x > a && x < b)
      return 1;
    else
      return 0;
  } else {
    if (x > a || x < b)
      return 1;
    else
      return 0;
  }
}

int in_bounds_closed(int x, int a, int b) {
  if (a <= b) {
    if (x > a && x <= b)
      return 1;
    else
      return 0;
  } else {
    if (x > a && x <= b)
      return 1;
    else
      return 0;
  }
}

