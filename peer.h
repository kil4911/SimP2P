//
// Created by kdl4u_000 on 12/8/2018.
//


#include <netdb.h>
#include <errno.h>
#include "ip.h"

#define PORT 12000
#define ID_LEN 20

typedef struct peer {
    int my_id;
    struct sockaddr_in my_sock_addr;
    char *my_ip;
    int max_peers;
    int num_peers;
    char *peers[];
    int my_sock_fd;
}peer;

peer *createPeer(int id, char *my_ip, int maxPeers) {
    peer * newPeer = (peer *) malloc(sizeof(peer));
    newPeer->my_id = id;
    newPeer->my_ip = my_ip;
    newPeer->num_peers = 0;
    newPeer->max_peers = maxPeers;
    newPeer-> peers = (char **) malloc(maxPeers * sizeof(char*)); //I am included in this list at index my_id

    for (int i = 0; i < maxPeers; i++)
        newPeer->peers[i] = malloc((ID_LEN+1) * sizeof(char));

    newPeer->my_sock_fd = 0;
    return newPeer;
}

void chooseLeader(peer *myPeer) {
    char *Leader = myPeer->peers[0];

    for (int i= 0; i < myPeer->max_peers - 1; i++) {
        if (ipComp(myPeer->peers[i], myPeer->peers[i+1]) == 0 ) {
            Leader = myPeer->peers[i+1];
        }
    }
}

void addPeer(peer *myPeer, char *toAdd, int index) {
    if (myPeer->num_peers < myPeer->max_peers) {
        myPeer->peers[index] = toAdd;
        myPeer->num_peers++;
    }
}

int create_loc_sock(peer *myPeer) {
    // Create a local socket for communication
    if ((myPeer->my_sock_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        printf("\n Socket creation error \n");
        return -1;
    }
}

int conn_2_serv(peer *myPeer, peer *otherPeer) {
    // Setting the server address to 0's.
    memset(&otherPeer->my_sock_addr, '0', sizeof(otherPeer->my_sock_addr));

    otherPeer->my_sock_addr.sin_family = AF_INET; 	// Address family
    otherPeer->my_sock_addr.sin_port = htons(PORT); // Server port in network byte order

    // Convert IPv4 and IPv6 addresses from text to binary form
    if(inet_pton(AF_INET, otherPeer->my_ip, &otherPeer->my_sock_addr.sin_addr)<=0)
    {
        printf("\nInvalid address/ Address not supported \n");
        return -1;
    }

    // Initiate a connection request to the server.
    if (connect(myPeer->my_sock_fd, (struct sockaddr *)&otherPeer->my_sock_addr, sizeof(otherPeer->my_sock_addr)) < 0)
    {
        printf("\nConnection Failed \n");
        return -1;
    }

    return 0;
}

void send_msg(peer *myPeer, peer *otherPeer,char* msg, char* msg_type) {
    // Sending message to the server.
    size_t msg_size = strlen(msg);
    if( send(myPeer->my_sock_fd, msg, msg_size, 0) != msg_size )
        perror("send");

    if (otherPeer != NULL) {
        if (strcmp(msg_type, "hello") == 0)
            printf("(Hello) from me(%s) to (%s)\n", myPeer->my_ip, otherPeer->my_ip);
        else
            printf("Msg sent from me(%s) to (%s)", myPeer->my_ip, otherPeer->my_ip);
    }
}

void recv_reply(peer *myPeer) {
    char buffer[1024] = {0};
    // Reading reply from the server.
    int valread = (int) read(myPeer->my_sock_fd , buffer, 1024);
    printf("%s\n",buffer );
}

void destroy_peer(peer *myPeer){
    for (int i = 0; i < myPeer->max_peers; i++) {
        free(myPeer->peers[i]);
    }
    free(myPeer->peers);
    free(myPeer);
}

int add_socks_2_set(peer *myPeer, fd_set readfds, const int *client_sockets) {
    int i, client_sd;

    //add master socket to set
    FD_SET(myPeer->my_sock_fd, &readfds);
    int max_sd = myPeer->my_sock_fd;

    //add child sockets to set
    for ( i = 0 ; i < myPeer->max_peers ; i++)
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

    return max_sd;
}

void wait_for_activity(peer *myPeer, int max_sd, fd_set readfds, int *client_sockets, int addrlen) {
    //wait for an activity on one of the sockets , timeout is NULL ,
    //so wait indefinitely
    int activity = select( max_sd + 1 , &readfds , NULL , NULL , NULL);
    struct sockaddr_in address;

    if ((activity < 0) && (errno!=EINTR))
    {
        printf("select error");
    }

    int new_socket;

    //If something happened on the master socket ,
    //then its an incoming connection
    if (FD_ISSET(myPeer->my_sock_fd, &readfds)) {
        if ((new_socket = accept(myPeer->my_sock_fd,
                                 (struct sockaddr *)&address, (socklen_t*)&addrlen))<0)
        {
            perror("accept");
            exit(EXIT_FAILURE);
        }

        //inform user of socket number - used in send and receive commands
        printf("New connection , socket fd is %d , ip is : %s , port : %d\n",
               new_socket , inet_ntoa(address.sin_addr) , ntohs( address.sin_port));

        //send new connection greeting message
        char *hello = "Hello from server ";
        char * hello_msg = concat(hello, getMyAddress());
        size_t msg_size = strlen(hello_msg);

        send_msg(myPeer, NULL, hello_msg, "hello");

        free(hello_msg);

        puts("Welcome message sent successfully");

        int i = 0;
        //add new socket to array of sockets
        for (i = 0; i < myPeer->max_peers; i++)
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

}

void do_IO(peer *myPeer, fd_set readfds, int *client_sockets, int addrlen) {
    int i = 0, client_sd, valread;
    char buffer[1024] = {0};
    struct sockaddr_in address;

    for (i = 0; i < myPeer->max_peers; i++) {
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

void accept_connections(peer *myPeer, int *client_sockets) {
    int addrlen = sizeof(myPeer->my_sock_addr);
    fd_set readfds; //set of socket descriptors
    int max_sd;

    while (1) {
        //clear the socket set
        FD_ZERO(&readfds);
        max_sd = add_socks_2_set(myPeer, readfds, client_sockets);
        wait_for_activity(myPeer, max_sd, readfds, client_sockets, addrlen);
        do_IO(myPeer, readfds, client_sockets, addrlen);
        break;
    }
}

void set_me_up_as_server(peer *myPeer) {
    int opt = 1;
    int i;

    int client_sockets[myPeer->max_peers];

    //initialise all client_socket[] to 0 so not checked
    for (i = 0; i < myPeer->max_peers; i++)
    {
        client_sockets[i] = 0;
    }

    // Creating (master) socket file descriptor
    create_loc_sock(myPeer);

    // Setting socket options (allows multiple connections)
    if (setsockopt(myPeer->my_sock_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)))
    {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }

    myPeer->my_sock_addr.sin_family = AF_INET;
    myPeer->my_sock_addr.sin_addr.s_addr = INADDR_ANY; // Making this socket available to all available interfaces.
    myPeer->my_sock_addr.sin_port = htons(PORT); // Attaching the socket to defined PORT

    // Forcefully attaching socket to the PORT
    if (bind(myPeer->my_sock_fd, (struct sockaddr *)&myPeer->my_sock_addr, sizeof(myPeer->my_sock_addr))<0)
    {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    // Start listening for incoming connections. This would make this socket a passive socket for
    // accepting the connections. 3 specifies the maximum length of queue for pending connections.
    // If this queue is full, then the client may receive an error.
    if (listen(myPeer->my_sock_fd, 3) < 0)
    {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    // Accepting the incoming connection.
    printf("(%s) is waiting for connections ...", myPeer->my_ip);
    accept_connections(myPeer, client_sockets);
}
