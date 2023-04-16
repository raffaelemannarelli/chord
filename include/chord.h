#include <inttypes.h>

#include "chord.pb-c.h"

// Length of a Chord Node or item key
const uint8_t KEY_LEN = 8;

/**
 * @brief Used to send messages to other Chord Nodes.
 * 
 * NOTE: Remember, you CANNOT send pointers over the network!
 */
typedef struct Message
{
    uint64_t len;
    void *ChordMessage;
} Message;

/**
 * @brief Print out the node or item key.
 * 
 * NOTE: You are not obligated to utilize this function, it is just showing
 * you how to properly print out an unsigned 64 bit integer.
 */
void printKey(uint64_t key);
void stabilize();
void fix_fingers();
void check_predecessor();
void create();
void join();
void handle_message(ChordMessage *msg);

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
int init_socket(struct sockaddr_in *addr);
