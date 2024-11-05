// Standard libraries added
#include <stdio.h>      //Prototypes of standard input, output functions
#include <stdlib.h>     //To use functions like bzero, exit, atoi etc
#include <string.h>     //Contains useful string functions
#include <sys/types.h>  //Contains other headers and number of basic derived types
#include <sys/socket.h> //Contains data definition and socket structure
#include <netinet/in.h> //Contains internet family protocol definitions
#include <arpa/inet.h>  //Containts internet operation definitions, used for converting internet addresses to format needed
#include <sys/time.h>   //Contains function definition to obtain and update date and time information
#include <netdb.h>      //Used for resolution of address and hostname


//We define packet header, trailer, acknoledgement and packet reject in both server and client so that recieved data is correctly interpreted and printed
// Required data structures declaration

//Data structure that stores data packet header

//Data structure to store header
struct header_packet
{
    unsigned short int start_of_packetID;
    unsigned char client_ID;
    unsigned short int data;
    unsigned char segment_No;
    unsigned char length;
} header = {0xFFFF, 0xFF, 0xFFF1, 0x0, 0xFF};

//Data structure that stores data packet trailer
struct trailer_packet
{
    unsigned short int end_of_packet_ID;
} trailer = {0xFFFF};

//Data structure that stores ACK(Acknowledge) packet
struct ACK_packet
{
    unsigned short int start_of_packetID;
    unsigned char client_ID;
    unsigned short int ack;
    unsigned char segment_No;
    unsigned short int end_of_packet_ID;
} acknowledge = {0xFFFF, 0xFF, 0xFFF2, 0x0, 0xFFFF};

//Data structure that stores Reject packet
struct reject_packet
{
    unsigned short int start_of_packetID;
    unsigned char client_ID;
    unsigned short int reject;
    unsigned short int rejectsubCode;
    unsigned char segment_No;
    unsigned short int end_of_packet_ID;
} reject = {0xFFFF, 0xFF, 0xFFF3, 0xFFF4, 0x0, 0xFFFF};

// code block for printing details of received packet
void print_datapacket()
{
    printf( "\n---------Data Packet Number %d---------\n" ,header.segment_No);
    printf("Start of Packet ID \t\t: \"%#X\"\n", header.start_of_packetID);
    printf("Client ID \t\t\t: \"%#X\"\n", header.client_ID);
    printf("DATA \t\t\t\t: \"%#X\"\n", header.data);
    printf("Segment Number \t\t\t: \"%d\"\n", header.segment_No);
    printf("Length of payload \t\t: \"%#X\"\n", header.length);
    printf("End of Packet ID \t\t: \"%#X\"\n", trailer.end_of_packet_ID);
    printf( "--------------------------------------\n\n\n\n" );
}

// code block for printing acknowledgement
void print_ack()
{
    printf( "\n--------Acknowledgement from Server for packet %d--------\n" , acknowledge.segment_No);
    printf("Start of Packet ID \t\t: \"%#X\"\n", acknowledge.start_of_packetID);
    printf("Client ID \t\t\t: \"%#X\"\n", acknowledge.client_ID);
    printf("ACK \t\t\t\t: \"%#X\"\n", acknowledge.ack);
    printf("Received Segment Number \t: \"%d\"\n", acknowledge.segment_No);
    printf("End of Packet ID \t\t: \"%#X\"\n", acknowledge.end_of_packet_ID);
    printf( "--------------------------------------\n\n\n" );
}

// code block for printing rejected packets
void print_rejected()
{
    printf( "\n---------Reject Packet %d---------\n" ,reject.segment_No);
    printf("Start of Packet ID \t\t: \"%#X\"\n", reject.start_of_packetID);
    printf("Client ID \t\t\t: \"%#X\"\n", reject.client_ID);
    printf("REJECT \t\t\t\t: \"%#X\"\n", reject.reject);
    printf("Reject sub code \t\t: \"%#X\"\n", reject.rejectsubCode);
    printf("Received Segment Number \t: \"%d\"\n", reject.segment_No);
    printf("End of Packet ID \t\t: \"%#X\"\n", reject.end_of_packet_ID);
    printf( "--------------------------------------\n\n\n" );
}

// Reject packets generation to send to client when there is error
void reject_generator(unsigned short int code)
{
    reject.start_of_packetID = 0xFFFF;
    reject.client_ID = header.client_ID;
    reject.reject = 0xFFF3;
    reject.rejectsubCode = code;
    reject.segment_No = header.segment_No;
    reject.end_of_packet_ID = 0xFFFF;
}

// Function to generate errors,if any
void error(char *msg)
{
    perror(msg);
    exit(1);
}

char packet[1024];  
char server_reply[16];                            
int socket_fd;                       //socket file descriptor
int b;                              //Bytes number sent from client
int packet_number;                     //used to test cases as per instructions
int counter;                          //counter for monitoring client attempts to send data packet to server
struct sockaddr_in server;          //socket address of server
struct sockaddr_in from_clnt;      //socket address of client
struct hostent *host_inf;          //Host info
socklen_t length;                   //from_clnt socket address structure length
char payload[256];                  //Payload
struct timeval time_out;             //used for ack_timer


// generate packets we need to send to server
void packet_generator(int seg, char *payload)
{
    header.segment_No = seg;                                                //segment number
    header.length = strlen(payload);                                    //payload length             
    memset(packet, 0, 264);                                             //all bytes in packet from length 0 to 264 are set as zeroes
    memcpy(packet, &header, sizeof(header));                            //to copy header in packet buffer
    memcpy(packet + sizeof(header), payload, strlen(payload));          //to copy payload in packet buffer
    memcpy(packet + sizeof(header) + strlen(payload), &trailer, 2);     //to copy trailer in packet buffer
    printf("Packet creation completed:");                                        
    print_datapacket();                                                     
}

// Function to send data packet to server
void send_packet()
{
    //Maximum 3 retries to send data packets to server
    counter = 3;
    while (counter >= 0)
    {
        // First attempt to send data packets
        if (counter == 3)
        {
            printf("\nPacket transfer is in progress: %s\n", payload);
        }
        
        b = sendto(socket_fd, packet, sizeof(header) + strlen(payload) + 2, 0, (struct sockaddr *)&server, length);

        // Prints in case of error
        if (b < 0)
        {
            error("Client: Failed to transfer data packet to server\n");
        }

        bzero(server_reply, 16);                                                            
        b = recvfrom(socket_fd, server_reply, 16, 0, (struct sockaddr *)&from_clnt, &length);   //receiving server response

        // If response is received successfully, copy it into server_reply and compare the acknowledgement and print
        // If rejected, copies it and prints the error  
        if (b >= 0) 
        {
            memcpy(&acknowledge, server_reply, sizeof(acknowledge));        
            if (acknowledge.ack == 0xFFF2)
            {
                printf( "Received acknowledgement\n " );
                print_ack();
                break;
            }
            else
            {
                memcpy(&reject, server_reply, sizeof(reject));
                printf( "Packet rejected\n" );
                printf( "Error: " );
                if (reject.rejectsubCode == 0xFFF4)
                {
                    printf( "Received data packet not in sequence\n");
                }
                else if (reject.rejectsubCode == 0xFFF5)
                {
                    printf( "Length mismatch in received data packet\n");
                }
                else if (reject.rejectsubCode == 0xFFF6)
                {
                    printf( "Missing End of data packet\n");
                }
                else if (reject.rejectsubCode == 0xFFF7)
                {
                    printf( "Duplicate packet\n");
                }
                else
                {
                    printf( "Unknown error\n");
                }
                print_rejected();
                counter++;
                break;
            }
        }

        // Printing retry message
        else
        {
            if(counter>0){
            printf("Error: Server didn't respond after 3 seconds. Trying to transfer again\n");
            }
        }
        counter--;
    }

    // Meassage to print if server didn't respond after 3 tries
    if (counter < 0)
    {
        error( "Error: Server didn't respond in three attempts\n" );
    }
}
int main(int argc, char *argv[])
{

    //Arguments are ./client - where client program's compiler output is stored, local host and port number (server will LISTEN)
    if (argc != 3)
    {
        printf("please give command: ./client server_name port_number(localhost 8080)\n");
        exit(1);
    }

    //Creation of socket at client
    socket_fd = socket(AF_INET, SOCK_DGRAM, 0);

    if (socket_fd < 0)
    {
        error("Error: Failed to create socket.\n");
    }

    server.sin_family = AF_INET;                //Address family is set to IPV4
    host_inf = gethostbyname(argv[1]);         //Hostname from user

    if (host_inf == 0)
    {
        error("Client: Unknown host\n");
    }

    bcopy((char *)host_inf->h_addr, (char *)&server.sin_addr, host_inf->h_length);        //host IP address copy to server to establish connection
    server.sin_port = htons(atoi(argv[2]));             //host port number to big-endian network byte order conversio
    length = sizeof(struct sockaddr_in);

    //Setting ACK timer to 3 seconds
    time_out.tv_sec = 3;
    time_out.tv_usec = 0;
    setsockopt(socket_fd, SOL_SOCKET, SO_RCVTIMEO, (char *)&time_out, sizeof(time_out));

    printf( "\n\nTest Case 1: Transfer 5 data packets to server without any errors and receive ACK\n\n" );
    packet_number = 1;

    while(packet_number <= 5){
        //Creation of packet
        bzero(payload, 256);                        
        memcpy(payload, "Payload\n", 34);           //Copy Payload in payload buffer
        packet_generator(packet_number, payload);        //Packet generation function call
        send_packet();                              //transfer data packets to server
        packet_number++;                               //counter for 5 data packets creation 
    }
    printf( "\n\n\n Packet counter reset to test error cases\n\n\n" );

    printf( "\n\nTest Case 2: Transfer 1 error free data packet and 4 data packets with errors given in assignment\n\n" );

    //Packet 1 - no error
    printf( "Packet 1: Error free packet\n" );
    bzero(payload, 256);
    memcpy(payload, "Payload\n", 34);
    packet_generator(1, payload);
    send_packet();

    //Packet 2 - duplicate packet error
    printf( "Packet 2: Producing 'Duplicate Packet' error\n" );
    bzero(payload, 256);
    memcpy(payload, "Payload\n", 34);
    packet_generator(1, payload);
    send_packet();

    //Packet 3 - out of sequence data packet
    printf( "Packet 3: Producing 'Out of Sequence' error\n" );
    bzero(payload, 256);
    memcpy(payload, "Payload\n", 34);
    packet_generator(3, payload);
    send_packet();

    //Packet 4 - data packet length mismatch
    printf( "Packet 4: Producing 'Length mismatch' error\n" );
    bzero(payload, 256);
    memcpy(payload, "Payload\n", 34);
    header.segment_No = 2;
    header.length = 0x44;
    memset(packet, 0, 264);
    memcpy(packet, &header, sizeof(header));
    memcpy(packet + sizeof(header), payload, strlen(payload));
    memcpy(packet + sizeof(header) + strlen(payload), &trailer, 2);
    printf("Packet creation completed");
    print_datapacket();
    send_packet();

    //Packet 5 - end of data packet missing
    printf( "Packet 5: Producing  'End of packet missing' error\n" );
    bzero(payload, 256);
    memcpy(payload, "Payload\n", 34);
    packet_generator(2, payload);
    trailer.end_of_packet_ID = 0xFFF0;
    memcpy(packet + sizeof(header) + strlen(payload), &trailer, 2);
    printf( "Changed the end of packet" );
    print_datapacket();
    send_packet();
}
