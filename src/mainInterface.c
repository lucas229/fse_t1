#include <stdio.h>
#include <stdlib.h>

#include "mainInterface.h"

void init() {
    system("clear");
    getEnrollment();
    mainMenu();
}

void getEnrollment() {
    printf("Matrícula: ");
    int num;
    scanf("%d", &num);
    system("clear");
    for(int i = 0; i < ENROLLMENT_LENGTH; i++) {
        enrollment[ENROLLMENT_LENGTH - i - 1] = num % 10;
        num /= 10;
    }
}

void mainMenu() {
}
