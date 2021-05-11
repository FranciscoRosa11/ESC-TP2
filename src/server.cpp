#include <iostream>
#include <string>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <unistd.h>
#include <thread>
#include <mutex>
#include <string.h>
#include <netdb.h>
#include <sys/uio.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <fstream>
using namespace std;

std::mutex mtx;
fstream readFile;

int64_t key = 0;

void worker(int newSd) {
    char buffer[1024];
    char textfile[1024];
    int text_len = 1024;
    int buffer_len = 1024;
    int bytecount;

    while(1) {
        memset(buffer, 0, buffer_len);
        memset(textfile, 0, text_len);
        if((bytecount = recv(newSd, buffer, buffer_len, 0))== -1){
            fprintf(stderr, "Error receiving data %d\n", errno);
            break;
        }
        printf("Received bytes %d\nReceived string \"%s\"\n", bytecount, buffer);
        if(!strcmp(buffer, "create")) {
            mtx.lock();
            int index = rand() % 100000;
            readFile.seekp(index * 1024);
            strcat(buffer, " SERVER ECHO");
            if((bytecount = send(newSd, buffer, strlen(buffer), 0))== -1){
                fprintf(stderr, "Error sending data %d\n", errno);
            }
            memset(buffer, 0, buffer_len);
            if((bytecount = recv(newSd, textfile, text_len, 0))== -1){
                fprintf(stderr, "Error receiving data to write in file%d\n", errno);
            }
            printf("Received bytes %d\n", bytecount);
            memset(buffer, 0, buffer_len);
            memset(textfile, 0, text_len);
            mtx.unlock();
        }
        // we want to update a register
        // receive "update" from client
        // ask for register key
        // receive register key and text
        if(!strcmp(buffer, "update")) {
            memset(buffer, 0, buffer_len);
            strcat(buffer, "SEND REGISTER KEY");
            if((bytecount = send(newSd, buffer, strlen(buffer), 0))== -1){ //ask for register key
                fprintf(stderr, "Error sending data %d\n", errno);
            }
            memset(buffer, 0, buffer_len);
            if((bytecount = recv(newSd, buffer, buffer_len, 0))== -1){ //receive register key
                fprintf(stderr, "Error receiving data to write in file%d\n", errno);
            }
            cout << "REGISTER KEY: " << buffer << endl;
            std::string k = buffer;
            cout << "KEY FILE IS " << k << endl;
            memset(buffer, 0, buffer_len);
            ofstream file ("../files/"+k+".txt");
            if((bytecount = recv(newSd, buffer, buffer_len, 0))== -1){ //receive text content
                fprintf(stderr, "Error receiving data to write in file%d\n", errno);
            }
            if(file.is_open()) {
                file << buffer << endl;
                file.close();
            }
            else {
                fprintf(stderr, "Error file does not exist %d\n", errno);
            }
            memset(buffer, 0, buffer_len);
        }
        if(!strcmp(buffer, "read")) {
            memset(buffer, 0, buffer_len);
            strcat(buffer, "SEND REGISTER KEY");
            if((bytecount = send(newSd, buffer, strlen(buffer), 0))== -1){ //ask for register key
                fprintf(stderr, "Error sending data %d\n", errno);
            }
            memset(buffer, 0, buffer_len);
            if((bytecount = recv(newSd, buffer, buffer_len, 0))== -1){ //receive register key
                fprintf(stderr, "Error receiving register key %d\n", errno);
            }
            cout << "REGISTER KEY: " << buffer << endl;
            std::string k = buffer;
            cout << "KEY FILE IS " << k << endl;
            memset(buffer, 0, buffer_len);
            std::ifstream file("../files/"+k+".txt");
            if(file.is_open()) {
                
                std::string content( (std::istreambuf_iterator<char>(file) ),
                       (std::istreambuf_iterator<char>()    ) );
                file.close();
                strcpy(buffer, content.c_str());
                if((bytecount = send(newSd, buffer, buffer_len, 0))== -1){ //send file content
                    memset(buffer, 0, buffer_len);
                    fprintf(stderr, "Error sending file data %d\n", errno);
                }
            }
            else {
                fprintf(stderr, "Error file does not exist %d\n", errno);
            }
            memset(buffer, 0, buffer_len);
        }
        printf("Sent bytes %d\n", bytecount);
    }
}

//Server side
int main(int argc, char *argv[])
{
    //for the server, we only need to specify a port number
    if(argc != 2)
    {
        cerr << "Usage: port" << endl;
        exit(0);
    }

    // Create records file
    char letters[] = "abcdefghijklmnopqrstuvwxyz";
    char message[1024];
    char r[1024];
    fstream outFile;
    outFile.open("../files/records.txt", fstream::out | fstream::in);
    int i = 0;
    int j = 0;
    for(i = 0; i < 100000; i++) {
        memset(message, 0, 1024);
        while(j < 1024) {
            char x = letters[rand() % 26];
            message[j] = x;
            j++;
        }
        j = 0;
        outFile.seekp(i * 1024);
        outFile.write(message, 1024);
    }

    outFile.close();

    readFile.open("../files/records.txt", fstream::out | fstream::in);

    //grab the port number
    int port = atoi(argv[1]);
    //buffer to send and receive messages with
    char msg[1500];
     
    //setup a socket and connection tools
    sockaddr_in servAddr;
    bzero((char*)&servAddr, sizeof(servAddr));
    servAddr.sin_family = AF_INET;
    servAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servAddr.sin_port = htons(port);
 
    //open stream oriented socket with internet address
    //also keep track of the socket descriptor
    int serverSd = socket(AF_INET, SOCK_STREAM, 0);
    if(serverSd < 0)
    {
        cerr << "Error establishing the server socket" << endl;
        exit(0);
    }
    //bind the socket to its local address
    int bindStatus = ::bind(serverSd, (struct sockaddr*) &servAddr, 
        sizeof(servAddr));
    if(bindStatus < 0)
    {
        cerr << "Error binding socket to local address" << endl;
        exit(0);
    }
    cout << "Waiting for a client to connect..." << endl;
    //listen for up to 5 requests at a time
    listen(serverSd, 5);
    //receive a request from client using accept
    //we need a new address to connect with the client
    sockaddr_in newSockAddr;
    socklen_t newSockAddrSize = sizeof(newSockAddr);
    //accept, create a new socket descriptor to 
    //handle the new connection with client
    struct timeval start1, end1;
    gettimeofday(&start1, NULL);
    int newSd;
    while((newSd = accept(serverSd, (sockaddr *)&newSockAddr, &newSockAddrSize)) != -1) {
        cout << "Connected with client!" << endl;

        std::thread t(worker,newSd);
        t.detach();
    }
    
    //we need to close the socket descriptors after we're all done
    gettimeofday(&end1, NULL);
    close(newSd);
    close(serverSd);
    cout << "********Session********" << endl;
    //cout << "Bytes written: " << bytesWritten << " Bytes read: " << bytesRead << endl;
    cout << "Elapsed time: " << (end1.tv_sec - start1.tv_sec) 
        << " secs" << endl;
    cout << "Connection closed..." << endl;
    readFile.close();
    return 0;   
}