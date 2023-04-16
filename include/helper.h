// helper functions
void add_connection(struct pollfd **pfds, int fd,
                    int *p_cons, int *p_size);
void remove_connection(struct pollfd **pfds, int fd,
		       int* p_cons);
void update_chord(struct chord_arguments *args,
                  struct timespec *curr_time,
		  struct timespec *last_stab,
                  struct timespec *last_ff,
		  struct timespec *last_cp);
double time_diff(struct timespec *t1,
		 struct timespec *t2);
double deci_to_sec(int time);
int socket_and_assert();
void bind_and_assert(int sock, struct sockaddr* addr);
void listen_and_assert(int sock);
