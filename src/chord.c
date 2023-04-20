#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <poll.h>
#include <time.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>


#include "chord_arg_parser.h"
#include "chord.h"
#include "hash.h"
#include "message.h"
#include "helper.h"

#define START_PFDS 16
#define BUFFER_SIZE 1024
#define COMMAND_MAX 100 // IDK what this value should be rn
#define FINGER_SIZE 160

// global variables
// finger_table size set to bit length of SHA-1
// NEED TO INITIALIZE ALL NODES IN ORDER TO PACK
//const uint8_t KEY_LEN = 8;
Node own_node;
Node finger_table[FINGER_SIZE];
Node *predecessor;
Node **successors;
int n_successors;
uint64_t node_key;
char command[COMMAND_MAX]; // buffer for commands
int next;

// TODO: FIGURE OUT IF query_id in msg IS IMPORTANT

void printKey(uint64_t key) {
  printf("%" PRIu64, key);
}

// may have to change connects---- it blocks :(
void stabilize() {
  // prevent making calls if successor currently set to self
  if (successors[0] == &own_node)
    return;  
  
  // request successors' predecessor
  ChordMessage response;
  get_predecessor_request(&response, successors[0]);
  // x = successor.predecessor
  uint64_t x = response.get_predecessor_response->node->key;

  // see if x should be n's successor 
  if (in_bounds(x, own_node.key, successors[0]->key)) {
    memcpy(successors[0],response.get_predecessor_response->node, sizeof(Node)); // successor = x;
    }

  // notify n's successor of its existence to make n its predecessor
  ChordMessage placeholder;
  notify_request(&placeholder, successors[0], &own_node); 
}

// RPC; replaces pred if potential pred is after pred
void notify(Node *pot_pred) {
  if (predecessor == NULL) {
    predecessor = malloc(sizeof(Node));
    memcpy(predecessor, pot_pred, sizeof(Node));
  } else if (in_bounds(pot_pred->key, predecessor->key,
		       own_node.key)) {
    memcpy(predecessor, pot_pred, sizeof(Node));
  }
}

void find_successor(Node *to_return, uint64_t id) {
  if (in_bounds_closed(id, own_node.key, successors[0]->key)) {
    fprintf(stderr, "successor is own node\n");
    memcpy(to_return, successors[0], sizeof(Node));
  } else {
    // rough sketch
    fprintf(stderr, "successor is not own node\n");
    Node *prime = closest_preceding_node(id);
    ChordMessage response;
    find_successor_request(&response, prime, &own_node);
    memcpy(to_return, response.find_successor_response->node,
	   sizeof(Node));
  }
}

Node* closest_preceding_node(uint64_t id){
  for(int i = FINGER_SIZE-1; i >= 0; i--) {
    if (in_bounds(finger_table[i].key, own_node.key, id)) {
      return &finger_table[i];
    }
  }
  return &own_node;
}

// not finished
void fix_fingers() {
  next = next + 1;
  if(next > FINGER_SIZE) next = 1; // m is the last entry in finger table so we loop

  // finger_table[next] = find_successor(n + 2^(next - 1));  // send find successor queury
  
}

// not finished
void check_predecessor() {
//   if(predecessor has failed) // send heartbeat message asking if theyre alright, someting
//     predecessor = NULL;
}

// handles creating ring from first chord
void create() {
  // no pred
  predecessor = NULL; 
  // deal with successors, itself is the successor
  successors[0] = &own_node;
}

// TODO
// handles joining a chord to existing ring
void join(struct sockaddr_in *join_addr) {
  // predecessor = nil
  predecessor = NULL;

  // successor = n'.find_sucessor(n)
  Node node;
  node_init(&node, join_addr);
  ChordMessage response;
  fprintf(stderr, "making find_successor_request\n");
  find_successor_request(&response, &node, &own_node);
  fprintf(stderr, "done 'find_successor_request\n");
  
  // TODO: what if this value is null?
  // put received node into sucessors list
  successors[0] = malloc(sizeof(Node));
  memcpy(successors[0],response.find_successor_response->node,sizeof(Node));
}

// initialize entire finger table to point to current node
void init_finger_table() {
  for (int i = 0; i < FINGER_SIZE; i++)
    finger_table[i] = own_node;
}

// TODO; no idea if any of this is correct
void handle_message(int fd, ChordMessage *msg) {

  if (msg->msg_case == CHORD_MESSAGE__MSG_NOTIFY_REQUEST) {
    // notify
    notify(msg->notify_request->node);
    notify_response(fd);
  } else if (msg->msg_case == CHORD_MESSAGE__MSG_FIND_SUCCESSOR_REQUEST) {
    // finds and returns successor for requesting node
    fprintf(stderr, "find successor request received\n");

    // TODO: follow algorithm to get node
    Node successor;
    find_successor(&successor, msg->find_successor_request->key);
    find_successor_response(fd, &successor);
  } else if (msg->msg_case == CHORD_MESSAGE__MSG_GET_PREDECESSOR_REQUEST) {
    // get predecessor
    // TODO: HOW DOES THIS BEHAVE IF PREDECESSOR == NULL???
    get_predecessor_response(fd, predecessor);
  } else if (msg->msg_case == CHORD_MESSAGE__MSG_CHECK_PREDECESSOR_REQUEST) {
    // check predecessor
    fprintf(stderr, "check predecessor request received\n");
  } else if (msg->msg_case == CHORD_MESSAGE__MSG_GET_SUCCESSOR_LIST_REQUEST) {
    // get sucessor list

  } else if (msg->msg_case == CHORD_MESSAGE__MSG_R_FIND_SUCC_REQ) {
    // r find successor
    fprintf(stderr, "r find successor request received\n");
  }
}

// handles commands from stdin
void handle_command() {
  char line[256];
  char string[256];

  printf("> ");
  fgets(line, 256, stdin);
  line[strlen(line)-1] = '\0';

  if (strcmp("PrintState", line) == 0) {
    printf("< Self...\n");
  } else if (strncmp("Lookup ", line, 7) == 0) {
    strcpy(string, line+7);
    printf("< %s\n", string);
  } else {
    printf("< ERROR: BAD FORMAT\n");
  }
}

int main(int argc, char *argv[]) {
  // arguments from parser  
  struct chord_arguments args = chord_parseopt(argc,argv);
  // initializes own_node
  node_init(&own_node, &args.my_address);
  init_finger_table();
  
  // book-keeping for surrounding nodes
  next = 0; // for finger table
  predecessor = NULL;
  successors = malloc(sizeof(Node *)*args.num_successors);
  for (int i = 0; i < args.num_successors; i++)
    successors[i] = NULL;
  n_successors = 0;
  
  // create and bind listening socket for incoming connections
  int listenfd = socket_and_assert();
  bind_and_assert(listenfd,(struct sockaddr*)&args.my_address);
  listen_and_assert(listenfd);
  
  if (args.join_address.sin_port == 0) {
    fprintf(stderr, "creating ring\n");
    // TODO: ensure correct way to distinguish no join address
    // no join address was given    
    create();

  } else {
    fprintf(stderr, "joining ring\n");
    join(&(args.join_address));
    // TODO: need to get all sucessor nodes after node obtained during join
  }
    
  // pfds table and book-keeping to manage all connections
  int p_size = START_PFDS, p_cons = 2;
  struct pollfd *pfds = malloc(sizeof(struct pollfd)*(2 + p_size));
 

  // command fle descriptor
  pfds[0].fd = STDIN_FILENO;
  pfds[0].events = POLLIN;

  // listens for new connections
  pfds[1].fd = listenfd;
  pfds[1].events = POLLIN;

  // init the rest of pfds
  for (int i = 0; i < p_size; i++) pfds[i+2].fd = -1;

  // timespecs for tracking regular intervals
  struct timespec curr_time, last_stab, last_ff, last_cp;
  clock_gettime(CLOCK_REALTIME, &last_stab);
  clock_gettime(CLOCK_REALTIME, &last_ff);
  clock_gettime(CLOCK_REALTIME, &last_cp);
  
  uint8_t buf[BUFFER_SIZE];
  struct sockaddr_in client_addr;
  socklen_t addr_size = sizeof(client_addr);
  // three threads
  // put mutex around global vars
  // main loop
  fprintf(stderr, "entering listening loop with successor port %hu\n",
	  successors[0]->port);
  while (1) {
    int p = poll(pfds, 2 + p_cons, 100);    
    clock_gettime(CLOCK_REALTIME, &curr_time);
    fprintf(stderr, ".");
    
    // handles calling update functions
    //update_chord(&args, &curr_time, &last_stab, &last_ff, &last_cp);

    // TODO: handling incoming messages

    // Handling incoming command on std io, this fd dedicated to commands
    // if (pfds[0].revents & POLLIN) handleCommand();

    // is there a new incoming connection
    // if ()
    
    // thread() ---> 

    // handling packets
    for (int i = 0; i < p_cons && p != 0; i++) {
      if (pfds[i].revents & POLLIN) { // if we have a conncetion
	fprintf(stderr, "got message\n");
	if (pfds[i].fd == STDIN_FILENO) {
	  handle_command();
	} else if (pfds[i].fd == listenfd) {
	  // socket that listen for incoming connections
	  fprintf(stderr, "detected and adding new connection\n");
          int fd = accept(listenfd,(struct sockaddr*)&client_addr,&addr_size);
          assert(fd >= 0);	
          add_connection(&pfds,fd,&p_cons,&p_size);
        } else {
          // socket for an existing chord connection
          int msg_len = recv(pfds[i].fd, buf, BUFFER_SIZE, 0);	

          if (msg_len < 0) {
            // error case
            fprintf(stderr, "error\n");
          } else if (msg_len == 0) {
            // connection closed case
	    fprintf(stderr, "connection closed\n");
            remove_connection(&pfds, pfds[i].fd, &p_cons);
          } else {
            // normal message case
            ChordMessage *msg = chord_message__unpack(NULL, msg_len, buf);
            assert(msg != NULL);
            handle_message(pfds[i].fd, msg);
            chord_message__free_unpacked(msg, NULL);

          }
        } // end of if else pfds.fd == listen
      } // end of pfds.revents pollin if statement
    } // end of p_cons loop
  }      
  return 0;
}
