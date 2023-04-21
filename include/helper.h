#include "chord_arg_parser.h"
#include "chord.h"

// helper functions
void add_connection(struct pollfd **pfds, int fd,
                    int *p_cons, int *p_size);
void remove_connection(struct pollfd **pfds, int fd,
		       int* p_cons);
double time_diff(struct timespec *t1,
		 struct timespec *t2);
double deci_to_sec(int time);
int socket_and_assert();
void bind_and_assert(int sock, struct sockaddr* addr);
void listen_and_assert(int sock);
uint64_t hash_string(char *str);
uint64_t hash_addr(struct sockaddr_in *addr);
void addr_from_node(struct sockaddr_in *addr, Node *node);
int in_bounds(int x, int a, int b);
int in_bounds_closed(int x, int a, int b);
void update_chord(struct chord_arguments *args);

