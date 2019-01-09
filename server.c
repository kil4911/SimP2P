// Server side C/C++ program to demonstrate Socket programming
#include <unistd.h>
#include <stdio.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <string.h>
#include <arpa/inet.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/time.h> //FD_SET, FD_ISSET, FD_ZERO macros
#include <netdb.h>
#include <mpi.h>

#include "peer.h"

#define PORT 12000


int main(int argc, char **argv)
{
    MPI_Init(&argc, &argv);
    int myRank, nTasks;


    MPI_Comm_size(MPI_COMM_WORLD, &nTasks);
    MPI_Comm_rank(MPI_COMM_WORLD, &myRank);


    int server_fd, new_socket, valread;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);
    char buffer[1024] = {0};
    fd_set readfds; //set of socket descriptors
    int client_sd, max_sd, activity;

    int max_peers = 4; int i;

    int client_sockets[max_peers];

    peer *peer1 = createPeer(0, "", 3);
    // Creating (master) socket file descriptor
    create_loc_sock(peer1);
    set_me_up_as_server(peer1);


    //initialise all client_socket[] to 0 so not checked
    for (i = 0; i < max_peers; i++)
        client_sockets[i] = 0;




    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0)
    {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    // Setting socket options (allows multiple connections)
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)))
    {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY; // Making this socket available to all available interfaces.
    address.sin_port = htons(PORT); // Attaching the socket to defined PORT

    // Forcefully attaching socket to the PORT
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address))<0)
    {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    // Start listening for incoming connections. This would make this socket a passive socket for
    // accepting the connections. 3 specifies the maximum length of queue for pending connections.
    // If this queue is full, then the client may receive an error.
    if (listen(server_fd, 3) < 0)
    {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    // Accepting the incoming connection.
    puts("Waiting for connections ...");

    while(1) {
        //clear the socket set
        FD_ZERO(&readfds);

        //add master socket to set
        FD_SET(server_fd, &readfds);
        max_sd = server_fd;

        //add child sockets to set
        for ( i = 0 ; i < max_peers ; i++)
        {
            //socket descriptor
            client_sd = client_sockets[i];

            //if valid socket descriptor then add to read list
            if(client_sd > 0)
                FD_SET( client_sd , &readfds);

            //highest file descriptor number, need it for the select function
            if(client_sd > max_sd)
                max_sd = client_sd;
        }

        //wait for an activity on one of the sockets , timeout is NULL ,
        //so wait indefinitely
        activity = select( max_sd + 1 , &readfds , NULL , NULL , NULL);

        if ((activity < 0) && (errno!=EINTR))
        {
            printf("select error");
        }

        //If something happened on the master socket ,
        //then its an incoming connection
        if (FD_ISSET(server_fd, &readfds)) {
            if ((new_socket = accept(server_fd,
                                     (struct sockaddr *)&address, (socklen_t*)&addrlen))<0)
            {
                perror("accept");
                exit(EXIT_FAILURE);
            }

            //inform user of socket number - used in send and receive commands
            printf("New connection , socket fd is %d , ip is : %s , port : %d\n",
                   new_socket , inet_ntoa(address.sin_addr) , ntohs(address.sin_port));

            //send new connection greeting message
            char *hello = "Hello from server ";
            char * hello_msg = concat(hello, getMyAddress());
            size_t msg_size = strlen(hello_msg);

            if( send(new_socket, hello_msg, msg_size, 0) != msg_size )
            {
                perror("send");
            }

            free(hello_msg);

            puts("Welcome message sent successfully");

            //add new socket to array of sockets
            for (i = 0; i < max_peers; i++)
            {
                //if position is empty
                if( client_sockets[i] == 0 )
                {
                    client_sockets[i] = new_socket;
                    printf("Adding to list of sockets as %d\n" , i);

                    break;
                }
            }
        }

        //else its some IO operation on some other socket
        for (i = 0; i < max_peers; i++) {
            client_sd = client_sockets[i];

            if (FD_ISSET( client_sd , &readfds)) {
                //Check if it was for closing , and also read the
                //incoming message
                if ((valread = (int) read( client_sd , buffer, 1024)) == 0) {
                    //Somebody disconnected , get his details and print
                    getpeername(client_sd , (struct sockaddr*)&address , \
                        (socklen_t*)&addrlen);
                    printf("Host disconnected , ip %s , port %d \n" ,
                           inet_ntoa(address.sin_addr) , ntohs(address.sin_port));

                    //Close the socket and mark as 0 in list for reuse
                    close( client_sd );
                    client_sockets[i] = 0;
                }
                    //Echo back the message that came in
                else
                {
                    //set the string terminating NULL byte on the end
                    //of the data read
                    buffer[valread] = '\0';
                    send(client_sd , buffer , strlen(buffer) , 0 );
                }
            }
        }
    }

    MPI_Finalize();
}
