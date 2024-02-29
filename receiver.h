#ifndef _RECEIVER_H_
#define _RECEIVER_H_

void* receiveMessage(void* unused);
void* writeMessage(void* unused);
void receiverInit(char* port);
void receiverShutdown();
void receiverCancel();

#endif