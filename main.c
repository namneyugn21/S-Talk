#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <string.h>

#include "list.h"
#include "sender.h"
#include "receiver.h"

int main(int argc, char* argv[]) {
    if (argc != 4) {
        printf("Enter exactly four arguments: s-talk [my port number] [remote machine name] [remote port number]\n");
    } else {
        printf("\nWelcome to S-Talk, you are chatting on port [%s]!\n", argv[1]);
        printf("Start typing to chat or type '!' to quit. Enjoy :)\n\n");
        senderInit(argv[1], argv[2], argv[3]);
        receiverInit(argv[1]);
        senderShutdown();
        receiverShutdown();
    }
    return 0;
}