#ifndef ULTRASONIC_SENSORS_H
#define ULTRASONIC_SENSORS_H

void setupUltrasonicSensors();
long obtenerDistancia(int trigPin, int echoPin);

// Lee los 3 sensores en secuencia con el retardo estable de 15ms entre lecturas.
void leerTresSensores(long &distIzq, long &distFront, long &distDer);

#endif
