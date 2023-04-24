#include "chord_arg_parser.h"
#include "chord.h"

// helper functions
double time_diff(struct timespec *t1,
		 struct timespec *t2);
double deci_to_sec(int time);
int socket_and_assert();
void bind_and_assert(int sock, struct sockaddr* addr);
void listen_and_assert(int sock);
uint64_t hash_string(char *str);
uint64_t hash_addr(struct sockaddr_in *addr);
void addr_from_node(struct sockaddr_in *addr, Node *node);
int in_bounds(uint64_t x, uint64_t a, uint64_t b);
int in_bounds_closed(uint64_t x, uint64_t a, uint64_t b);
void update_chord_stab(struct chord_arguments *args);
void update_chord_ff(struct chord_arguments *args);
void update_chord_cp(struct chord_arguments *args);
void update_chord_us(struct chord_arguments *args);
int nodes_equal(Node *n1, Node *n2);
