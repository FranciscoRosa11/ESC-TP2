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

void worker(int clientSd) {

    //create a message buffer 
    char msg[1024];
    char message[1024]; 
    int bytesRead, bytesWritten = 0;
    int count = 0;
    char letters[] = "abcdefghijklmnopqrstuvwxyz";

    while(count < 22)
    {
        int randNumber = rand() % 22;
        memset(&msg, 0, sizeof(msg));//clear the buffer
        memset(&message, 0, sizeof(message));//clear the buffer
        int i = 0;
        while(i < 1024) {
            char x = letters[rand() % 26];
            message[i] = x;
            i++;
        }
        /*if(data == "exit")
        {
            send(clientSd, (char*)&msg, strlen(msg), 0);
            break;
        }*/
        if(randNumber != 0) { //"create" operation
            strcpy(msg, "create");
            bytesWritten += send(clientSd, (char*)&msg, strlen(msg), 0); //send "create"
            memset(&msg, 0, sizeof(msg));//clear the buffer
            bytesRead += recv(clientSd, (char*)&msg, sizeof(msg),0); //server asks for register key
            cout << "Server: " << msg << endl;
            memset(&msg, 0, sizeof(msg));
            int regKey = rand() % 100000;
            std::string s = std::to_string(regKey);
            strcpy(msg, s.c_str());
            bytesWritten += send(clientSd, (char*)&msg, strlen(message), 0); //send register key
            memset(&msg, 0, sizeof(msg));
            bytesWritten += send(clientSd, (char*)&message, strlen(message), 0); //send register content 
            memset(&message, 0, sizeof(message));//clear the buffer
        }
        // we want to read a register
        // send "read" to server
        // server asks for register key
        // send register key
        // receive and print text
        else {
            bytesWritten += send(clientSd, (char*)&msg, strlen(msg), 0); // send "read"
            memset(&msg, 0, sizeof(msg));//clear the buffer
            bytesRead += recv(clientSd, (char*)&msg, sizeof(msg), 0);
            cout << "Server: " << msg << endl; // server asked for register key
            memset(&msg, 0, sizeof(msg));//clear the buffer
            bytesWritten += send(clientSd, (char*)&msg, strlen(msg), 0); // send register key
            memset(&msg, 0, sizeof(msg));
            //bytesWritten += send(clientSd, (char*)&message, strlen(message), 0); // send text to update
            bytesRead += recv(clientSd, (char*)&msg, sizeof(msg), 0); // receive register text
            cout << msg << endl;
            memset(&message, 0, sizeof(message));//clear the buffer
            memset(&msg, 0, sizeof(msg));
        }
    }

}

//Client side
int main(int argc, char *argv[])
{
    //we need 2 things: ip address and port number, in that order
    if(argc != 3)
    {
        cerr << "Usage: ip_address port" << endl; exit(0); 
    } //grab the IP address and port number 
    char *serverIp = argv[1]; int port = atoi(argv[2]); 
    //create a message buffer 
    char msg[1024];
    char message[1024]; 
    //setup a socket and connection tools 
    struct hostent* host = gethostbyname(serverIp); 
    sockaddr_in sendSockAddr;   
    bzero((char*)&sendSockAddr, sizeof(sendSockAddr)); 
    sendSockAddr.sin_family = AF_INET; 
    sendSockAddr.sin_addr.s_addr = 
        inet_addr(inet_ntoa(*(struct in_addr*)*host->h_addr_list));
    sendSockAddr.sin_port = htons(port);
    int clientSd = socket(AF_INET, SOCK_STREAM, 0);
    //try to connect...
    int status = connect(clientSd,
                         (sockaddr*) &sendSockAddr, sizeof(sendSockAddr));
    if(status < 0)
    {
        cout<<"Error connecting to socket!"<<endl;
    }
    cout << "Connected to the server!" << endl;
    int bytesRead, bytesWritten = 0;
    struct timeval start1, end1;
    gettimeofday(&start1, NULL);

    int num_threads = 8;
    int num = 0;

    while(num < num_threads) {
        cout << "Connected with client!" << endl;

        std::thread t(worker,clientSd);
        t.detach();
    }

    gettimeofday(&end1, NULL);
    close(clientSd);
    cout << "********Session********" << endl;
    cout << "Bytes written: " << bytesWritten << 
    " Bytes read: " << bytesRead << endl;
    cout << "Elapsed time: " << (end1.tv_sec- start1.tv_sec) 
      << " secs" << endl;
    cout << "Connection closed" << endl;
    return 0;    
}