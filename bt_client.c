#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <netinet/in.h>
#include <netinet/ip.h> //ip hdeader library (must come before ip_icmp.h)
#include <netinet/ip_icmp.h> //icmp header
#include <arpa/inet.h> //internet address library
#include <netdb.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <signal.h>
#include <openssl/hmac.h> // need to add -lssl to compile

#include "bencode.h"
#include "bt_lib.h"
#include "bt_setup.h"

int main (int argc, char * argv[]){

  bt_args_t* bt_args;
  bt_info_t torrent_info;
  bt_info_t *bt_info;
  be_node * node; // top node in the bencoding
    peer_t *peer=NULL;
  int i, listen;
  char * handshake, *recv_handshake;
  socklen_t length;
  bt_msg_t msg;
  FILE* log_file;
bt_request_t req;
  char host_ip[INET6_ADDRSTRLEN];
  
    
   //initialize all the pointers
    
    bt_args = malloc(sizeof(bt_args_t) + 2500);
    handshake = malloc(128);
    recv_handshake=malloc(128);
    length = sizeof(struct sockaddr);
    
    bt_args->port = 0;
   parse_args(bt_args, argc, argv);


  if(bt_args->verbose)
   {
     for(i=0;i<MAX_CONNECTIONS;i++){
      if(bt_args->peers[i] != NULL)
        print_peer(bt_args->peers[i]);
    }

  }
    
    if((log_file = fopen(bt_args->log_file, "w")) < 0){
        perror("\t Could not open log file \n");
        exit(1);
    }
    
 

  //read and parse the torrent file
  node = load_be_node(bt_args->torrent_file);
    
   //Get the information from the node. 
   parse_bt_info(&torrent_info, node, "");
    

/*
  if(bt_args->verbose){
    be_dump(node);
  } */


//printf("port %d\n",bt_args->port );
   if(bt_args->port == 0)
   {
     bind_to_ports(AF_INET,INIT_PORT, BACKLOG, &torrent_info, bt_args);
     printf("\t Listening on %d \n", INIT_PORT);
       bt_args -> port = INIT_PORT;
   }
   else
   {
       
       // printf("filename%s\n", f );
       create_hs(bt_args, &torrent_info, handshake);
       send_hs_request(bt_args, bt_args->peers[0], handshake);
       printf("\t Handshake sent successfully \n");
       
       write_to_log ( 1,bt_args->log_file,bt_args->peers[0],&torrent_info);
       recv_handshake = malloc(2000);
       recv_hs_resp(bt_args->peers[0], recv_handshake);
       printf("\t Handshake received successfully \n");
       if((check_hs(bt_args->peers[0], handshake, recv_handshake)) > 0)
       {
           printf("\t Check mismatch \n");
           exit(1);
       }
       else 
       {
        write_to_log ( 2,bt_args->log_file,bt_args->peers[0],&torrent_info);
       }
      
        //msg->length=torrent_info->length;

      
   }
 
 
    bt_args->bt_info = &torrent_info;
    
    
    
    
  //main client loop
  printf("Starting Main Loop\n");
  int interested=1;
  do{

    //try to accept incoming connection from new peer
       bt_info=&torrent_info;
         
        bt_msg_t *mbit= &msg;
        bt_msg_t r;
        bt_msg_t *t=&r;
       
        read_from_peer(bt_args->peers[0], mbit, &torrent_info, bt_args);
    
        printf("request msg sent\n");
       write_to_log(5,bt_args->log_file,bt_args->peers[0],&torrent_info);  
        int o=bt_info->num_pieces;
        //printf("no of pieces %d\n",o );
      /*  for(i=0;i<o;i++)
    {
        read_from_peer(bt_args->peers[0], mbit, &torrent_info,bt_args);
    }*/
      for(i=0;i<o;i++)
      {
        read_pieces(bt_args->peers[0],t,bt_args);
      }

      printf("pieces read successfully\n");
        interested=0;
    

    //poll current peers for incoming traffic
    //   write pieces to files
    //   udpdate peers choke or unchoke status
    //   responses to have/havenots/interested etc.
    
    //for peers that are not choked
    //   request pieaces from outcoming traffic

    //check livelenss of peers and replace dead (or useless) peers
    //with new potentially useful peers
    
    //update peers, 

  }while(interested==1);

  return 0;
}
