#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "mainInterface.h"
#include "pid.h"

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
        printf("2 - Alterar temperatura de referência\n");
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
    int status;
    while(1) {
        if(readModbus(USER_CMD, enrollment, &status) != -1 && status == 1) {
            printf("Ligando\n");
            unsigned char s = 1;
            writeModbus(SYS_STATUS, enrollment, &s);
            float internalTemperature;
            readModbus(INT_TEMP, enrollment, &internalTemperature);
            printf("Temperatura interna: %f\n", internalTemperature);
            printf("Sinal de controle: %lf\n", pid_controle(internalTemperature));
            while(1) {
                if(readModbus(USER_CMD, enrollment, &status) != -1 && status == 2) {
                    printf("Desligando\n");
                    s = 0;
                    writeModbus(SYS_STATUS, enrollment, &s);
                }
                printf("-------------------Estado: %d\n", status);
                sleep(1);
            }
        } else {
            printf("Falha\n");
        }
        sleep(1);
    }
}

void referenceTemperatureMenu() {
    printf("Temperatura de referência: ");
    scanf("%f", &referenceTemperature);
    system("clear");
}

void parametersMenu() {
    printf("Kp: ");
    scanf("%lf", &kp);
    printf("Ki: ");
    scanf("%lf", &ki);
    printf("Kd: ");
    scanf("%lf", &kd);
    system("clear");
    pid_configura_constantes(kp, ki, kd);
}
