#include "base64_encoder-decoder.c"
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>

void connection_close(char buffer[], int client_socket)
{
    buffer[0] = '3';                                  // Type 3 message as it is used for closing the connection
    strcpy(buffer + 1, encoder("connection_close"));  // Append encoded value of "c" after the type of the message
    printf("CONNCECTION CLOSED\n");                   // Message on console to be printed
    write(client_socket, buffer, strlen(buffer));     // Write in client socket
}

void read_message(char buffer[])
{
    char input;           // Used to read user input character by character
    bzero(buffer, 1500);  // Used to set the N=1500 bytes of buffer to 0 

    printf("Enter the message to be sent\n");
    int i = 0;

    while (1)
    {
        // Store the message in the buffer, message size should not exceed 1000.
        scanf("%c", &input); 
        if (input == '\n')
            break;

        if (i == 1000)
        {
            // Maximum input size is 1000 characters
            printf("The first 1000 characters of message are being sent\n");
            break;
        }
        buffer[i++] = input;
    }
}

int main(int argc, char *argv[])
{
    char input;                                            // Used to read user input character by character
    int client_socket = socket(AF_INET, SOCK_STREAM, 0);   // AF_NET is for IPv4, SOCK_STREAM indicates that TCP socket is created ans 0 for IP
    char buffer[1500];                                     // Buffer to store recieving and modified sending value

    // Client Socket could not be created
    if (client_socket == -1)
    {
        printf("CLIENT SOCKET CREATION FAILED\n");
        return 0;
    }

    struct sockaddr_in server;      // Structure describing an Internet socket address.
    server.sin_family = AF_INET;    // Address family - IPv4

    // Server IP address which is inputted by user
    if (inet_aton(argv[1], &server.sin_addr) == 0)
    {
        // inet_aton converts Internet host address from numbers-and-dots notation in CP
        // into binary data and store the result in the structure INP.
        printf("SERVER IP ADDRESS ERROR\n");
        return 0;
    }

    int server_port = atoi(argv[2]);                // Server port given as argument by user
    server.sin_port = htons(server_port);           // htons changes the endianness of 2 bytes.
    socklen_t length = sizeof(struct sockaddr_in);  // Size of socket

    // Failure to establish a connection
    if (connect(client_socket, (struct sockaddr *)&server, length) == -1)
    {
        // connect function is used to 
        // Open a connection on socket client_socket to peer at server address (which is length bytes long).
        printf("COULD NOT CONNECT TO THE SERVER\n");
        exit(0);
    }

    while (1)
    {
        printf("Send message?\nPress 'y' for YES or any other key for NO and press enter: \n");

        scanf("%c", &input);
        if (input != 'y')
        {
            connection_close(buffer, client_socket);
            break;
        }

        getchar();                                      // Read a character from standard input.This function is a possible cancellation point
        read_message(buffer);                           // Function defined above i.e to read the message
        strcpy(buffer + 1, encoder(buffer));            // Append the encoded string/message after the msg type
        buffer[0] = '1';                                // Set the type of the message as 1
        write(client_socket, buffer, strlen(buffer));   // Write to the client socket
        bzero(buffer, 1500);                            // Used to set the N=1500 bytes of buffer to 0 

        int received = read(client_socket, buffer, 50); // Recieve (ACK) acknowledgement from  the server
        strcpy(buffer + 1, decoder(buffer + 1));        // Decode the message

        // Type 2 message means an acknowledgement from the server
        if (buffer[0] != '2')
        {
            printf("Acknowledgement could not received.\nPlease resend the message.\n");
            continue;
        }

        printf("Recieved a message from server having IP %s and port %d\n", argv[1], server_port);
        printf("%s\n", buffer + 1);
    }

    close(client_socket);  // Close the client socket

    return 0;
}
