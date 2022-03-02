#ifndef MAININTERFACE_H
#define MAININTERFACE_H

#include "modbus.h"

unsigned char enrollment[ENROLLMENT_LENGTH];
double kp, ki, kd;
float referenceTemperature;

void initMenu();
void getEnrollmentMenu();
void mainMenu();
void startOven();
void referenceTemperatureMenu();
void externalPotentiometerMenu();
void keyboardInputMenu();
void temperatureCurveMenu();
void parametersMenu();

#endif
