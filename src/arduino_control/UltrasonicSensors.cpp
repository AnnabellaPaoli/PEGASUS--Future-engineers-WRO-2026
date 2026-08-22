#include <Arduino.h>
#include "UltrasonicSensors.h"
#include "Config.h"

void setupUltrasonicSensors() {
  pinMode(PIN_TRIG_IZQ, OUTPUT);   pinMode(PIN_ECHO_IZQ, INPUT);
  pinMode(PIN_TRIG_FRONT, OUTPUT); pinMode(PIN_ECHO_FRONT, INPUT);
  pinMode(PIN_TRIG_DER, OUTPUT);   pinMode(PIN_ECHO_DER, INPUT);
}

// Lectura de ultrasonidos estable con timeout de 20ms
long obtenerDistancia(int trigPin, int echoPin) {
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  long duracion = pulseIn(echoPin, HIGH, 20000); // 20ms de espera segura
  if (duracion == 0) return 300;
  return duracion * 0.034 / 2;
}

void leerTresSensores(long &distIzq, long &distFront, long &distDer) {
  distIzq = obtenerDistancia(PIN_TRIG_IZQ, PIN_ECHO_IZQ);
  delay(15);
  distFront = obtenerDistancia(PIN_TRIG_FRONT, PIN_ECHO_FRONT);
  delay(15);
  distDer = obtenerDistancia(PIN_TRIG_DER, PIN_ECHO_DER);
}
