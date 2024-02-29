#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <arpa/inet.h>

#include "sender.h"
#include "receiver.h"
#include "list.h"

#define MAX_LEN 1024

static List* senderList;
static pthread_t inputThread;
static pthread_t sendThread;
static pthread_mutex_t inputMutex;
static pthread_cond_t inputCond;
static pthread_cond_t sendCond;
static char* localPort;
static char* remotePort;
static char* remoteAddress;

void* inputMessage(void* unused) {
    while (1) {
        char message[MAX_LEN];
        fgets(message, MAX_LEN, stdin);
        if (message[0] == '!' && strlen(message) == 2) {
            senderCancel();
            receiverCancel();
        }
        while(1) {
            if (List_append(senderList, message) == 0) { 
                // Append the list successfully
                pthread_mutex_lock(&inputMutex);
                {
                    pthread_cond_signal(&inputCond);
                }
                pthread_mutex_unlock(&inputMutex);
                break;
            } else {
                // The list is not appended (maybe full or error)
                // Hence it will wait until there is a free node to append
                printf("Failed to input the message... Try again!\n");
                pthread_mutex_lock(&inputMutex);
                {
                    pthread_cond_wait(&sendCond, &inputMutex);
                }
                pthread_mutex_unlock(&inputMutex);
            }
        }
    }
    return NULL;
}

void* sendMessage(void* unused) {
    // Set up the socket of the local device
    struct sockaddr_in sin;
    memset(&sin, 0, sizeof(sin));
    sin.sin_family = AF_INET;
    sin.sin_port = htons(atoi(localPort));
    sin.sin_addr.s_addr = htonl(INADDR_ANY);
    int localSocket = socket(PF_INET, SOCK_DGRAM, 0);
    if (localSocket == -1) {
        perror("Failed to get the sender socket!\n");
        exit(EXIT_FAILURE);
    }
    // Set up the address info of the remote receiver
    struct addrinfo hints;
    struct addrinfo* result; // info will be stored here
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_protocol = 0;
    hints.ai_flags = 0;
    if (getaddrinfo(remoteAddress, remotePort, &hints, &result) != 0) {
        perror("There is a problem getting the remote address information!\n");
        exit(EXIT_FAILURE);
    }

    while (1) {
        // Wait for the signal from inputMessage that there is a message input
        pthread_mutex_lock(&inputMutex);
        {
            pthread_cond_wait(&inputCond, &inputMutex);
        }
        pthread_mutex_unlock(&inputMutex);
        // Retrieve the message from the senderList
        char message[MAX_LEN];
        strncpy(message, (char*)List_remove(senderList), sizeof(message) - 1);
        message[sizeof(message) - 1] = '\0';
        // Signal the waiting sendCond that there is a node available
        pthread_mutex_lock(&inputMutex);
        {
            pthread_cond_signal(&sendCond);
        }
        pthread_mutex_unlock(&inputMutex);
        // Send the message to the remote receiver
        if (sendto(localSocket, message, strlen(message), 0, result->ai_addr, result->ai_addrlen) == -1) {
            perror("There is an error sending the message...\n");
            exit(EXIT_FAILURE);
        }
    }
}

void senderInit(char* local, char* remoteAddr, char* remote) {
    // Create the list
    senderList = List_create();
    // Initialize all the neccessary info
    localPort = local;
    remotePort = remote;
    remoteAddress = remoteAddr;
    // Create the input thread from keyboard
    // and create the send thread to send the message
    pthread_create(&inputThread, NULL, inputMessage, NULL);
    pthread_create(&sendThread, NULL, sendMessage, NULL);
}

void senderShutdown() {
    pthread_join(inputThread, NULL);
    pthread_join(sendThread, NULL);
}

void senderCancel() {
    pthread_cancel(inputThread);
    pthread_cancel(sendThread);
}