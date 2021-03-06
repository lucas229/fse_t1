#ifndef MAININTERFACE_H
#define MAININTERFACE_H

#define RESISTOR_PIN 4
#define FAN_PIN 5
#define POTENTIOMETER_MODE 0
#define CURVE_MODE 1
#define TERMINAL_MODE 2

void initMenu();
void initDevices();
void enrollmentMenu();
void mainMenu();
void waitOven();
int potentiometerMode();
int curveMode();
int terminalMode();
void initGpio();
int calculateSignal(float internalTemperature);
FILE *getLogFile();
void logData(FILE *file, float internalTemperature, double externalTemperature, int signal);
int readCommand();
void referenceTemperatureMenu();
void parametersMenu();
void showValues();
void displayOff();
void updateDisplay(float internalTemperature, double externalTemperature);
void closeConnections();
void closeAll();
void showError();
void printData(float internalTemperature, double externalTemperature, int signal);

#endif
