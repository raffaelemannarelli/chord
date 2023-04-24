#include <inttypes.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include "chord.pb-c.h"

#ifndef LEN
#define LEN
// Length of a Chord Node or item key
//const uint8_t KEY_LEN = 8;



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
#endif

/**
 * @brief Print out the node or item key.
 * 
 * NOTE: You are not obligated to utilize this function, it is just showing
 * You how to properly print out an unsigned 64 bit integer.
 */
void printKey(uint64_t key);
void stabilize();
void notify(Node *pot_pred);
void find_successor(Node *to_ruturn, uint64_t id);
void closest_preceding_node(Node *to_return, uint64_t id);
void fix_fingers();
void check_predecessor();
void create();
void join(struct sockaddr_in *join_addr);
void init_finger_table();
void update_successors();
void handle_message(int fd);
void handle_command();
void print_node(Node *node);
void look_up(uint64_t key);
