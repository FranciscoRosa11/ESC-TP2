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

    struct timeval start1, end1;
    gettimeofday(&start1, NULL);

    std::thread::id t_id = std::this_thread::get_id();
    cout << "THREAD ID: " << t_id << endl; 

    char buffer[3000];
    char textfile[1025];
    int text_len = 1024;
    int buffer_len = 3000;
    int bytecount;

    if(readFile.is_open()) {
        cout << "GLOBAL FILE IS OPEN" << endl;
    }
    while(1) {
        int leng = 0;
        char *op = (char*)malloc(4);
        memset(buffer, 0, buffer_len);
        memset(textfile, 0, text_len);
        while (leng < 4) {
            if((bytecount = recv(newSd, buffer, 4, 0))== -1) { //receive data-length
                fprintf(stderr, "Error receiving data %d\n", errno);
                break;
            }
            leng += bytecount;
            std::cout << "READ BYTES " << leng << " " << buffer << endl;
        }
        buffer[bytecount] = '\0';
        int len = atoi(buffer);
        std::cout << "A LEN DA LINHA 51 É " << len << endl;
        if(len >= 3000) {
            strcpy(op, "get");
            len = len - 3000;
            std::cout << op << endl;
        }
        else {
            strcpy(op, "put");
        }
        memset(buffer, 0, buffer_len);
        if(strcmp(op, "put") == 0) {
            std::cout << "ESTOU NO PUT" << endl;
            if((bytecount = recv(newSd, buffer, len, 0))== -1){ //receive data
                fprintf(stderr, "Error receiving data %d\n", errno);
                break;
            }
            std::cout << "RECEIVED DATA WITH BYTES " << bytecount << endl;
            std::cout << "HERE BEFORE LINE 56" << endl;
            buffer[bytecount] = '\0';
            char array[3][1024];
            int size = 0;
            char* res;
            res = strtok(buffer, "|");
            while(res != NULL) {
                strcpy(array[size],res);
                size++;
                res = strtok(NULL, "|");
            }
            mtx.lock();
            readFile.seekp(atoi(array[1]) * 1024, ios::beg);
            readFile.write(array[2], 1024);
            readFile.seekp(0, ios::beg);
            mtx.unlock();
            //readFile.close();
            memset(buffer, 0, buffer_len);
        } else if(strcmp(op, "get") == 0) {
            std::cout << "ESTOU NO GET" << endl;
            if((bytecount = recv(newSd, buffer, len, 0))== -1){ //receive data with key
                fprintf(stderr, "Error receiving data %d\n", errno);
                break;
            }
            std::cout << "RECEIVED DATA WITH BYTES " << bytecount << endl;
            std::cout << buffer << endl;
            buffer[bytecount] = '\0';
            char array[2][1024];
            int size = 0;
            char* res;
            res = strtok(buffer, "|");
            while(res != NULL) {
                strcpy(array[size],res);
                size++;
                res = strtok(NULL, "|");
            }
            memset(buffer, 0, buffer_len);
            readFile.seekp((int) atoi(array[1]) * 1024, ios::beg);
            char r[100000];
            if(readFile.is_open()) {
                readFile.read(r, 1024);
                std::cout << r << endl;
                readFile.seekp(0, ios::beg);
            } else {
                std::cout << "FILE CLOSED" << endl;
            }
            send(newSd, r, 1024, 0); //send data to client
            std::cout << "SENT 1024 bytes to client" << endl;
            //readFile.close();
            memset(buffer, 0, buffer_len);
        }
        
    }
    gettimeofday(&end1, NULL);
    std::cout << "Elapsed time: " << (end1.tv_sec - start1.tv_sec) 
        << " secs" << endl;
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
    std::ofstream outFile("../files/records.txt");
    //outFile.open("../files/records.txt", fstream::out | fstream::in);
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
    readFile.open("../files/records.txt", ios::in | ios::out);

    //things();
    
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
    std::cout << "Waiting for a client to connect..." << endl;
    //listen for up to 5 requests at a time
    listen(serverSd, 5);
    //receive a request from client using accept
    //we need a new address to connect with the client
    sockaddr_in newSockAddr;
    socklen_t newSockAddrSize = sizeof(newSockAddr);
    //accept, create a new socket descriptor to 
    //handle the new connection with client
    
    int newSd;
    while((newSd = accept(serverSd, (sockaddr *)&newSockAddr, &newSockAddrSize)) != -1) {
        std::cout << "Connected with client!" << endl;

        std::thread t(worker,newSd);
        t.detach();
    }
    
    //we need to close the socket descriptors after we're all done
    
    close(newSd);
    close(serverSd);
    std::cout << "********Session********" << endl;
    //cout << "Bytes written: " << bytesWritten << " Bytes read: " << bytesRead << endl;
    std::cout << "Connection closed..." << endl;
    return 0;   
}