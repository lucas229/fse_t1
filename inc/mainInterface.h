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
void initOven();
void initGpio();
int calculateSignal(float internalTemperature);
int getLogFile();
void logData(int file, float internalTemperature, double externalTemperature, int signal);
int readCommand();
void referenceTemperatureMenu();
void parametersMenu();
void showValues();
void displayOff();
void updateDisplay(float internalTemperature, double externalTemperature);
void closeConnections();
void closeAll();
void showError();

#endif
