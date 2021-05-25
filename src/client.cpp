#include <iostream>
#include <time.h>       /* time */
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
#include <vector>
#include <sys/uio.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <fstream>
using namespace std;

std::mutex mtx;

void worker(int clientSd) {

    srand(time(NULL));

    cout << "IM WORKING BITCH" << endl;

    //create a message buffer 
    char msg[3000];
    char message[1025]; 
    int bytesRead, bytesWritten = 0;
    int count = 0;
    char letters[] = "123456789abcdefghijklmnopqtrsuvxwyzABCDEFGHIJKLMNOPQTRSUVWXYZ";
    int m = 0;
    while(m < 22)
    {
        srand(17);
        int random = 0;
        random = rand() % 5;
        string data;
        memset(&msg, 0, sizeof(msg));//clear the buffer
        memset(&message, 0, sizeof(message));//clear the buffer
        int i = 0;
        while(i < 1024) {
            char x = letters[rand() % 61];
            message[i] = x;
            i++;
        }
        //message[1024] = '\0';
        if(random >= 0)
            data = "put";
        else
            data = "get";
        
        //len of message will be: 3 + 1(|) + 
        
        strcpy(msg, data.c_str());
        // we want to update a register
        // send "update" to server
        // server asks for register key
        // send register key and text
        if(data == "put") {
            strcat(msg, "|");
            char s[1];
            sprintf(s, "%d", random);
            strcat(msg, s);
            strcat(msg, "|");
            strcat(msg, message);
            cout << "LENGTH OF MESSAGE " << strlen(msg) << endl;
            char d[1];
            sprintf(d, "%d", (int) strlen(msg));
            send(clientSd, d, strlen(d),0);
            send(clientSd, msg, sizeof(msg), 0);
        }
        // we want to read a register
        // send "read" to server
        // server asks for register key
        // send register key
        // receive and print text
        /*if(data == "read") {
            bytesWritten += send(clientSd, (char*)&msg, strlen(msg), 0); // send "read"
            memset(&msg, 0, sizeof(msg));//clear the buffer
            bytesRead += recv(clientSd, (char*)&msg, sizeof(msg), 0);
            cout << "Server: " << msg << endl; // server asked for register key
            memset(&msg, 0, sizeof(msg));//clear the buffer
            getline(cin, data);
            strcpy(msg, data.c_str());
            bytesWritten += send(clientSd, (char*)&msg, strlen(msg), 0); // send register key
            memset(&msg, 0, sizeof(msg));
            //bytesWritten += send(clientSd, (char*)&message, strlen(message), 0); // send text to update
            bytesRead += recv(clientSd, (char*)&msg, sizeof(msg), 0); // receive register text
            cout << msg << endl;
            memset(&message, 0, sizeof(message));//clear the buffer
            memset(&msg, 0, sizeof(msg));
        }*/
    m++;
    
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
    
    int bytesRead, bytesWritten = 0;
    struct timeval start1, end1;
    gettimeofday(&start1, NULL);
    std::vector<std::thread> threads;
    int i;
    int num_threads = 1;
    int num = 0;
    for(i = 0; i < num_threads; i++) {
        int clientSd = socket(AF_INET, SOCK_STREAM, 0);
        //try to connect...
        int status = connect(clientSd, (sockaddr*) &sendSockAddr, sizeof(sendSockAddr));
        if(status < 0)
        {
            cout<<"Error connecting to socket!"<<endl;
        }
        cout << "Connected to the server!" << endl;

        threads.push_back(std::thread(worker,clientSd));
        //t.detach();
    }

    for(auto&thread : threads) {
        thread.join();
    }
    gettimeofday(&end1, NULL);
    //close(clientSd);
    cout << "********Session********" << endl;
    cout << "Bytes written: " << bytesWritten << 
    " Bytes read: " << bytesRead << endl;
    cout << "Elapsed time: " << (end1.tv_sec- start1.tv_sec) 
      << " secs" << endl;
    cout << "Connection closed" << endl;
    return 0;    
}