#include <stdio.h>
#include <signal.h>
#include <stdlib.h>

#include "mainInterface.h"

void handleSignal(int signal) {
    system("clear");
    printf("Encerrando o programa (Ctrl+c)\n");
    closeConnections();
    exit(0);
}

int main() {
    signal(SIGINT, handleSignal);
    initMenu();
    return 0;
}
