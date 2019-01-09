// Client side C/C++ program to demonstrate Socket programming
#include <stdio.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include "peer.h"

#define PORT 12000 			// Server's listening  port
#define SERVER_IP "129.21.34.191"	// Server's IP address. In this case it is docker01

int main(int argc, char const *argv[])
{
    int maxPeers = 3;

    int my_id = 0;
    char *peer_ip = "";
    char *msg = "I am client";
    peer *thisPeer = createPeer(my_id, peer_ip, maxPeers);

    peer *peer2 = createPeer(my_id, peer_ip, maxPeers);

    create_loc_sock(thisPeer);
    conn_2_serv(thisPeer, peer2);
    send_msg(thisPeer, peer2, msg, "hello");
    recv_reply(thisPeer);


    /*struct sockaddr_in serv_addr;
    struct sockaddr_in address;
    int sock = 0, valread;
    char *hello = "Hello from client";
    char buffer[1024] = {0};


    // Create a local socket for communication
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        printf("\n Socket creation error \n");
        return -1;
    }



    // Setting the server address to 0's.
    memset(&serv_addr, '0', sizeof(serv_addr));

    serv_addr.sin_family = AF_INET; 	// Address family
    serv_addr.sin_port = htons(PORT); 	// Server port in network byte order

    // Convert IPv4 and IPv6 addresses from text to binary form
    if(inet_pton(AF_INET, SERVER_IP, &serv_addr.sin_addr)<=0)
    {
        printf("\nInvalid address/ Address not supported \n");
        return -1;
    }

    // Initiate a connection request to the server.
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        printf("\nConnection Failed \n");
        return -1;
    }

    // Sending message to the server.
    send(sock , hello , strlen(hello) , 0 );
    printf("Hello message sent\n");

    // Reading reply from the server.
    valread = (int) read( sock , buffer, 1024);
    printf("%s\n",buffer );*/

    return 0;
}
