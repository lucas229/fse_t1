#ifndef MAININTERFACE_H
#define MAININTERFACE_H

#define ENROLLMENT_LENGTH 4

unsigned char enrollment[ENROLLMENT_LENGTH];
double kp, ki, kd;
float referenceTemperature;

void initMenu();
void getEnrollmentMenu();
void mainMenu();
void oven();
void referenceTemperatureMenu();
void externalPotentiometerMenu();
void keyboardInputMenu();
void temperatureCurveMenu();
void parametersMenu();

#endif
