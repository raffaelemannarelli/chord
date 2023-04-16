#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <poll.h>
#include <time.h>
#include <assert.h>
#include <unistd.h>
#include <sys/socket.h>

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
int init_socket() {
  int sockfd = socket(AF_INET, SOCK_STREAM, 0);
  assert(sockfd >= 0);
  return sockfd;
}
