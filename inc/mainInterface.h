#ifndef MAININTERFACE_H
#define MAININTERFACE_H

#define RESISTOR_PIN 4
#define FAN_PIN 5
#define POTENTIOMETER_MODE 0
#define CURVE_MODE 1
#define TERMINAL_MODE 2

void initMenu();
void enrollmentMenu();
void mainMenu();
void waitOven();
void initOven();
void initGpio();
int calculateSignal(int internalTemperature);
int getLogFile();
void logData(int file, float internalTemperature, double externalTemperature, int resistorSignal, int fanSignal);
int readCommand();
void referenceTemperatureMenu();
void parametersMenu();
void showValues();
void lcdOff();
void updateDisplay(float internalTemperature, double externalTemperature);
void closeConnections();

#endif
