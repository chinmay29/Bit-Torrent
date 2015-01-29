#ifndef _BT_LIB_H
#define _BT_LIB_H

//standard stuff
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include <poll.h>

//networking stuff
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 

//#include "bt_lib.h"
#include "bencode.h"


/*Maximum file name size, to make things easy*/
#define FILE_NAME_MAX 1024

/*Maxium number of connections*/
#define MAX_CONNECTIONS 5

/*initial port to try and open a listen socket on*/
#define INIT_PORT 6667 

/*max port to try and open a listen socket on*/
#define MAX_PORT 6699

/*Different BitTorrent Message Types*/
/* Can create an enumerate type out of this ? */
#define BT_CHOKE 0
#define BT_UNCHOKE 1
#define BT_INTERSTED 2
#define BT_NOT_INTERESTED 3
#define BT_HAVE 4
#define BT_BITFILED 5
#define BT_REQUEST 6
#define BT_PIECE 7
#define BT_CANCEL 8

/*size (in bytes) of id field for peers*/
#define ID_SIZE 20


/* new constants defined as per requirement */

#define BACKLOG 4       /* Based on max connections allowed */
#define HANDSHAKE_LENGTH 68  /* 20 bytes for "19BitTorrent" + 8 bytes of 0's + 20 bytes of sha1 digest + 20 bytes of peer_id */


//holds information about a peer


typedef struct peer
  {
      
    unsigned char id[ID_SIZE]; //the peer id
    unsigned short port; //the port to connect n
    struct sockaddr_in sockaddr; //sockaddr for peer
    int choked; //peer choked?
    int interested; //peer interested?
      
    //added by us as per requirement
      int peer_sock; //This will hold the socket file descriptor to which the peer is bound.
      char string_address[INET6_ADDRSTRLEN]; //Array to hold the address as a string
      char hex_id[2*ID_SIZE+1]; //peer id converted to hex. This can be used to calcuate the unique hash.
      short recv_bitfield; //Check if peer received the bitfield, i.e the data message
      
  }peer_t;

typedef struct {
    char *bitfield[10]; //bitfield where each bit represents a piece that
    //the peer has or doesn't have
    size_t size;//size of the bitfiled
} bt_bitfield_t;




//holds information about a torrent file
typedef struct {
  char announce[FILE_NAME_MAX]; //url of tracker
  char name[FILE_NAME_MAX]; //name of file
   unsigned char info_hash[20]; //info_hash required for handshake
  int piece_length; //number of bytes in each piece
  int length; //length of the file in bytes
  int num_pieces; //number of pieces, computed based on above two values
  char ** piece_hashes; //pointer to 20 byte data buffers containing the sha1sum of each of the pieces
    
} bt_info_t;


//holds all the agurments and state for a running the bt client
typedef struct {
  int verbose; //verbose level
  char save_file[FILE_NAME_MAX];//the filename to save to
  char log_file[FILE_NAME_MAX];//the log file
  char torrent_file[FILE_NAME_MAX];// *.torrent file
    
  FILE * f_save;

  peer_t * peers[MAX_CONNECTIONS]; // array of peer_t pointers
  unsigned char id[ID_SIZE]; //this bt_clients id. Changed to char for handshake purposes.
  int sockets[MAX_CONNECTIONS]; //Array of possible sockets
  struct pollfd poll_sockets[MAX_CONNECTIONS]; //Arry of pollfd for polling for input
  
  /*set once torrent is parsed*/
  bt_info_t * bt_info; //the parsed info for this torrent
    
    
  //Added by us as per requirement
   unsigned short port;
   bt_bitfield_t bitfield;
  

} bt_args_t;


/* The request message structure. This is used to send a request message to the peer. 
   The message contains the index of the piece needed, the offset and the length as power of 2.
   This is as per the bittorrent specifications */

typedef struct{
  int index; //which piece index
  int begin; //offset within piece
  int length; //amount wanted, within a power of two
} bt_request_t;


/* The actual piece structure. Contains the index, offset in each piece and the pointer to the start of the data. */


typedef struct{
  int index; //which piece index
  int begin; //offset within piece
  char *piece; //pointer to start of the data for a piece
} bt_piece_t;


/* The message structure which is exchanged between peers. Contains fields for request flags and other flags as per bit torrent specifications  */


typedef struct bt_msg{
  int length; //length of remaining message, 
              //0 length message is a keep-alive message
  unsigned int bt_type;//type of bt_mesage

  //payload can be any of these
  union { 
   bt_bitfield_t bitfiled;
    int have; //what piece you have
    bt_piece_t piece; //a peice message
    bt_request_t request; //request messge
    bt_request_t cancel; //cancel message, same type as request
    char data[0];//pointer to start of payload, just incase
  }payload;

} bt_msg_t;

/* The message strucutre */






void parse_bt_info(bt_info_t * bt_info, be_node * node, char *map_key);

/*choose a random id for this node*/
    unsigned int select_id();

/*propogate a peer_t struct and add it to the bt_args structure*/
    int add_peer(peer_t *peer, bt_args_t *bt_args, int state);

/*drop an unresponsive or failed peer from the bt_args*/
    int drop_peer(peer_t *peer, bt_args_t *bt_args);

/* initialize connection with peers */
    int init_peer(peer_t *peer, unsigned char * id, char * ip, unsigned short port);


/*calc the peer id based on the string representation of the ip and
  port*/
    void calc_id(char * ip, unsigned short port, unsigned char * id);

/* print info about this peer */
    void print_peer(peer_t *peer);

/* check status on peers, maybe they went offline? */
    int check_peer(peer_t *peer);

/*check if peers want to send me something*/
    int poll_peers(bt_args_t *bt_args);

/*send a msg to a peer*/
    int send_to_peer(peer_t * peer, bt_msg_t * msg);

/*read a msg from a peer and store it in msg*/
int read_from_peer(peer_t * peer, bt_msg_t *msg, bt_info_t *bt_info, bt_args_t *bt_args);

/* save a piece of the file */
    int save_piece(bt_args_t * bt_args, bt_piece_t * piece);

/*load a piece of the file into piece */
    int load_piece(bt_args_t * bt_args, bt_piece_t * piece);

/*load the bitfield into bitfield*/
    int get_bitfield(bt_args_t * bt_args, bt_bitfield_t * bitfield);


/*once we get a connection, we send the bitfields. Added as per requirement */
    void send_bit_field( bt_args_t* bt_args, peer_t* peer);


/*compute the sha1sum for a piece, store result in hash*/
    int sha1_piece(bt_args_t * bt_args, bt_piece_t * piece, unsigned char * hash);


/*Contact the tracker and update bt_args with info learned, 
  such as peer list*/
    int contact_tracker(bt_args_t * bt_args);



/* Added new functions as per requirement. */


/* Once we get a complete piece , we set the index bit to 1 to broadcast that we have it */
    int set_bit(bt_bitfield_t *bitfield, int index);

/* Check if we have a particular piece specified by the index. We can send this piece if requested */

    int i_got(bt_args_t *bt_args, int index);

/* Check if a peer has a particular piece. We may or may not need this piece */

    int they_got(bt_args_t *bt_args, int peer, int index);


/* These methods are used to implement the handshake protocol between peers and establish a connection */




/* start the handshake by sending appropriate data */
    void create_hs ( bt_args_t *bt_args , bt_info_t *bt_info, char * handshake);



/* send the initial request message */
    int send_hs_request ( bt_args_t *bt_args , peer_t *peer, char* handshake);



/* receive the handshake form the peer */
    void recv_hs_resp(peer_t *peer, char* handshake);



/* Check if the handshake is successful. This is done by checking the hash that was sent and received. If the hash mathches only then is a connection initiated */
    int check_hs (peer_t *peer, char* host_hash,char* recvd_hs );



/* once the handshake is complete , establish a connection */
    void initiate_connection (bt_args_t* bt_args, int listener, struct sockaddr_in* client_addr, unsigned int con_length, char* handshake);



/*generic function to print hex data */
    void get_host_addr( char* host_addr);
    void hex_to_ascii(unsigned char* buffer, char* hex_string, int len);



/*functions used to make connections. One just announces port numbers and the other one performs the connections */

int bind_to_ports ( int style, int port, int backlog, bt_info_t *bt_info, bt_args_t *bt_args);

int make_connection ( int style, char *address, unsigned short port);

void write_to_log ( int m, char filename[] ,peer_t *peer, bt_info_t *bt_info );

void printv (int verbose, const char *format, ...);

int save_file(bt_msg_t * msg, bt_info_t *bt_info, bt_args_t *bt_args);

int create_request( bt_msg_t * msg, peer_t *peer);

char *create_piece(int index, int offset,bt_args_t *bt_args, bt_info_t *bt_info);

int read_pieces(peer_t * peer, bt_msg_t *msg, bt_args_t *bt_args);



#endif
