#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <poll.h>
#include <time.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>

#include "chord_arg_parser.h"
#include "chord.h"
#include "hash.h"
#include "message.h"
#include "helper.h"

#define BUFFER_SIZE 1024
#define FINGER_SIZE 64

// TODO:
//   - TODO: put locks on variables
//   - CRASH DURING SIMUTANEOUS STABILIZES
//   - implement other functions
//   - prevent crashes when other nodes disconnect
//   - may be able to use query_id to hold fd info in nodes
//   - determine if poll or threads should be utilized

// global variables
// finger_table size set to bit length of SHA-1
Node own_node;
Node finger_table[FINGER_SIZE];
Node *predecessor;
Node **successors;
int n_successors;
uint64_t node_key;
int next;

void printKey(uint64_t key) {
  printf("%" PRIu64, key);
}

void stabilize() {
  fprintf(stderr, "stabil");

  // case when no successor known
  if (nodes_equal(successors[0], &own_node)) {
    if (predecessor != &own_node) {
      memcpy(successors[0], predecessor, sizeof(Node));

      // update_successor ??

      ChordMessage placeholder;
      notify_request(&placeholder, successors[0], &own_node); 
    }
    fprintf(stderr, "izing\n");
    return;
  }
  
  // request successors' predecessor
  ChordMessage response;
  get_predecessor_request(&response, successors[0]);
  // x = successor.predecessor
  uint64_t x = response.get_predecessor_response->node->key;

  // see if x should be n's successor 
  if (in_bounds(x, own_node.key, successors[0]->key)) {
    memcpy(successors[0],response.get_predecessor_response->node,
	   sizeof(Node)); // successor = x;

     // update_successor ??
     
    }

  // notify n's successor of its existence to make n its predecessor
  ChordMessage placeholder;
  notify_request(&placeholder, successors[0], &own_node);
  fprintf(stderr, "izing\n");
}

// RPC; replaces pred if potential pred is after pred
void notify(Node *pot_pred) {
  if (predecessor == &own_node) {
    predecessor = malloc(sizeof(Node));
    memcpy(predecessor, pot_pred, sizeof(Node));
  } else if (in_bounds(pot_pred->key, predecessor->key,
		       own_node.key)) {
    memcpy(predecessor, pot_pred, sizeof(Node));
  }
}

void find_successor(Node *to_return, uint64_t id) {
  if (in_bounds_closed(id, own_node.key, successors[0]->key)) {
    memcpy(to_return, successors[0], sizeof(Node));
  } else {
    // rough sketch
    Node prime;
    closest_preceding_node(&prime, id);
    // table has not get updated for proper node, so skip
    if (nodes_equal(&prime, &own_node)) {
      memcpy(to_return, &own_node, sizeof(Node));
    } else {
      ChordMessage response;
      find_successor_request(&response, &prime, id);
      assert(response.find_successor_response != NULL);
      memcpy(to_return, response.find_successor_response->node,
	     sizeof(Node));
    }
  }
}

void closest_preceding_node(Node *to_return, uint64_t id){
  for(int i = FINGER_SIZE-1; i >= 0; i--)
    if (in_bounds(finger_table[i].key, own_node.key, id)) {
      memcpy(to_return, &finger_table[i], sizeof(Node));
      return;
    }
  memcpy(to_return, &own_node, sizeof(Node));
}

// not finished
void fix_fingers() {
  // TODO: does key+val need a modulus?
  fprintf(stderr, "fixing %dth", next+1);
  // set to find successor queury
  uint64_t add_val = 1;
  find_successor(&finger_table[next], own_node.key+(add_val<<(next)));
  // loop next
  if(++next >= FINGER_SIZE)
    next = 0;
  fprintf(stderr, " finger\n");
}

// not finished
void check_predecessor() {
  // TODO: PREVENT STALL IF THIS BREAKS
  // ChordMessage response;
  // check_predecessor_request(&response, predecessor); // ask our pred if he's still there
  // note, im not sure if the response will actually determinei if pred has failed, cuz if it has then
  // i mean we won't get a response, and there is no timeout timer
  // i think how we determine if a pred has failed is based on the return value of connect()
  // so we may have to rethink this format 

//   if(predecessor has failed) // send heartbeat message asking if theyre alright, someting
//     predecessor = NULL;
}

// handles creating ring from first chord
void create() {
  // predecessor = nil (cannot use NULL as error sending messages)
  predecessor = &own_node; 
  // deal with successors, itself is the successor
  memcpy(successors[0], &own_node, sizeof(Node));
}

// handles joining a chord to existing ring
void join(struct sockaddr_in *join_addr) {
  // predecessor = nil (cannot use NULL as error sending messages)
  predecessor = &own_node;

  // successor = n'.find_sucessor(n)
  Node node;
  node_init(&node, join_addr);
  ChordMessage response;
  find_successor_request(&response, &node, own_node.key);
  
  // put received node into sucessors list
  successors[0] = malloc(sizeof(Node));

  assert(response.find_successor_response != NULL);
  assert(response.find_successor_response->node != NULL);

  memcpy(successors[0],response.find_successor_response->node,sizeof(Node));
}

// initialize entire finger table to point to current node
void init_finger_table() {
  for (int i = 0; i < FINGER_SIZE; i++) {
    node__init(&finger_table[i]);
    finger_table[i].key = own_node.key;
    finger_table[i].port = own_node.port;
    finger_table[i].address = own_node.address;
  }
}

void handle_message(int fd) {
  uint8_t buf[BUFFER_SIZE];
  ChordMessage *msg;
  
  int msg_len = recv(fd, buf, BUFFER_SIZE, 0);		 
  if (msg_len < 0) {
    return;
  } else if (msg_len == 0) {
    return;
  } else {
    // normal message case
    msg = chord_message__unpack(NULL, msg_len, buf);
    assert(msg != NULL);
  }
    
  if (msg->msg_case == CHORD_MESSAGE__MSG_NOTIFY_REQUEST) {
    // notify
    fprintf(stderr, "notify received\n");
    notify(msg->notify_request->node);
    notify_response(fd);
  } else if (msg->msg_case == CHORD_MESSAGE__MSG_FIND_SUCCESSOR_REQUEST) {
    // finds and returns successor for requesting node
    Node successor;
    find_successor(&successor, msg->find_successor_request->key);
    //print_node(&successor);
    find_successor_response(fd, &successor);
  } else if (msg->msg_case == CHORD_MESSAGE__MSG_GET_PREDECESSOR_REQUEST) {
    // get predecessor
    // TODO: HOW DOES THIS BEHAVE IF PREDECESSOR == NULL???
    get_predecessor_response(fd, predecessor);
  } else if (msg->msg_case == CHORD_MESSAGE__MSG_CHECK_PREDECESSOR_REQUEST) {
    // check predecessor
    //fprintf(stderr, "check predecessor request received\n");
  } else if (msg->msg_case == CHORD_MESSAGE__MSG_GET_SUCCESSOR_LIST_REQUEST) {
    // get sucessor list
    get_successor_list_response(fd, successors, n_successors);
  } else if (msg->msg_case == CHORD_MESSAGE__MSG_R_FIND_SUCC_REQ) {
    // r find successor
    //fprintf(stderr, "r find successor request received\n");
  }

  chord_message__free_unpacked(msg, NULL);
}

void update_successors() {
  if (!nodes_equal(successors[0], &own_node)) {
    fprintf(stderr, "updating successors\n");         
    ChordMessage *response = get_successor_list_request(successors[0]);
    assert(response != NULL);
    GetSuccessorListResponse *list = response->get_successor_list_response;
    assert(list != NULL);
    for (int i = 1; i < n_successors; i++) 
      memcpy(successors[i], list->successors[i-1], sizeof(Node));
    chord_message__free_unpacked(response,NULL);   
  }
}

// handles commands from stdin
void handle_command() {
  char line[256];
  char string[256];

  fgets(line, 256, stdin);
  line[strlen(line)-1] = '\0';

  if (strcmp("PrintState", line) == 0) {
    printf("< Self "), print_node(&own_node);
    for (int i = 0; i < n_successors; i++)
      printf("< Successor [%d] ", i+1), print_node(successors[i]);
    for (int i = 0; i < FINGER_SIZE; i++)
      printf("< Finger [%d] ", i+1), print_node(&finger_table[i]);

  } else if (strncmp("Lookup ", line, 7) == 0) {
    strcpy(string, line+7);
    printf("< %s ", string);
    printf("%lu\n", hash_string(string));
    look_up(hash_string(string));
  } else {
    printf("< ERROR: BAD FORMAT\n");
  }

  printf("> "), fflush(stdout);
}

// TODO: implement message look-up
void look_up(uint64_t key) {
  if (nodes_equal(successors[0], &own_node)) {
    // one node case
    print_node(&own_node);
  } else {
    Node node;
    find_successor(&node, key);
    print_node(&node);
  }
}

void print_node(Node *node) {
  char ip[24];
  inet_ntop(AF_INET, &(node->address), ip, 24);
  printf("%lu %s %d\n", node->key, ip, ntohs(node->port));
}

int main(int argc, char *argv[]) {
  printf("> "), fflush(stdout);

  // arguments from parser  
  struct chord_arguments args = chord_parseopt(argc,argv);
  // initializes own_node
  node_init(&own_node, &args.my_address);
  init_finger_table();
  
  // book-keeping for surrounding nodes
  next = 0; // for finger table
  n_successors = args.num_successors;
  predecessor = &own_node;
  successors = malloc(sizeof(Node *)*n_successors);
  for (int i = 0; i < args.num_successors; i++) {
    successors[i] = malloc(sizeof(Node));
    memcpy(successors[i], &own_node,sizeof(Node));
  }
  
  // create and bind listening socket for incoming connections
  int listenfd = socket_and_assert();
  bind_and_assert(listenfd,(struct sockaddr*)&args.my_address);
  listen_and_assert(listenfd);
  
  if (args.join_address.sin_port == 0) {
    create();
  } else {
    join(&(args.join_address));
  }
    
  // pfds table and book-keeping to manage all connections
  struct pollfd pfds[2];
  // command fle descriptor
  pfds[0].fd = STDIN_FILENO, pfds[0].events = POLLIN;
  // listens for new connections
  pfds[1].fd = listenfd, pfds[1].events = POLLIN;

  struct sockaddr_in client_addr;
  socklen_t addr_size = sizeof(client_addr);

  // separate thread handles calling update functions
  pthread_t stab_id;
  pthread_create(&stab_id, NULL, &update_chord_stab, &args);
  pthread_t ff_id;
  pthread_create(&ff_id, NULL, &update_chord_ff, &args);
  pthread_t cp_id;
  pthread_create(&cp_id, NULL, &update_chord_cp, &args);
  pthread_t us_id;
  pthread_create(&us_id, NULL, &update_chord_us, &args);

  // main while loop
  while (1) {
    int p = poll(pfds, 2, 100);    

    fprintf(stderr, ".");
    if (pfds[0].revents & POLLIN) { // if we have a conncetion
      handle_command();
    }

    if (pfds[1].revents & POLLIN) {
      int fd = accept(listenfd,(struct sockaddr*)&client_addr,&addr_size);
      assert(fd >= 0);      
      pthread_t thread_id;
      pthread_create(&thread_id, NULL, &handle_message, (void*) fd);
    }
  } 
  return 0;
}
