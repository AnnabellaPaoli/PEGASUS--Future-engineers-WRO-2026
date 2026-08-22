#include <Arduino.h>
#include "MotorControl.h"
#include "Config.h"

void setupMotor() {
  pinMode(PIN_ENA, OUTPUT);
  pinMode(PIN_IN1, OUTPUT);
  pinMode(PIN_IN2, OUTPUT);
  
  // Protección eléctrica: Inicializamos Pin 10 en HIGH para coincidir con tu jumper físico de 5V lógicos
  digitalWrite(PIN_ENA, HIGH); 
  
  digitalWrite(PIN_IN1, LOW);
  digitalWrite(PIN_IN2, LOW);
}

// CONTROL DE VELOCIDAD POR PIN 11 CON PWM (Evita caídas de tensión y reinicios en suelo)
void controlarMotor(int velocidad) {
  if (velocidad > 0) {
    // Avanzar (IN1 en LOW, PWM de velocidad en IN2)
    digitalWrite(PIN_IN1, LOW);
    analogWrite(PIN_IN2, velocidad); 
  } 
  else if (velocidad < 0) {
    // Retroceder con PWM dócil en Pin 11 (Alineado con tus límites de reversa de 100 a 150)
    int velocidadAbs = abs(velocidad);
    velocidadAbs = constrain(velocidadAbs, 100, 150); 
    
    digitalWrite(PIN_IN1, HIGH);
    analogWrite(PIN_IN2, 255 - velocidadAbs); // Inversión matemática para reversa PWM dócil
  } 
  else {
    // Detener por completo (Freno electrónico)
    digitalWrite(PIN_IN1, LOW);
    digitalWrite(PIN_IN2, LOW);
  }
}