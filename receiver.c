#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <arpa/inet.h>

#include "receiver.h"
#include "sender.h"
#include "list.h"

#define MAX_LEN 1024

static List* receiverList;
static pthread_t receiveThread;
static pthread_t writeThread;
static pthread_mutex_t receiveMutex;
static pthread_cond_t receiveCond;
static pthread_cond_t writeCond;
static int senderPort; // the port number of the sender

void* receiveMessage(void* unused) {
    // Set up the socket and binding process for the sender
    // Learned from Dr. Brian Fraser video
    struct sockaddr_in sinRemote;
    unsigned int sinLen = sizeof(sinRemote);
    memset(&sinRemote, 0, sizeof(sinRemote));
    sinRemote.sin_family = AF_INET;
    sinRemote.sin_port = htons(senderPort);
    sinRemote.sin_addr.s_addr = htonl(INADDR_ANY);
    int senderSocket = socket(PF_INET, SOCK_DGRAM, 0);
    if (senderSocket == -1) {
        perror("Failed to get the sender socket!\n");
        exit(EXIT_FAILURE);
    }
    if (bind(senderSocket, (struct sockaddr*) &sinRemote, sizeof(sinRemote)) == -1) {
        perror("Failed to bind the sender socket!\n");
        exit(EXIT_FAILURE);
    }

    char message[MAX_LEN];
    while (1) {
        int bytesRx = recvfrom(senderSocket, message, MAX_LEN, 0, (struct sockaddr*) &sinRemote, &sinLen);
        int terminateIdx = (bytesRx < MAX_LEN) ? bytesRx : MAX_LEN - 1;
        message[terminateIdx] = '\0';
        while (1) {
            if (List_append(receiverList, message) == 0) {
                // Append the new message to the list successfully
                pthread_mutex_lock(&receiveMutex);
                {
                    pthread_cond_signal(&writeCond);
                }
                pthread_mutex_unlock(&receiveMutex);
                break;
            } else {
                // The list is not appended (maybe full or error)
                // Hence it will wait until there is a free node to append
                printf("Failed to load the message... Attempt to try again!");
                pthread_mutex_lock(&receiveMutex);
                {
                    pthread_cond_wait(&receiveCond, &receiveMutex);
                }
                pthread_mutex_unlock(&receiveMutex);
            }
        }
    }
    return NULL;
}

void* writeMessage(void* ununsed) {
    while (1) {
        // Waiting for a message to be printed
        pthread_mutex_lock(&receiveMutex);
        {
            pthread_cond_wait(&writeCond, &receiveMutex);
        }
        pthread_mutex_unlock(&receiveMutex);
        // Retrieve the message from the receiverList
        char message[MAX_LEN];
        strncpy(message, (char*)List_remove(receiverList), sizeof(message) - 1);
        message[sizeof(message) - 1] = '\0';
        // Signal the waiting receiveCond that there is a node available
        pthread_mutex_lock(&receiveMutex);
        {
            pthread_cond_signal(&receiveCond);
        }
        pthread_mutex_unlock(&receiveMutex);
        // Print the message onto the screen
        char indicator[] = ">> ";
        fputs(indicator, stdout);
        fputs(message, stdout);
        fflush(stdout);
    }
}

void receiverInit(char* port) {
    // Create the list
    receiverList = List_create();
    // Initialize all the neccessary info
    senderPort = atoi(port);
    // Create the input thread from keyboard
    // and create the send thread to send the message
    pthread_create(&receiveThread, NULL, receiveMessage, NULL);
    pthread_create(&writeThread, NULL, writeMessage, NULL);
}

void receiverShutdown() {
    pthread_join(receiveThread, NULL);
    pthread_join(writeThread, NULL);
}

void receiverCancel() {
    pthread_cancel(receiveThread);
    pthread_cancel(writeThread);
}