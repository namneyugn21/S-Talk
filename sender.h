#ifndef _SENDER_H_
#define _SENDER_H_

void* inputMessage(void* unused);
void* sendMessage(void* unused);
void senderInit(char* local, char* remoteAddr, char* remote);
void senderShutdown();
void senderCancel();

#endif