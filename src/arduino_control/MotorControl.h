#ifndef MOTOR_CONTROL_H
#define MOTOR_CONTROL_H

void setupMotor();

// Velocidad positiva = avanzar, negativa = retroceder, 0 = freno.
void controlarMotor(int velocidad);

#endif
