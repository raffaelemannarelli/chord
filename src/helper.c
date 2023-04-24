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
#include "chord.pb-c.h"
#include "chord.h"
#include "hash.h"
#include "helper.h"
#include "message.h"

#define BACKLOG 16
#define UPDATE_TIME .5
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

uint64_t hash_string(char *str) {
  // helpful variables
  uint8_t checksum[20];
  struct sha1sum_ctx *ctx = sha1sum_create(NULL, 0);
  assert(ctx != NULL);
  // call hash and return truncated value
  sha1sum_finish(ctx, (const uint8_t*)str,
		 strlen(str), checksum);
  sha1sum_destroy(ctx);
  return sha1sum_truncated_head(checksum);
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
void update_chord(struct chord_arguments *args) {
  // track time
  struct timespec curr_time, last_stab, last_ff, last_cp, last_us;
  clock_gettime(CLOCK_REALTIME, &last_stab);
  clock_gettime(CLOCK_REALTIME, &last_ff);
  clock_gettime(CLOCK_REALTIME, &last_cp);
  clock_gettime(CLOCK_REALTIME, &last_us);
  
  while (1) {
    clock_gettime(CLOCK_REALTIME, &curr_time);
    if (time_diff(&last_stab,&curr_time) >
	deci_to_sec(args->stabilize_period)) {
      stabilize();
      clock_gettime(CLOCK_REALTIME, &last_stab);
    }
    if (time_diff(&last_ff,&curr_time) >
	deci_to_sec(args->fix_fingers_period)) {
      fix_fingers();
      clock_gettime(CLOCK_REALTIME, &last_ff);
    }
    if (time_diff(&last_cp,&curr_time) >
	deci_to_sec(args->check_predecessor_period)) {
      check_predecessor();
      clock_gettime(CLOCK_REALTIME, &last_cp);
    }
    if (time_diff(&last_us,&curr_time) > UPDATE_TIME) {
      update_successors();
      clock_gettime(CLOCK_REALTIME, &last_us);
    }
  }
}

void addr_from_node(struct sockaddr_in *addr, Node *node) {
  addr->sin_family = AF_INET;
  addr->sin_addr.s_addr = node->address;
  addr->sin_port = node->port;
}

int in_bounds(int x, int a, int b) {
  if (a == b) {
    return 1;
  } else if (a < b) {
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
  if (a == b) {
    return 1;
  } if (a < b) {
    if (x > a && x <= b)
      return 1;
    else
      return 0;
  } else {
    if (x > a || x <= b)
      return 1;
    else
      return 0;
  }
}

int nodes_equal(Node *n1, Node *n2) {
  /*if (memcmp(n1, n2, sizeof(Node)) == 0)
    return 1;
  else
  return 0;*/
  if (n1->port != n2->port || n1->address != n2->address ||
      n1->key != n2->key)
    return 0;
  return 1;
}
