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
#include <chrono>
#include <fcntl.h>
#include <fstream>
using namespace std;
using namespace std::chrono;

std::mutex mtx;
fstream readFile;
ofstream resultsFile("../files/results.txt", ios::app | ios::out);
int64_t key = 0;

void worker(int newSd) {

    ifstream fromFile("../files/results.txt", ios::in);

    auto start = high_resolution_clock::now();

    std::thread::id t_id = std::this_thread::get_id();
    cout << "THREAD ID: " << t_id << endl; 

    char buffer[3000];
    char textfile[1025];
    int text_len = 1024;
    int buffer_len = 3000;
    int bytecount;

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
            //std::cout << "READ BYTES " << leng << " " << buffer << endl;
        }
        buffer[bytecount] = '\0';
        if(strcmp(buffer, "exit") == 0) {
            cout << "CIAO " << buffer << " THREAD " << t_id << endl;
            close(newSd);
            break;
        }
        int len = atoi(buffer);
        if(len >= 3000) {
            strcpy(op, "get");
            len = len - 3000;
        }
        else {
            strcpy(op, "put");
        }
        memset(buffer, 0, buffer_len);
        if(strcmp(op, "put") == 0) {
            if((bytecount = recv(newSd, buffer, len, 0))== -1){ //receive data
                fprintf(stderr, "Error receiving data %d\n", errno);
                break;
            }
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
            if((bytecount = recv(newSd, buffer, len, 0))== -1){ //receive data with key
                fprintf(stderr, "Error receiving data %d\n", errno);
                break;
            }
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
            //mtx.lock();
            fromFile.seekg((int) atoi(array[1]) * 1024, ios::beg);
            char r[100000];
            if(fromFile.is_open()) {
                fromFile.read(r, 1024);
                fromFile.seekg(0, ios::beg);
            } else {
                std::cout << "FILE CLOSED" << endl;
            }
            //mtx.unlock();
            send(newSd, r, 1024, 0); //send data to client
            //readFile.close();
            memset(buffer, 0, buffer_len);
        }
        
    }
    fromFile.close();
    auto stop = high_resolution_clock::now();
    auto duration = duration_cast<milliseconds>(stop - start);
    char count[100];
    sprintf(count, "%lld\n", duration.count());
    mtx.lock();
    if (resultsFile.is_open())
    {
        resultsFile << count;
        resultsFile.flush();
    }
    else
    {
        std::cerr << "didn't write" << std::endl;
    }
    cout << "THREAD " << t_id << " took: " << duration.count() << " milliseconds" << endl;
}

//Server side
int main(int argc, char *argv[])
{
    //for the server, we only need to specify a port number
    if(argc != 3) //third argument is the number of clients we will be testing with
    {
        cerr << "Usage: port" << endl;
        exit(0);
    }
    char* numClients = argv[2];
    resultsFile << "Number of clients: " << numClients << "\n";
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
    listen(serverSd, atoi(numClients));
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