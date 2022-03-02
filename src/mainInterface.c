#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "mainInterface.h"

void initMenu() {
    system("clear");
    getEnrollmentMenu();
    mainMenu();
}

void getEnrollmentMenu() {
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
    while(1) {
        printf("1 - Acionar forno\n");
        printf("2 - Atualizar temperatura de referência\n");
        printf("3 - Alterar parâmetros\n");
        printf("0 - Finalizar\n");
        int choice;
        scanf("%d", &choice);
        system("clear");
        if(choice == 1) {
            startOven();
        } else if(choice == 2) {
            referenceTemperatureMenu();
        } else if(choice == 3) {
            parametersMenu();
        } else {
            break;
        }
    }
}

void startOven() {
}

void referenceTemperatureMenu() {
    printf("1 - Potenciômetro externo\n");
    printf("2 - Entrada de teclado\n");
    printf("3 - Curva de temperatura\n");
    printf("0 - Retornar\n");
    int choice;
    scanf("%d", &choice);
    system("clear");
    if(choice == 1) {
        externalPotentiometerMenu();
    } else if(choice == 2) {
        keyboardInputMenu();
    } else if(choice == 3) {
        temperatureCurveMenu();
    }
}

void externalPotentiometerMenu() {
    readModbus(REF_TEMP, enrollment, &referenceTemperature);
    printf("Temperatura de referência: %f\n\n", referenceTemperature);
}

void keyboardInputMenu() {
    printf("Temperatura de referência: ");
    scanf("%f", &referenceTemperature);
    system("clear");
}

void temperatureCurveMenu() {
}

void parametersMenu() {
    printf("Kp: ");
    scanf("%lf", &kp);
    printf("Ki: ");
    scanf("%lf", &ki);
    printf("Kd: ");
    scanf("%lf", &kd);
    system("clear");
}
