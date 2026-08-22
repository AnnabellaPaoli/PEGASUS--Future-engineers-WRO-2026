#include <Arduino.h>
#include <Servo.h>
#include "Config.h"
#include "RobotState.h"
#include "CameraLink.h"
#include "UltrasonicSensors.h"
#include "MotorControl.h"
#include "Navigation.h"
#include "Maneuvers.h"

Servo miServo;
EstadoRobot estadoActual = NAVEGANDO;

// Variables globales para la memoria de los sensores
long distIzqAnt = 100;
long distFrontAnt = 100;
long distDerAnt = 100;

void setup() {
  Serial.begin(9600);

  setupCameraLink();
  miServo.attach(PIN_SERVO);
  setupUltrasonicSensors();
  setupMotor();

  controlarMotor(0);
  miServo.write(CENTRO_SERVO);

  // 1. Calentamiento y estabilización inicial de sensores
  Serial.println("Calentando sensores...");
  for (int i = 0; i < 5; i++) {
    long izqDummy, frontDummy, derDummy;
    leerTresSensores(izqDummy, frontDummy, derDummy);
    delay(50);
  }

  // =========================================================
  // 2. BOTÓN VIRTUAL ANTI-RUIDO (CONFIRMACIÓN DE 3 LECTURAS)
  // =========================================================
  Serial.println("--- MODO STANDBY ACTIVADO ---");
  Serial.println("Coloque la mano frente al sensor frontal (< 8 cm)...");

  long izqWait, frontWait, derWait;
  
  // Paso A: Esperar detección de mano (3 lecturas consecutivas)
  int lecturasManoCerca = 0;
  while (lecturasManoCerca < 3) {
    leerTresSensores(izqWait, frontWait, derWait);
    
    if (frontWait >= 2 && frontWait <= 8) {
      lecturasManoCerca++;
    } else {
      lecturasManoCerca = 0;
    }
    delay(60);
  }

  Serial.println("¡Mano detectada! Retire la mano para arrancar...");

  // Paso B: Esperar a retirar la mano (3 lecturas despejadas)
  int lecturasManoRetirada = 0;
  while (lecturasManoRetirada < 3) {
    leerTresSensores(izqWait, frontWait, derWait);
    
    if (frontWait > 15) {
      lecturasManoRetirada++;
    } else {
      lecturasManoRetirada = 0;
    }
    delay(60);
  }

  // Reset de memoria y limpieza de puerto Serie antes del arranque
  distIzqAnt = 100;
  distFrontAnt = 100;
  distDerAnt = 100;

  while (Serial.available() > 0) {
    Serial.read();
  }

  Serial.println("¡Orden de salida confirmada! Arrancando...");
  delay(400);
}

void loop() {
  // 1. LEER CÁMARA
  resetEventosCamara();
  leerCamara();

  // 2. LEER SENSORES FÍSICOS
  long distIzq, distFront, distDer;
  leerTresSensores(distIzq, distFront, distDer);

  // =========================================================
  // 3. FILTRO DE SENSORES ANTI-FALSOS DISPAROS Y ANTI-CHOQUE
  // Si mide <= 0 y venía midiendo cerca (< 30cm), es un choque real (1cm).
  // Si no venía cerca o mide > 300cm, es vía libre o error sin eco (400cm).
  // =========================================================
  if (distIzq <= 0) {
    distIzq = (distIzqAnt < 30) ? 1 : 400; 
  } else if (distIzq > 300) {
    distIzq = 400; 
  } else {
    distIzqAnt = distIzq;
  }

  if (distFront <= 0) {
    distFront = (distFrontAnt < 30) ? 1 : 400; 
  } else if (distFront > 300) {
    distFront = 400; 
  } else {
    distFrontAnt = distFront;
  }

  if (distDer <= 0) {
    distDer = (distDerAnt < 30) ? 1 : 400; 
  } else if (distDer > 300) {
    distDer = 400; 
  } else {
    distDerAnt = distDer;
  }

  // =========================================================
  // 4. DECLARACIÓN DE VARIABLES DE CONTROL
  // (Ubicadas antes del switch para evitar errores de scope)
  // =========================================================
  int anguloFinal = CENTRO_SERVO;
  int velocidadFinal = VEL_CRUCERO;

  static EstadoRobot estadoAnterior = DETENIDO;
  bool entrando = (estadoActual != estadoAnterior);
  estadoAnterior = estadoActual;

  // =========================================================
  // 5. MÁQUINA DE ESTADOS PRINCIPAL
  // =========================================================
  switch (estadoActual) {
    case NAVEGANDO:
      ejecutarNavegando(distIzq, distFront, distDer, camara, anguloFinal, velocidadFinal, estadoActual);
      break;

    case UTURN:
      ejecutarUTurn(entrando, distFront, anguloFinal, velocidadFinal, estadoActual);
      break;

    case PARQUEANDO:
      ejecutarParqueo(entrando, distIzq, distDer, distFront, camara, anguloFinal, velocidadFinal, estadoActual);
      break;

    case FRENO_META:
      ejecutarFrenoMeta(entrando, distFront, anguloFinal, velocidadFinal, estadoActual); 
      break;

    case DETENIDO:
    default:
      ejecutarDetenido(anguloFinal, velocidadFinal);
      break;
  }

  // Restricción de límites del servo
  anguloFinal = constrain(anguloFinal, CENTRO_SERVO - RANGO_SERVO, CENTRO_SERVO + RANGO_SERVO);

  // =========================================================
  // 6. RAMPA SLEW RATE (Protección del servo MG996R)
  // =========================================================
  static int anguloFiltrado = CENTRO_SERVO;
  static unsigned long ultimoTiempoSlew = 0;
  const int PASO_MAXIMO = 4; 

  if (millis() - ultimoTiempoSlew > 15) {
    if (abs(anguloFinal - anguloFiltrado) > PASO_MAXIMO) {
      if (anguloFinal > anguloFiltrado) anguloFiltrado += PASO_MAXIMO;
      else anguloFiltrado -= PASO_MAXIMO;
    } else {
      anguloFiltrado = anguloFinal;
    }
    ultimoTiempoSlew = millis();
  }

  // =========================================================
  // 7. TELEMETRÍA POR PUERTO SERIE
  // =========================================================
  Serial.print("E: "); Serial.print(estadoActual);
  Serial.print(" | I: "); Serial.print(distIzq);
  Serial.print(" | F: "); Serial.print(distFront);
  Serial.print(" | D: "); Serial.print(distDer);
  Serial.print(" | Meta: "); Serial.print(camara.detectaMeta);
  Serial.print(" | Vel: "); Serial.print(velocidadFinal);
  Serial.print(" | Ang: "); Serial.println(anguloFiltrado);

  // =========================================================
  // 8. ESCRITURA EN ACTUADORES
  // =========================================================
  miServo.write(anguloFiltrado);
  controlarMotor(velocidadFinal);

  delay(5);
}