#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <strings.h>
#include <math.h>
#include <arpa/inet.h> //internet address library
#include <netdb.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <time.h>
#include <openssl/sha.h> //hashing pieces
#include <openssl/hmac.h> // need to add -lssl to compile
#include "bencode.h"
#include "bt_lib.h"
#include "bt_setup.h"

extern int v_print;

char hash_string[28] = "19BitTorrent Protocol00000000"; //initial handshake bytes

void printv(int verbose, const char *format, ...)
{
    //Special type of variable which can handle variable number of arguments
    va_list args;
    va_start(args, format);
    
    // If verbosity flag is on then print it
    if (verbose == 1)
        vfprintf (stdout, format, args);
    else
        ;
}


void hex_to_ascii(unsigned char* buffer, char* hex_id, int len) {
    int i;
    for (i=0; i<len; i++) {
        sprintf(hex_id, "%02x", buffer[i]);
        hex_id+=2;
    }
}


void calc_id(char * ip, unsigned short port, unsigned char *id){
  char data[256];
  int len;
  
  //format print
  len = snprintf(data,256,"%s%u",ip,port);

  //id is just the SHA1 of the ip and port string
  SHA1((unsigned char *) data, len, (unsigned char *) id); 

  return;
}


/**
 * init_peer(peer_t * peer, int id, char * ip, unsigned short port) -> int
 *
 *
 * initialize the peer_t structure peer with an id, ip address, and a
 * port. Further, it will set up the sockaddr such that a socket
 * connection can be more easily established.
 *
 * Return: 0 on success, negative values on failure. Will exit on bad
 * ip address.
 *   
 **/
int init_peer(peer_t *peer, unsigned char * id, char * ip, unsigned short port){
    
  struct hostent * hostinfo;
  char srvrName[INET_ADDRSTRLEN];
    
  //set the host id and port for referece
  memcpy(peer->id, id, ID_SIZE);
  peer->port = port;
    int sockfd=0;

    
  //get the host by name
  if((hostinfo = gethostbyname("localhost")) ==  NULL){
    perror("gethostbyname failure, no such host?");
    herror("gethostbyname");
    exit(1);
  }
    //set defaults
    
    
    
  //zero out the sock address
  bzero(&(peer->sockaddr), sizeof(peer->sockaddr));
      
  //set the family to AF_INET, i.e., Iternet Addressing
  peer->sockaddr.sin_family = AF_INET;
    
  //copy the address to the right place
  bcopy((char *) (hostinfo->h_addr), 
        (char *) &(peer->sockaddr.sin_addr.s_addr),
        hostinfo->h_length);
    
  //encode the port
  peer->sockaddr.sin_port = htons(port);
  strncpy(peer->string_address, "localhost", INET6_ADDRSTRLEN);
    calc_id(ip, port, peer->id);
    
  //initially peer is choked and interested
    peer->choked = 1;
    peer->interested =0;
    peer->recv_bitfield = 0;
    peer->peer_sock = -1; //Not connected to socket descriptor is -1
    
    hex_to_ascii(peer->id, peer->hex_id, ID_SIZE);
  
    printf(" \t The PEER is %s:%hd with peer_id:%s\n", peer->string_address, peer->port, peer->hex_id);
   /* printf("\n\t\tTrying to connect to server \n");
    //struct sockaddr_in *sockaddr;

    
    //make connection to the server
    //memset(&sockaddr, 0, sizeof(sockaddr));
   

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    
    peer->peer_sock = sockfd;
  //  sockaddr->sin_addr.s_addr = peer->destaddr.sin_addr.s_addr;
   // sockaddr->sin_port = nc_args.destaddr.sin_port;
    
   
   
     if (inet_ntop(AF_INET, &peer->sockaddr.sin_addr.s_addr, srvrName, sizeof(srvrName)) != NULL);
    

    
    if(connect(sockfd, (struct sockaddr *) &peer->sockaddr, sizeof(peer->sockaddr)) < 0)
    {
        
        perror("\t netcat_part : ERROR while connecting. Check if the server is running and the port number is correct \n");
        exit(1);
    }
    else
    {
        printf("\t Connection established \n");

    }
    
*/    
    
  return 0;

}

/**
 * print_peer(peer_t *peer) -> void
 *
 * print out debug info of a peer
 *
 **/
void print_peer(peer_t *peer){
  int i;

  if(peer){
    printf("peer: %s:%u ",
           inet_ntoa(peer->sockaddr.sin_addr),
           peer->port);
     printf("id: ");
    for(i=0;i<ID_SIZE;i++){
      printf("%02x",peer->id[i]);
    }
    printf("\n");
  }
}


/* We use this functionality for our peer acting as a server. It should be able to bind to any of the free sockets/ports on the localhost , not just one, in order to support multiple clients. This is achieved by means of the INADDR_ANY argument to the bind function call. Consequent listen and connect calls make the socket bind to a random free port */


int bind_to_ports ( int style , int port, int backlog, bt_info_t *bt_info, bt_args_t *bt_args)
{
    
    char handshake[HANDSHAKE_LENGTH];
    char new_handshake [HANDSHAKE_LENGTH];
    int sockfd, newsockfd, portno;
    socklen_t clilen;
    bt_info_t torrent_info;
    struct sockaddr_in serv_addr, cli_addr;
    ssize_t n;
    bt_msg_t msg3,msg1;
    bt_msg_t *m1=&msg3;
    bt_msg_t *m2=&msg1;
    sockfd = socket(style, SOCK_STREAM, 0);
    if (sockfd < 0)
     {
        perror("ERROR opening socket");
         exit(1);
     }
    bzero((char *) &serv_addr, sizeof(serv_addr));
    portno = port;
    serv_addr.sin_family = style;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(portno);
    if (bind(sockfd, (struct sockaddr *) &serv_addr,
             sizeof(serv_addr)) < 0){
        perror("ERROR on binding");
        exit(1);
        
    }
    
    listen(sockfd,backlog);
    clilen = sizeof(cli_addr);
    newsockfd = accept(sockfd,
                       (struct sockaddr *) &cli_addr,
                       &clilen);
    if (newsockfd < 0)
        perror("ERROR on accept");
    bzero(handshake,68);
    
    memcpy(new_handshake,hash_string,28); //The bit torrent string and reserved 8 bytes with 0's
    memcpy(new_handshake+28,bt_info->info_hash,20); //The SHA1 info hash
    memcpy(new_handshake+48,bt_args->id,20); //The peer id
    int cflag=0;
    int iflag=0;
    char *pmsg;
    pmsg= malloc(bt_info->piece_length);
    

    for(;;)
    {
        n = read(newsockfd,handshake,68);
        if (n < 0) perror("ERROR reading from socket");
        n = write(newsockfd,new_handshake,68);
        if (n < 0) perror("ERROR writing to socket");

       // struct bt_msg *h;
        //h= malloc( sizeof(struct bt_msg)); 
        
        //h->payload.bitfiled=bit;
        int i;
        int q=bt_info->num_pieces;
        printf("pieces : %d\n", q);
                //h->payload.bt_bitfield_t.bitfield[0]="1";
          
        for(i=0;i<q;i++)
           { 
                     m2->payload.bitfiled.bitfield[i]="1";
            }        
         
  
       //printf("hie\n");
      //char *d=   h->payload.bt_bitfield_t.bitfield;
       
       printf("bitfield:  %s\n", m2->payload.bitfiled.bitfield[0]);
       //m2->payload.have=1;
        printf("size of h %d\n", sizeof(m2));
        m2->bt_type=5;      
         n = write(newsockfd,m2, 8+ sizeof(int)*20);
           write_to_log(4,bt_args->log_file,bt_args->peers[0],&torrent_info);
         if (n < 0) 
         {
          perror("ERROR writing to socket");
        }
        else
         { 
          printf("msg sent \n");
        }
 
          n = read(newsockfd, m1 , 8+ sizeof(int));
        if (n < 0) {perror("ERROR reading from socket");}
        else {
                     //    printf("length :%d\n",m1->length );
       // printf("index :%d\n", m1->payload.request.index);     
         if(m1->bt_type=1)
          {
            cflag=1;
            iflag=1;
             printf("State : Not choked \n");
              printf("State : interested \n");
          }
  
        }
      
            if(cflag==1 && iflag==1)
            {

              printf("peer can now request for a file\n");

            for(i=0;i<q;i++)
              {
                 n = read(newsockfd, m1 , 8+ sizeof(int));
                if (n < 0) {perror("ERROR reading from socket");}
                else {
                  printf("request received\n");
                    m1->payload.piece.piece=malloc(bt_info->piece_length);
                   int index= m1->payload.request.index;
                   //printf("index :%d\n",index );
                   int offset=m1->payload.request.begin;
                   int p= bt_info->piece_length;
                   pmsg=create_piece(index,offset,bt_args, bt_info);
                   printf("Index : %d\n", index);
                   //bt_piece_t 
                   m1->payload.piece.piece=pmsg;
                   m1->payload.piece.index=index;
                   m1->payload.piece.begin=offset;
                   m1->bt_type=7;
                  

                   printf("size of pointer :%d\n", sizeof(m1->payload.piece.piece) );
                   printf("total size : %ld\n", sizeof(m1)+ sizeof(m1->payload)+ sizeof(m1->payload.bitfiled)+ sizeof(m1->payload.piece));
                  printf("piece: %s\n", m1->payload.piece.piece);
                    n = write(newsockfd,m1,( sizeof(int)*4));
                    if (n < 0) {perror("ERROR writing from socket");}
                  else {


                  }
                    }
              } 
            }


    }

    close(newsockfd);
    close(sockfd);
    
    
    
    return 0;
}

char *create_piece(int index, int offset,bt_args_t *bt_args, bt_info_t *bt_info)
{
 
  FILE* fd = fopen(bt_info->name, "r");
  ssize_t bytes_read;
  int piece_length=bt_info->piece_length;

 // char *PIECE;
  printf("length %d\n", piece_length );
  printf("file name : %s\n", bt_info->name);
  char *PIECE;
  char buffer[piece_length];
  PIECE=malloc(piece_length);
  long file_size=0;
  size_t amount, lastamount, total;
  if (fd == NULL)
    {
        printf("\t Torrent : %s No such file or directory \n",bt_info->name);
        exit(1);
    }
  file_size=bt_info->length;
  int l=((index)*piece_length+ offset);
  printf("l=  %d\n",l );
  fseek(fd, l, SEEK_SET);

   printf("\t Trying to read a file \n");
   printf("\t the length is %d \n",piece_length);

    //printv(verbosity,"\t Reading file from offset: %d \n", offset);
    lastamount = 0; 
     total=piece_length-offset; 
     amount=0;
   
    while(( amount += fread(buffer,total, 1 ,fd))<= total)
       {
          if (amount == lastamount) {
           break; }
           lastamount = amount;
       }

       strcpy(PIECE,buffer);
      //char *str = (char *) malloc(piece_length+index+offset);
        return PIECE;            
            
    
}
/* Since we made the server bind to random ports to provide for multiple connections, we make the client part scalable as well.  Here we use addrinfo in orer to get all combinationsof IP address/ports to which it can be connected and store them as a linked list. This is done by the addrinfo and getaddrinfo function calls internally. This is then used by the connect system calls.  */


 int make_connection ( int style , char * address, unsigned short port)
{
    int sockfd;
    struct hostent *server;
    struct sockaddr_in serv_addr;
    
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
        perror("ERROR opening socket");
    server = gethostbyname(address);
    if (server == NULL) {
        fprintf(stderr,"ERROR, no such host\n");
        exit(0);
    }
    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    bcopy((char *)server->h_addr,
          (char *)&serv_addr.sin_addr.s_addr,
          server->h_length);
    serv_addr.sin_port = htons(port);
    if (connect(sockfd,(struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0)
        perror("\t Connectin failed \n");
    
    
    return sockfd;
    
}
FILE *fp;


/*Here we just traverse the node structure and populate the bt_info strucutre with the required information such as
  the ANNOUNCE URL , NAME, PIECE_LENGTH, NUM_PIECES and other torrent info */

void parse_bt_info (bt_info_t* bt_info, be_node* node, char* map_key )
{
    unsigned int i;
    char* piece;
    /*Check the node type and extract info accordingly */
    /*This is similar to what is done in the bencode.c file */
    
    switch ( node->type)
    {
            
        case BE_INT : // If we have an integer type.  Two of our attributes are of type integer viz piece and piece length
            if ( (strcasecmp(map_key, "length")) == 0)
            {
                printv(v_print,"\t SIZE : %lld \n", node->val.i);
                bt_info->length = (int)node->val.i;
                
            }
            
            if ((strcasecmp(map_key, "piece length")) == 0 )
            {
                printv(v_print,"\t SIZE of each piece : %lld \n", node->val.i);
                bt_info->piece_length = (int)node->val.i;
            }
            break;
            
            
            
        case BE_STR : //If we have a string. There are 3 possible options here. announce, name, piece
            
            if ((strcasecmp(map_key, "announce")) == 0)
            {
                printf("\t URL : %s \n", node->val.s);
                strcpy(bt_info->announce, node->val.s);
                
            }
            
            if((strcasecmp(map_key,"name" ))== 0)
               {
                   printf("\t FILENAME : %s \n", node->val.s);
                   strcpy(bt_info->name, node->val.s);
                   SHA1((unsigned char *)bt_info->name, strlen(bt_info->announce),bt_info->info_hash);
                   printv(v_print,"\t The size of the info_hash is %lu : \n", strlen(bt_info->info_hash));


               }
            
            if((strcasecmp(map_key,"pieces")) == 0)
            {
                
                bt_info->num_pieces = (int)be_str_len( node ) / 20; //Since its always a multiple of 20 bytes.
                
                //allocate equivalent memory to the piece_hashes buffer pointers .
            
                bt_info->piece_hashes = malloc(bt_info->num_pieces * (sizeof(char*)));
                
                //Allocate memory for each of the piece which we then map to piece_hashes
                printf("\t PIECES : %d \n",  bt_info->num_pieces);
                
                for ( i =0; i< bt_info->num_pieces; i++)
                {
                    piece = malloc(20);
                }
                
                
                /*Now we need to strip out and map each index and its corresponding has to the piece_hashes buffer pointer in bt_info. Since each piece is of 20 bytes we allocate memory accordingly first */
                
                for ( i = 0 ; i<bt_info->num_pieces; i++)
                {
                    memcpy(piece,node->val.s + i*20, 20);
                    bt_info->piece_hashes[i] = piece;
                }
                
                
            }
            break;
            
            
        case BE_LIST : // If we get a list. We are assuming that the current metainfo file does not have lists.
            break;
            
            
            
        case BE_DICT : // A dictionary is just a conglomeration of intergers and strings. Hence we just recurse.
            
            for (i =0 ; node->val.d[i].val ; ++i)
            {
                parse_bt_info(bt_info, node->val.d[i].val, node->val.d[i].key);
                
            }
            
            break;
    
        default :
            
            printf("\t Invalid type in the metainfo. File not per bencoding standard \n");
            break;
            
    }
    
    
}


/*int add_peer ( peer_t *peer, bt_args_t* bt_args, int state)
{


    if(bt_args->peers[0] == NULL){
        peer->peer_sock = state;
        if(peer->peer_sock < 0)
            peer->peer_sock= make_connection(AF_INET, peer->string_address, peer->port);
        bt_args->peers[0] = peer;
        printf("\t Peer is added and connected. Now listen on new port \n");
        return 0;
    }

    return 1;
}*/
void write_to_log ( int m, char filename [], peer_t *peer,  bt_info_t *bt_info, char* data, ...)
{

 va_list args; 
switch(m)
{
  case 1: 
    // printf("hey\n");
      //printf("peer %d\n", peer->port);
     //printf("filename %s\n",filename );
      fp = fopen(filename, "w");
    if (fp == NULL)
      {
        printf("Error opening file!\n");
        exit(1);
      }
    time_t t;
    time(&t);

    fprintf(fp," %s \t ",ctime(&t));
/* print some text */
    fprintf(fp, " HANDSHAKE INIT ");
    fprintf(fp, "peer :%d  ",peer->id );
    fprintf(fp, "port :%d\n ",peer->port );
/* print integers and floats */

     fclose(fp);
     break;
  case 2: 
    // printf("hey\n");
      //printf("peer %d\n", peer->port);
     //printf("filename %s\n",filename );
      fp = fopen(filename, "a");
    if (fp == NULL)
      {
        printf("Error opening file!\n");
        exit(1);
      }
  
    time(&t);

    fprintf(fp," %s ",ctime(&t));
/* print some text */
    fprintf(fp, " HANDSHAKE SUCCESS ");
    fprintf(fp, "peer :%d  ",peer->id );
    fprintf(fp, "port :%d\n ",peer->port );
/* print integers and floats */

     fclose(fp);
     break;
  
  case 3: 
          fp = fopen(filename, "a");
    if (fp == NULL)
      {
        printf("Error opening file!\n");
        exit(1);
      }
    time(&t);

    fprintf(fp," %s",ctime(&t));
/* print some text */
    fprintf(fp, " MESSAGE REQUEST FROM "); 
    fprintf(fp, "peer_id : %d  ",peer->id );
    fprintf(fp, "pieces : %d  ",bt_info->num_pieces );
    fprintf(fp, "length :  %d ",bt_info->length );
    fclose(fp);
     break;

     case 4: 
          fp = fopen(filename, "a");
    if (fp == NULL)
      {
        printf("Error opening file!\n");
        exit(1);
      }
    time(&t);

    va_start(args, data);
    vprintf(fp, data, args);

    fclose(fp);
     break;


 case 5: 
          fp = fopen(filename, "a");
    if (fp == NULL)
      {
        printf("Error opening file!\n");
        exit(1);
      }
    time(&t);

    fprintf(fp," %s",ctime(&t));
/* print some text */
    fprintf(fp, " BITFIELD received from seeder. Sendign REQUEST for pieces "); 
    fclose(fp);
     break;


}
}

/* our handshake functions */

void create_hs(bt_args_t *bt_args , bt_info_t *bt_info, char *handshake)
/* Here we just create the handshake character array as per the protocol */

{
 
    memcpy(handshake,hash_string,28); //The bit torrent string and reserved 8 bytes with 0's
    memcpy(handshake+28,bt_info->info_hash,20); //The SHA1 info hash
    memcpy(handshake+48,bt_args->id,20); //The peer id
    
    printf("\t Handshake created \n");
}


int send_hs_request (bt_args_t *bt_args, peer_t *peer, char *hs )
{
   //just write the request to the socket to which the peer is bound
   printf("\t Attempt to send handshake \n");

   if ( write (peer->peer_sock, hs , HANDSHAKE_LENGTH) < 0 )
   {
       perror("\t Handshake send failed \n");
       exit(1);
   }
    printv(v_print,"\t Handshake sent successfully \n");
    return 0;
}

void recv_hs_resp (peer_t *peer, char *hs)
{
    //again just read the handshake to the socket bound by the peer
    printf("\t Waiting to read handshake response \n");
    if ( read (peer->peer_sock, hs , HANDSHAKE_LENGTH) < 0)
    {
        perror("\t Failed receiving handshake \n");
        exit(1);
    }
    
}


int check_hs (peer_t *peer,char *hash, char *hs)
{

    //Here we dont check for the expected peer id. Just the host hash and the inital 28bytes stream
    
    if ( memcmp (hs, hash , (HANDSHAKE_LENGTH-20)) != 0)
    {
        printf("\t Handhshake mismatch. Peer cannot connect \n");
        close(peer->peer_sock);
        exit(1);
        
    }
    else
    {
        printf("\t Handshake succeeded. \n");
        printf(" \t Peers can begin file transfer \t");
    }
    return 0;
    
}
int read_from_peer(peer_t * peer, bt_msg_t *msg, bt_info_t *bt_info, bt_args_t *bt_args)
{
    printf("Waiting for pieces \n");
    int p=bt_info->piece_length;
    printf("piece_length %d\n",p );
  if ( read (peer->peer_sock, msg , (p+ sizeof(int)*30)) < 0)
    {
        perror("\t Failed receiving piece \n");
        exit(1);
    }
printf("type:  %d\n",msg -> bt_type);
    switch ( msg -> bt_type)
      {

        case BT_BITFILED:
        printf("\t Attempting to create a request message \t");
        create_request(msg,peer); 
          break;


        case BT_PIECE:
         printf("index : %d\n",msg->payload.piece.index);

            FILE* fdi = fopen("downloadnew.mp3", "w");

        //ssize_t numBytesRcvd = 0 , bytes_written =0;
          printf("piece :%d\n",p);
          if((fwrite(msg->payload.piece.piece, p, 1, fdi)) == -1 )
                    printf("\t Error writing to file \n");
                else
                {
                    printf("\t Data saved successfully \n");
                    fclose(fdi);
                    //bytes_written = ftell(fdi);
                    //printv(verbosity,"\t %zd bytes written to file %s  \n", bytes_written, file);
                }
          //  save_file(msg,bt_info,bt_args);
            break;
      }
 
    
}
int read_pieces(peer_t * peer, bt_msg_t *msg, bt_args_t *bt_args)
{
  int p= bt_args->bt_info->piece_length;
  printf("p : %d\n", p);
  //bt_piece_t msg;
 if ( read (peer->peer_sock, msg , ( sizeof(int)*4)) < 0)
    {
        perror("\t Failed receiving piece \n");
        exit(1);
    }
    char *s1;
   // s1=msg->payload.piece.piece;

    printf("piece: %d\n", msg->payload.piece.index);
    FILE* fdi = fopen("downloadnew2.mp3", "a+");

        //ssize_t numBytesRcvd = 0 , bytes_written =0;
          //printf("piece :%d\n",p);
          if((fwrite(s1,p,1, fdi)) == -1 )
                    printf("\t Error writing to file \n");
                else
                {
                    printf("\t Data saved successfully \n");
                    fclose(fdi);
                    //bytes_written = ftell(fdi);
                    //printv(verbosity,"\t %zd bytes written to file %s  \n", bytes_written, file);
                }
}
int save_file(bt_msg_t * msg, bt_info_t *bt_info, bt_args_t *bt_args)
{


  int a=bt_info->piece_length;
  char buffer[a];
  char *s1;
  s1=msg->payload.piece.piece;
    FILE* fdi = fopen(bt_args->save_file, "wa+");
        ssize_t numBytesRcvd = 0 , bytes_written =0;
      printf("piece :%d\n",a);
       if((fwrite(s1, 100, 1, fdi)) == -1 )
                    printf("\t Error writing to file \n");
                else
                {
                    printf("\t Data saved successfully \n");
                    //bytes_written = ftell(fdi);
                    //printv(verbosity,"\t %zd bytes written to file %s  \n", bytes_written, file);
                }

}
int create_request( bt_msg_t * msg, peer_t *peer)
{

   int i;
    int count=0;
   //  msg->payload.have=1;
  
       int p= ( sizeof(msg->payload.bitfiled.bitfield)/ sizeof(msg->payload.bitfiled.bitfield[0]));
   //char *bar = msg->payload.bitfiled.bitfield;
    printf("have:  %d \n", p);
  // printf("bit %c\n", bar);
   // printf("%s | ", bar);
   
   for(i=0;i<p;i++) 
      {
        if(msg->payload.bitfiled.bitfield[i]=="1")
        {
        count++;
        }
      }
   
    
   printf("pieces : %d\n",count );    

   // msg->bt_type=1; 
    msg->bt_type=1;
     send_to_peer(peer,msg);

  //  
  
  double a=2, b=15, c;
  c = pow(b,a);
  for(i=0;i<count;i++)
  { 

    msg->payload.request.index=i;
     msg->payload.request.begin=0;
      msg->payload.request.length=c;
      send_to_peer(peer,msg);
         
    } 
}
    int send_to_peer(peer_t * peer, bt_msg_t * msg)
    {
    int f= sizeof(msg);
   // printf("size %d\n",f );
    //msg->bt_type=1;
    //msg->payload.request.index=2;
   if ( write (peer->peer_sock, msg , 8 + sizeof(int))< 0 )
   {
       perror("\t msg send failed \n");
       exit(1);
   }
    printv(v_print,"\t msg sent successfully \n");
    return 0;




    }


