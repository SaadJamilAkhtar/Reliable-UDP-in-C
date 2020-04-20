#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libgen.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <stdbool.h>

// sender = client
// buffer represents window in below code.

// global variables
int BUFFSIZE = 500;
int noOfBuffers = 6;


//functions
void createPackets(FILE *fp, char buffer[noOfBuffers][BUFFSIZE], int *seqNo, bool *endOfFile, int *length);
void resetBuffers(char buffer[noOfBuffers][BUFFSIZE]);
void sendNoOfPackets(int sockfd, struct sockaddr_in receiver, int len, int seqNo);
void sendPackets(int sockfd, struct sockaddr_in receiver, int len, char buffer[noOfBuffers][BUFFSIZE], int seqNo, bool endOfFile, int *length);
void receiveAck(int sockfd, struct sockaddr_in receiver,int len,bool *Ack);
bool ackMissing(bool *Ack, int seqNo);
void sendMissingPackets(int sockfd, struct sockaddr_in receiver, int len, int seqNo, bool *Ack, char buffer[noOfBuffers][BUFFSIZE]);
void resetData(int *seqNo, bool *Ack);


// main function
int main(int argc, char* argv[]){

    // ensure no of args
    if(argc < 3){
        perror("Invalid no. of args\n");
        exit(1);
    }

    //variable declaration
    int noOfPackets = 0;
    int port = atoi(argv[2]);
    int sockfd;
    struct sockaddr_in receiver;
    char buffer[noOfBuffers][BUFFSIZE];
    char tempFilename[BUFFSIZE];
    bool Ack[5];
    int missingPackets[5];
    int seqNo = 0;
    int len;
    bool endOfFile = false;
    int length[5];
    char buff[BUFFSIZE];


    // create socket
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if(sockfd < 0){
        perror("Socket could not be created\n");
        exit(1);
    }


    // set variables to null
    memset(&receiver, 0, sizeof(receiver));
    memset(&Ack, 0, 5);
    memset(&buff, 0, sizeof(buff));

    // create address
    receiver.sin_family = AF_INET;
    receiver.sin_port = htons(port);
    receiver.sin_addr.s_addr = inet_addr("127.0.0.1");
    len = sizeof(receiver);

    // check file
    char *filename = basename(argv[1]);
    if (filename == NULL)
    {
        perror("Can't get filename");
        exit(1);
    }

    // send filename
    
    
    strncpy(buff, filename, strlen(filename));
    if (sendto(sockfd, buff, BUFFSIZE, 0, (struct sockaddr *) &receiver, sizeof(receiver)) == -1)
    {
        perror("Can't send filename");
        exit(1);
    }


    // open file
    FILE *fp = fopen(argv[1], "rb");
    if (fp == NULL)
    {
        perror("Can't open file");
        exit(1);
    }

    // main loop

    while(1){

        createPackets(fp, buffer, &seqNo, &endOfFile, length);
        sendNoOfPackets(sockfd, receiver, len, seqNo);
        sendPackets(sockfd, receiver, len, buffer, seqNo, endOfFile, length);
        receiveAck(sockfd, receiver, len, Ack);

        while(ackMissing(Ack, seqNo)){
            sendMissingPackets(sockfd, receiver, len, seqNo, Ack, buffer);
            receiveAck(sockfd, receiver, len, Ack);
        }

        resetData(&seqNo, Ack);

        if(endOfFile){
            break;
        }
        

    }


    printf("\nFile transfer complete.\n");
    printf("Terminating connection...\n");
    memset(&receiver, 0, sizeof(receiver));
    printf("Connection terminated.\n");

    return 0;

}


void createPackets(FILE *fp,char buffer[noOfBuffers][BUFFSIZE], int *seqNo, bool *endOfFile, int *length){

    resetBuffers(buffer);

    printf("Creating Packets...\n");

    for(int i=0; i<5; i++){
        buffer[i][0] = *seqNo + '0'; // add sequence no to packets
        strcat(buffer[i], " ");		// add a space for distinguishing header and packet ( not necessary )
        length[i] = fread(buffer[i]+2, sizeof(char), BUFFSIZE-2, fp); // generating palyload of packets
        *seqNo += 1;

        if(feof(fp)){
            *endOfFile = true;
            strcpy(buffer[5],"end of file");
            break;
        }    
    }

}

void resetBuffers(char buffer[noOfBuffers][BUFFSIZE]){

    for(int i=0; i<6; i++){
        memset(&buffer[i], 0, sizeof(buffer[i]));
    } 
}

void sendNoOfPackets(int sockfd,struct sockaddr_in receiver,int len,int seqNo){

    char noOfPackets[5];
    printf("sending no. of packets...\n");
    memset(&noOfPackets, 0, sizeof(noOfPackets));
    noOfPackets[0] = seqNo + '0';
    sendto(sockfd, noOfPackets, sizeof(noOfPackets), 0, (struct sockaddr *)&receiver, len);

}


void sendPackets(int sockfd, struct sockaddr_in receiver, int len, char buffer[noOfBuffers][BUFFSIZE], int seqNo, bool endOfFile, int *length){

    char end[10];
    memset(&end, 0, sizeof(end));
    strcpy(end, "end");
    printf("Sending packets...\n");
    for(int i=0; i<seqNo; i++){
        
        sendto(sockfd, buffer[i], length[i]+2, 0, (struct sockaddr *)&receiver, len); // sending generated packets
    }
	// after sending packets sending either end of window represented by variable "end" or end of file present in buffer[5]
    if(endOfFile){
        sendto(sockfd, buffer[5], sizeof(buffer[5]), 0, (struct sockaddr *)&receiver, len);
    }else{
        sendto(sockfd, end, sizeof(end), 0, (struct sockaddr *)&receiver, len);
    
    }

}


void receiveAck(int sockfd, struct sockaddr_in receiver,int len,bool *Ack){
   
    printf("Receiving Acks...\n");
    recvfrom(sockfd, Ack, 5, 0, (struct sockaddr *)&receiver, &len);  

}
// check for missing acks
bool ackMissing(bool *Ack, int seqNo){

    printf("Checking for missing Acks...\n");
    for(int i=0; i<seqNo; i++){
        if(!Ack[i]){
            return true;
        }
    }
    return false;

}

void sendMissingPackets(int sockfd, struct sockaddr_in receiver, int len, int seqNo, bool *Ack, char buffer[noOfBuffers][BUFFSIZE]){
   
    printf("Sending missing packets...\n");
    for(int i=0; i<seqNo; i++){

        if(!Ack[i]){ // resending packets with ack 0
            sendto(sockfd, buffer[i], BUFFSIZE, 0, (struct sockaddr *)&receiver, len);
        }

    }

}


void resetData(int *seqNo, bool *Ack){
    *seqNo = 0;
    memset(&Ack, 0, 5);
}