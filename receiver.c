#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <stdbool.h> 

// receiver = server

// global variables
int BUFFSIZE = 500;


// functions
int recvNoOfPackets(int sockfd, struct sockaddr_in sender, int len);
void recvPackets(int sockfd, struct sockaddr_in sender, int len, int noOfPackets,char buffer[6][BUFFSIZE], bool *endOfTransmission, bool *Ack, int *length);
void resetData(char buffer[6][BUFFSIZE], bool *Ack);
void sendAck(int sockfd, struct sockaddr_in sender, int len, bool *Ack);
int packetsMissing(int noOfPackets, bool *Ack); 
int findMissingPackets(int *missingPackets, bool *Ack, int noOfPackets);
void recvMissingPackets(int sockfd, struct sockaddr_in sender, int len,int *missingPackets,char buffer[6][BUFFSIZE],int noOfPackets, bool *endOfTransmission, bool *Ack, int *length);
void writeToFile(FILE *fp, char buffer[6][BUFFSIZE], int noOfPackets, int *length);


// main function
int main(int argc, char *argv[])
{   
    // ensure no of args
    if(argc != 2){
        perror("Port Not Specified\n");
        exit(1);
    }

    // variables
    int noOfPackets = 0;
    int port = atoi(argv[1]);
    int sockfd;
    struct sockaddr_in receiver, sender;
    char buffer[6][BUFFSIZE];
    char filename[BUFFSIZE];
    char tempFilename[BUFFSIZE];
    bool Ack[5];
    bool endOfTransmission = false;
    int length[6];

    // create socket
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if(sockfd < 0){
        perror("Socket could not be created\n");
        exit(1);
    }

    // set addresses to null
    memset(&sender, 0, sizeof(sender));
    memset(&receiver, 0, sizeof(receiver));

    // create address
    receiver.sin_family = AF_INET;
    receiver.sin_port = htons(port);
    receiver.sin_addr.s_addr = inet_addr("127.0.0.1");

    // bind port
    if(bind(sockfd, (struct sockaddr *)&receiver, sizeof(receiver))){
        perror("Bing error\n");
        exit(1);
    }

    // get file name
    strcpy(filename,"recv_");
    int len = sizeof(sender);
    if (recvfrom(sockfd, tempFilename, BUFFSIZE, 0, (struct sockaddr *) &sender, &len) == -1)
    {
        perror("Can't receive filename");
        exit(1);
    }
    strcat(filename, tempFilename);

    // open file
    FILE *fp = fopen(filename, "wb");
    if (fp == NULL)
    {
        perror("Can't open file");
        exit(1);
    }

    // recv file
    while(1){
        noOfPackets = recvNoOfPackets(sockfd, sender, len);
        recvPackets(sockfd, sender, len, noOfPackets, buffer, &endOfTransmission, Ack, length);
        sendAck(sockfd, sender, len, Ack);

        while(packetsMissing(noOfPackets, Ack)){
            recvMissingPackets(sockfd, sender, len, missingPackets, buffer, noOfPackets, &endOfTransmission, Ack, length);
            sendAck(sockfd, sender, len, Ack);
        }

        writeToFile(fp, buffer, noOfPackets, length);
        resetData(buffer, Ack);

        if(endOfTransmission){
            break;
        }
        
        
        
        

    }

    printf("\nFile transfer complete.\n");
    printf("Closing socket....\n");
    close(sockfd);
    fclose(fp);
    printf("Terminating connection...\n");
    memset(&sender, 0, sizeof(sender));
    memset(&receiver, 0, sizeof(receiver));
    printf("Connection terminated.\n");

    return 0;
}

// receive no of packets
int recvNoOfPackets(int sockfd, struct sockaddr_in sender, int len){

    char no_of_packets[5];
    memset(&no_of_packets, 0, sizeof(no_of_packets));
    recvfrom(sockfd,no_of_packets, sizeof(no_of_packets), 0,(struct sockaddr *)&sender, &len);
    printf("No. of packets = %c\n", no_of_packets[0]);
    return no_of_packets[0] - '0';

}

// recv packets
void recvPackets(int sockfd, struct sockaddr_in sender,int len, int noOfPackets,char buffer[6][BUFFSIZE], bool *endOfTransmission, bool *Ack, int *length){

    int packetNo = 0;
    printf("Receiving packets...\n");

    for(int i=0; i<=noOfPackets; i++){
        
        length[i] = recvfrom(sockfd,buffer[i],BUFFSIZE, 0, (struct sockaddr *)&sender, &len);
        
        if(strcmp(buffer[i],"end of file") == 0){
            *endOfTransmission = true;
            break;
        }

        if(strcmp(buffer[i],"end") == 0){
            break;
        }


        
        packetNo = buffer[i][0] - '0'; // packet no.
        Ack[packetNo] = 1;	// generating ack for received packet

     
        

    }


    printf("Packets received.\n");


}


void resetData(char buffer[6][BUFFSIZE], bool *Ack){

    for(int i=0; i<6; i++){

        memset(&buffer[i], 0, sizeof(buffer[i]));

    } 
    memset(&Ack, 0, 5);

}






void sendAck(int sockfd, struct sockaddr_in sender, int len, bool *Ack){

    printf("Sending Ack...\n");
    sendto(sockfd, Ack, 5, 0, (struct sockaddr*)&sender, len);

}


int packetsMissing(int noOfPackets,bool *Ack){

    printf("Checking for missing packets...\n");
    for(int i=0; i<noOfPackets; i++){
        if(!Ack[i]){	// if ack of a packet = 0 then this statement will execute
            return 1;
        }
    }
    return 0;

}

int findMissingPackets(int *missingPackets, bool *Ack, int noOfPackets){

    int noOfMissingPackets = 0;
    memset(&missingPackets, 0, 20);

    printf("Finding missing packets\n");

    for(int i=0; i<noOfPackets; i++){
        if(!Ack[i]){		// find no of packets with ack = 0
            noOfMissingPackets += 1;
        }
    }

    return noOfMissingPackets;


}


void recvMissingPackets(int sockfd, struct sockaddr_in sender, int len,int *missingPackets,char buffer[6][BUFFSIZE],int noOfPackets, bool *endOfTransmission, bool *Ack, int *length){

    int noOfMissingPackets = 0;
    noOfMissingPackets = findMissingPackets(missingPackets, Ack , noOfPackets);
    printf("Receiving Missing Packets...\n");
    recvPackets(sockfd, sender, len, noOfMissingPackets, buffer, endOfTransmission, Ack, length);

}


void writeToFile(FILE *fp, char buffer[6][BUFFSIZE], int noOfPackets, int *length){
    
    int packetNo = 0;
	// writing packets to file according to sequence number
    printf("Writing to file...\n");
    for(int i=0; i<noOfPackets; i++){
        for(int j=0; j<noOfPackets; j++){
            packetNo = buffer[j][0] - '0';
            if(packetNo == i){
                if (fwrite(buffer[j] + 2, sizeof(char), (length[i]-2), fp) <= 0){
                        perror("Write File Error");
                        exit(1);
                    }
                    break;
                }
        }                
    }

}