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

void worker(char *serverIp, int port) {

    std::thread::id t_id = std::this_thread::get_id();
    cout << "THREAD ID: " << t_id << endl; 

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
        int status = connect(clientSd, (sockaddr*) &sendSockAddr, sizeof(sendSockAddr));
        if(status < 0)
        {
            std::cout<<"Error connecting to socket!"<<endl;
        }
        std::cout << "Connected to the server!" << endl;

    srand(time(NULL));

    //create a message buffer 
    char msg[3000];
    char message[1025]; 
    int bytesRead, bytesWritten = 0;
    int count = 0;
    char letters[] = "123456789abcdefghijklmnopqtrsuvxwyzABCDEFGHIJKLMNOPQTRSUVWXYZ";
    int m = 0;
    while(1) {
        while(m < 22)
    {
        srand(m+25);
        int random = 0;
        random = rand() % 22; // 0->read && >0->write
        int rd = 0;
        rd = rand() % 100000;
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
        if(random > 0)
            data = "put";
        else
            data = "get";
        
        //len of message will be: 3 + 1(|) + 1024+3+1+1+(1---6) = 1030 -- 1035
        
        strcpy(msg, data.c_str());
        // we want to update a register
        // send "update" to server
        // server asks for register key
        // send register key and text
        if(data == "put") {
            strcat(msg, "|");
            char s[1];
            sprintf(s, "%d", rd);
            strcat(msg, s);
            strcat(msg, "|");
            strcat(msg, message);
            char d[1];
            sprintf(d, "%d", (int) strlen(msg));
            send(clientSd, d, strlen(d),0); // this is sending the data size
            send(clientSd, msg, strlen(msg), 0); //sending the actual data
        } else if(data == "get") {
            strcat(msg, "|");
            char s[1];
            sprintf(s, "%d", rd);
            strcat(msg,s);
            char d[1];
            sprintf(d, "%d", (int) strlen(msg) + 3000);
            send(clientSd, d, strlen(d), 0); // send data length
            send(clientSd, msg, strlen(msg), 0); //send register key
            memset(&msg, 0, sizeof(msg));
            recv(clientSd, msg, 1024, 0); // receive data;
            std::cout << "THREAD " << t_id << " " << msg << endl;
            memset(&msg, 0, sizeof(msg));
        }
        m++;
    
        }

        memset(&msg, 0, sizeof(msg));
        strcpy(msg, "exit");
        send(clientSd, msg, strlen(msg),0);
        close(clientSd);
        break;

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
    
    
    int bytesRead, bytesWritten = 0;
    struct timeval start1, end1;
    gettimeofday(&start1, NULL);
    std::vector<std::thread> threads;
    int i;
    char* nt = argv[3];
    int num_threads = atoi(nt);
    int num = 0;
    for(i = 0; i < num_threads; i++) {
        threads.push_back(std::thread(worker,serverIp, port));
        //t.detach();
    }

    for(auto&thread : threads) {
        thread.join();
    }
    gettimeofday(&end1, NULL);
    //close(clientSd);
    std::cout << "********Session********" << endl;
    std::cout << "Bytes written: " << bytesWritten << 
    " Bytes read: " << bytesRead << endl;
    std::cout << "Elapsed time: " << (end1.tv_sec- start1.tv_sec) 
      << " secs" << endl;
    std::cout << "Connection closed" << endl;
    return 0;    
}