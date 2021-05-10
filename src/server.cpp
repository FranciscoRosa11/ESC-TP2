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
        }
        printf("Received bytes %d\nReceived string \"%s\"\n", bytecount, buffer);
        if(!strcmp(buffer, "create")) {
            mtx.lock();
            key += 1;
            mtx.unlock();
            strcat(buffer, " SERVER ECHO");
            if((bytecount = send(newSd, buffer, strlen(buffer), 0))== -1){
                fprintf(stderr, "Error sending data %d\n", errno);
            }
            std::string s = std::to_string(key);
            std::ofstream outfile ("../files/"+s+".txt");
            memset(buffer, 0, buffer_len);
            if((bytecount = recv(newSd, textfile, text_len, 0))== -1){
                fprintf(stderr, "Error receiving data to write in file%d\n", errno);
            }
            outfile << textfile << std::endl;
            outfile.close();
            printf("Received bytes %d\n", bytecount);
            const char *cstr = s.c_str();
            strcat(buffer, cstr);
            memset(buffer, 0, buffer_len);
            memset(textfile, 0, text_len);
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
            printf("REGISTER KEY: %s\n", buffer);
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
    //int newSd = accept(serverSd, (sockaddr *)&newSockAddr, &newSockAddrSize);
    /*if(newSd < 0)
    {
        cerr << "Error accepting request from client!" << endl;
        exit(1);
    }*/
    
    
    //lets keep track of the session time
    
    //also keep track of the amount of data sent as well
    /*while(1)
    {
        //receive a message from the client (listen)
        cout << "Awaiting client response..." << endl;
        memset(&msg, 0, sizeof(msg));//clear the buffer
        bytesRead += recv(newSd, (char*)&msg, sizeof(msg), 0);
        if(!strcmp(msg, "exit"))
        {
            cout << "Client has quit the session" << endl;
            break;
        }
        cout << "Client: " << msg << endl;
        cout << ">";
        string data;
        getline(cin, data);
        memset(&msg, 0, sizeof(msg)); //clear the buffer
        strcpy(msg, data.c_str());
        if(data == "exit")
        {
            //send to the client that server has closed the connection
            send(newSd, (char*)&msg, strlen(msg), 0);
            break;
        }
        //send the message to client
        bytesWritten += send(newSd, (char*)&msg, strlen(msg), 0);
    }*/
    
    //we need to close the socket descriptors after we're all done
    gettimeofday(&end1, NULL);
    close(newSd);
    close(serverSd);
    cout << "********Session********" << endl;
    //cout << "Bytes written: " << bytesWritten << " Bytes read: " << bytesRead << endl;
    cout << "Elapsed time: " << (end1.tv_sec - start1.tv_sec) 
        << " secs" << endl;
    cout << "Connection closed..." << endl;
    return 0;   
}