#include "base64_encoder-decoder.c"
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>

void rcv_ack(int socket_fd, struct sockaddr_in *client)
{
    char *ip = inet_ntoa(client->sin_addr);
    // Convert Internet number in IN to ASCII representation. The return value
    // is a pointer to an internal array containing the string.
    // i.e  the client IP address

    int port = client->sin_port;
    // Port of the client

    printf("\nNEW CLIENT CONNECTION [%s : %d] ESTABLISHED\n", ip, port);

    char buffer[1500]; //Buffer used to store,sending messages

    // Wait for message from client till close connection request is recieved
    while (1)
    {
        bzero(buffer, 1500);                    // Set the 1500 bytes of buffet as 0
        int st = read(socket_fd, buffer, 1500); // Read the message from the socket

        // If the message type is 3 then we have a close connection request.
        if (buffer[0] == '3')
            break;

        if (buffer[0] == '1')
        {
            printf("\nMessage received from client %s : %d\n\tEncoded Message: %s\n", ip, port, buffer + 1); // Print the encoded message
            printf("\tDecoded Message: %s\n", decoder(buffer + 1));                                           // Print the decoded message

            bzero(buffer, 1500);                      // Set the 1500 bytes of buffet as 0
            buffer[0] = '2';                          // Message type 2 refers to acknowledgement by the server
            strcpy(buffer + 1, encoder("ACK"));        // Append the encoded value of "ACK" after the message type
            write(socket_fd, buffer, strlen(buffer)); // Send the acknowledgement
        }
        else
            break;
    }

    close(socket_fd); // Close the  connection
    printf("\nCLIENT CONNECTION [%s : %d] CLOSED\n", ip, port);
    exit(0);
}

int main(int argc, char *argv[])
{
    int server_socket = socket(AF_INET, SOCK_STREAM, 0); //AF_NET is for IPv4, SOCK_STREAM indicates that TCP socket is created and 0 is for IP

    // Unable to create the server socket
    if (server_socket == -1)
    {
        printf("SERVER SOCKET CREATION FAILED\n");
        exit(0);
    }

    struct sockaddr_in server, client;

    server.sin_family = AF_INET;                   // code for address family - IPv4
    server.sin_addr.s_addr = INADDR_ANY;           // Address of host set to accept any incoming messages. Binds the socket to all available local interfaces
    server.sin_port = htons(atoi(argv[1]));        // Endianness of server port number changed
    memset(&server.sin_zero, 8, 0);                // Pad to size of `struct sockaddr' i.e a array of size 8.
    socklen_t length = sizeof(struct sockaddr_in); // Length of the server socket

    //If the server port is already in use then the binding fails
    if (bind(server_socket, (struct sockaddr *)&server, length) < 0)
    {
        printf("\nBINDING FAILED\n");
        exit(0);
    }

    // Prepare to accept connections on server socket.
    // 25 connection requests will be queued before further requests are refused.
    // Returns 0 on success, -1 for errors.

    if (listen(server_socket, 25) == -1)
    {
        printf("\nLISTEN FAILED\n");
        exit(0);
    }
    printf("SERVER WORKING\n");

    fflush(stdout); // This function is a possible cancellation point and therefore not marked with __THROW.

    while (1)
    {
        int client_socket = accept(server_socket, (struct sockaddr *)&client, &length); // Client tries to connect to the socket

        // Server client connection could not be made
        if (client_socket < 0)
        {
            printf("\nSERVER-CLIENT CONNECTION COULD NOT BE ESTABLISHED\n");
            exit(0);
        }

        int concurrent = fork();
        // Fork used to create a child process to handle the client
        // Hence multiple clients can be handled concurrently.

        if (concurrent == 0)
        {
            close(server_socket);            // Server socket is handled by parent process
            rcv_ack(client_socket, &client); // Used to handle a client
        }
        else if (concurrent == 0)
        {
            // error while creating child process
            printf("\nCOULDN'T ESTABLISH CONNECTION\n");
        }
        else
            close(client_socket); // Client socket handled by a child process
    }

    return 0;
}
