#---------------------------------------#
# Project 3 : BitTorrent client implementation
# Harsh Pathak
# hpathak@indiana.edu
#---------------------------------------#

INTRODUCTION :

	In this project assignment, we implement a bit-torrent client which adheres to the bit-torrent specifications.  We do not implement the functionality of the tracker. The client which we have implemented can support connection type of 1-seeder and N leechers with restart functionality; i.e one client who has the complete file will accept connection from 5 ( since this is the default limit on connections, can be changed as required) other peers and upload the file to each of them . If any one of the downloading client fails abruptly, starts again and connects to the seeder, it will continue the download from where it left off. This restart ability doesn't require additional complex code as such, just some clever use of file manipulations.  

IMPLEMENTATION DETAILS :

   The client implementation is spread across a set of CPP files which divide the functionality to make the code reusable.  The following files have been implemented, based in decreasing importance of functionality:

 a) bt_lib.h : This is the main header file which contains definitions of peer structures, message structures, payload structure, information regarding different connection states such as CHOKED, UNCHOKED etc. It also contains method definitions to add a peer, drop a peer, establish handshake, check handshake validity and establish a connection to a peer. Since this is a header file, it contains just the prototypes, not the actual implementations. 

b) bt_lib.cpp : This is the cpp code file which implements the bt_lib header prototypes described earlier. It essentially implements the bititorrent client protocol . It implements methods such as check_handshake, initiate_connection, add_peer, drop_peer etc, which we have defined in the bit_lib header. This file also has code check liveness of the peer by polling other peers intermittently.  Another method which is implemented here is the calc_id method which generates an unique peer_id and a SHA1 hash. The SHA1 has we used is just based on the IP address and the port number provided. 

c) bt_setup.cpp : 



d) bt_client.cpp : This is the main client functionality. It just calls functions implemented in other supporting files (bt_lib, bt_setup ) and runs the main clients infinite loop for accepting connections and handling other tasks such as bencode parsing the files, checking liveness of the peers etc. 

e) bencode.cpp, bencode.h : This is the torrent file parsing library. It is the C implementation of the bencoding algorithm. We are using the files as-is with only basic modifications which are required to make the file cpp compliant.


COMPILING AND EXECUTING THE CODE:

A) COMPILING :

This project uses multiple files which work together as a whole. Hence all of these files need to be compiled together. We provide a make file which does this. To compile the program just execute the below command 

   make


B) EXECUTING: 

    The bit torrent can be executed in any one of the following ways :





