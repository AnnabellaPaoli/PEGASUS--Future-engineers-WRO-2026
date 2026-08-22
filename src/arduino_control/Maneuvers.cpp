#include <Arduino.h>
#include "RobotState.h"
#include "Maneuvers.h"
#include "Config.h"

void ejecutarUTurn(bool entrando, long distFront,
                    int &anguloFinal, int &velocidadFinal, EstadoRobot &estado) {
  static unsigned long tiempoInicio = 0;
  if (entrando) {
    tiempoInicio = millis();
  }

  if (millis() - tiempoInicio < 1300) {
    anguloFinal = CENTRO_SERVO - RANGO_SERVO; 
    velocidadFinal = 80;
    
    if (distFront < 15) velocidadFinal = 0;
  } else {
    estado = NAVEGANDO;
  }
}

void ejecutarParqueo(bool entrando, long distIzq, long distDer, long distFront,
                      const CameraData &camara,
                      int &anguloFinal, int &velocidadFinal, EstadoRobot &estado) {
  static int fase = 0;
  static unsigned long tiempoPaso = 0;
  static unsigned long tiempoInicioParqueo = 0;
  static bool huecoDetectado = false;
  static unsigned long tiempoHuecoDetectado = 0;

  if (entrando) {
    fase = 0;
    tiempoInicioParqueo = millis();
    tiempoPaso = millis();
    huecoDetectado = false;
  }

  // FASE 0: Búsqueda de cajón
  if (fase == 0) {
    velocidadFinal = 55;
    long errParqueo = distDer - distIzq;
    errParqueo = constrain(errParqueo, -20, 20);
    anguloFinal = CENTRO_SERVO + (errParqueo * 1.1); 

    if (distDer > DIST_DER_CAJON) {
      if (!huecoDetectado) {
        huecoDetectado = true;
        tiempoHuecoDetectado = millis();
      }
    } else {
      huecoDetectado = false;
    }

    bool huecoSostenido = huecoDetectado && (millis() - tiempoHuecoDetectado > 300);
    bool timeoutEmergencia = (millis() - tiempoInicioParqueo > 4000);

    if (huecoSostenido || camara.comandoEspecial == 4 || timeoutEmergencia) {
      fase = 1;
      tiempoPaso = millis();
    }
  } 
  // FASE 1: Superar cajón
  else if (fase == 1) {
    anguloFinal = CENTRO_SERVO;
    velocidadFinal = 55;
    
    if (millis() - tiempoPaso > 800 || distFront <= 30) {
      fase = 2;
      tiempoPaso = millis();
    }
  } 
  // FASE 2: Reversa con cola a la derecha
  else if (fase == 2) {
    anguloFinal = CENTRO_SERVO + RANGO_SERVO; 
    velocidadFinal = -120;
    
    if (millis() - tiempoPaso > 1200) {
      fase = 3;
      tiempoPaso = millis();
    }
  } 
  // FASE 3: Reversa con cola a la izquierda
  else if (fase == 3) {
    anguloFinal = CENTRO_SERVO - RANGO_SERVO; 
    velocidadFinal = -120;
    
    if (millis() - tiempoPaso > 1000) {
      fase = 4;
    }
  } 
  // FASE 4: Parqueado
  else if (fase == 4) {
    estado = DETENIDO;
  }
}

void ejecutarDetenido(int &anguloFinal, int &velocidadFinal) {
  velocidadFinal = 0;
  anguloFinal = CENTRO_SERVO;
}

// =========================================================
// FRENO META CON GOLPE DE REVERSA ACTIVO
// =========================================================
void ejecutarFrenoMeta(bool entrando, long distFront, int &anguloFinal, int &velocidadFinal, EstadoRobot &estado) {
  static unsigned long tiempoInicioFreno = 0;
  
  if (entrando) {
    tiempoInicioFreno = millis();
    Serial.println("--- ¡META DETECTADA! INICIANDO FRENADO ACTIVO ---");
  }

  anguloFinal = CENTRO_SERVO;

  // FASE 1: Contrapulso de reversa (180 ms) para vencer la inercia del motor
  if (millis() - tiempoInicioFreno < 180) {
    velocidadFinal = -100; 
  } 
  // FASE 2: Detención total
  else {
    velocidadFinal = 0; 
    estado = DETENIDO;  
    Serial.println("¡Vehículo detenido en seco sobre la meta!");
  }
}