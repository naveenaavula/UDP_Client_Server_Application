// Standard libraries added
#include <stdio.h>      //Prototypes of standard input, output functions
#include <stdlib.h>     //To use functions like bzero, exit, atoi etc
#include <string.h>     //Contains useful string functions
#include <sys/types.h>  //Contains other headers and number of basic derived types
#include <sys/socket.h> //Contains data definition and socket structure
#include <netinet/in.h> //Contains internet family protocol definitions
#include <arpa/inet.h>  //Containts internet operation definitions, used for converting internet addresses to format needed
#include <sys/time.h>   //Contains function definition to obtain and update date and time information
#include <unistd.h>     //Included to use timer

//We define packet header, trailer, acknoledgement and packet reject in both server and client so that recieved data is correctly interpreted and printed
// Required data structures declaration

//Data structure that stores data packet header
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
    printf( "\n--------Acknowledgement from Server for packet%d--------\n" , acknowledge.segment_No);
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

// error generation function in case of errors
void error(char *msg)
{
    perror(msg);
    exit(1);
}

char buffer_payload[255];   //stores payload received from the client
char packet[1024];          //stores the entire data packet received from the client
char server_reply[16];      //stores reply from server (acknowledgement/reject)

int main(int argc, char *argv[])
{
    //sleep(30);                    //used for  ack_timer demo
    int socket_fd;                       //socket file descriptor 
    int socket_length;                     //server address structure's length
    int b;                          //Bytes number received from client
    int count = 0;                  //counter to track received packet's sequence
    struct sockaddr_in server;      //socket address of server
    struct sockaddr_in from_clnt;  //socket address of client
    socklen_t from_clnt_len;       //socket address length

    //Arguments are ./server - where server program's compiler output is stored and port number (server will LISTEN)

    if (argc != 2)
    {
        fprintf(stderr, "please give command - ./server port_number in order deploy server\n");
        exit(1);
    }


    //Creation of socket
    socket_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (socket_fd < 0)
    {
        error("Error: Failed to create socket");
    }

    socket_length = sizeof(server);                    //server socket address length is stored
    memset(&server, 0,socket_length);                  //all bytes in server are set to zero until length "length"
    server.sin_family = AF_INET;                //Address family is set to IPV4
    server.sin_addr.s_addr = INADDR_ANY;        
    server.sin_port = htons(atoi(argv[1]));     // port number to big-endian network byte order conversion

    // socket binding to port number as mentioned by user and error generation in case of errors
    if (bind(socket_fd, (struct sockaddr *)&server, socket_length) < 0)
    {
        error("Error: Failure in binding socket");
    }
    from_clnt_len = sizeof(struct sockaddr_in);

    //Server deployment so it can listen to client
    printf( "Server deployment is completed and no errors to report\n" );       
    while (1)                                                           //Condition is always true
    {
        b = recvfrom(socket_fd, packet, 1024, 0, (struct sockaddr *)&from_clnt, &from_clnt_len);
        if (b < 0)
        {
            error("Error while receiving data packet");
        }
        
        memcpy(&header, packet, sizeof(header));                                       //Header copy from the packet received from packet buffer 
        memcpy(&buffer_payload, packet + sizeof(header), b - sizeof(header) - 2);      //payload copy from received packet
        buffer_payload[strlen(buffer_payload)] = '\0';
        memcpy(&trailer, packet + b - 2, 2);                                           //Copies the trailer from the received packet from "packet" buffer

        //Printing received packet
        printf("\n\nDatapacket received from client\n\n");
        printf("Sequence number of data packet: %d \n", header.segment_No);
        printf("Size of data packet: %d\n", b);
        printf("Content: %s", buffer_payload);
        print_datapacket();
        printf("\n\n");

        //Analysis of packets and errors generation as given in assignmnet
        if (header.segment_No == count)
        {
            //Producing duplicate packet error
            printf( "Server:Rejecting the packet.  \nError: Duplicate Packet\n");
            reject_generator(0xFFF7);                             
            print_rejected();                                   
            memset(server_reply, 0, 16);                        //To store reject message, fill server_reply byte with zeroes    
            memcpy(server_reply, &reject, sizeof(reject));       
            b = sendto(socket_fd, &server_reply, sizeof(reject), 0, (struct sockaddr *)&from_clnt, from_clnt_len); //Sending to client
            if (b < 0)
            {
                error( "Server: Failed to send an acknowledgement to server.\n" );
            }
        }

        else if (header.segment_No != count + 1)
        {
            //Producing data packet not in sequence error
            printf( "Server: Rejecting the packet. \nError: Packet out of sequence\n" ); 
            reject_generator(0xFFF4);                            
            print_rejected();                                  
            memset(server_reply, 0, 16);                         
            memcpy(server_reply, &reject, sizeof(reject));      
            b = sendto(socket_fd, &server_reply, sizeof(reject), 0, (struct sockaddr *)&from_clnt, from_clnt_len); 
            if (b < 0)
            {
                error( "Server: Failed to send an acknowledgement to server.\n" );
            }
            
        }
        else if (header.length != strlen(buffer_payload))
        {
            //Length mismatch in packet error
            printf( "Server: Rejecting the packet.  \nError: Length mismatch\n" );
            reject_generator(0xFFF5);                             
            print_rejected();                                   
            memset(server_reply, 0, 16);                         
            memcpy(server_reply, &reject, sizeof(reject));       
            b = sendto(socket_fd, &server_reply, sizeof(reject), 0, (struct sockaddr *)&from_clnt, from_clnt_len); 
            if (b < 0)
            {
                error( "Server: Failed to send an acknowledgement to server.\n" );
            }
            
        }
        else if (trailer.end_of_packet_ID != 0xFFFF)
        {
            //Producing missing end of packet error
            printf( "Server: Rejecting the packet. \nError: End of packet missing\n");
            reject_generator(0xFFF6);                             
            print_rejected();                                   
            memset(server_reply, 0, 16);                        
            memcpy(server_reply, &reject, sizeof(reject));       
            b = sendto(socket_fd,&server_reply,sizeof(reject),0,(struct sockaddr*)&from_clnt,from_clnt_len);   
            if (b < 0){
                error( "Server: Failed to send an acknowledgement to server.\n" );
            }
        
        }
        else
        {

            //If data packet is accepted
            printf( "Server: Data packet is accepted. Sending an acknowledgement to client.\n" );
            count++;                                                        //maintaining this variable to send 5 data packets
            acknowledge.start_of_packetID = 0xFFFF;                            
            acknowledge.client_ID = header.client_ID;                       
            acknowledge.ack = 0xFFF2;                                       
            acknowledge.segment_No = header.segment_No;                             
            acknowledge.end_of_packet_ID = 0xFFFF;                                    
            printf("Size of Acknowledgement: %lu Bytes\n", sizeof(acknowledge));
            print_ack();                                                    
            memset(server_reply, 0, 16);                                    
            memcpy(server_reply, &acknowledge, sizeof(acknowledge));        
            b = sendto(socket_fd, &server_reply, sizeof(acknowledge), 0, (struct sockaddr *)&from_clnt, from_clnt_len);    
            if (b < 0)
            {
                error( "Server: Failed to send an acknowledgement to client.\n" );
            }
            //resetting counter after 5 packets are received successfully to error messages testing
            if (count == 5)
            {
                printf("\n\n\n\nResetting the counter to test other cases\n\n\n\n");
                count = 0;
            }
        }
    }
}