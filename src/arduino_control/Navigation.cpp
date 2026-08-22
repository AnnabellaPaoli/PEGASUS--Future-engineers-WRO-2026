#include <Arduino.h>
#include "RobotState.h"
#include "Navigation.h"
#include "Config.h"

void ejecutarNavegando(long distIzq, long distFront, long distDer,
                       const CameraData &camara,
                       int &anguloFinal, int &velocidadFinal,
                       EstadoRobot &estado) {

  // =========================================================
  // VARIABLES ESTÁTICAS Y TIEMPO GLOBAL DE NAVEGACIÓN
  // =========================================================
  static int sentidoPista = TIPO_PISTA;
  static unsigned long tiempoInicioNavegacion = 0;

  if (tiempoInicioNavegacion == 0) {
    tiempoInicioNavegacion = millis();
  }

#if USAR_CAMARA
  if (sentidoPista == 0) {
    if (camara.comandoEspecial == 1) sentidoPista = 1;
    else if (camara.comandoEspecial == 2) sentidoPista = 2;
  }
#endif

  int direccionInterior;
  if (sentidoPista == 1) direccionInterior = -1;       // Antihorario -> Izquierda
  else if (sentidoPista == 2) direccionInterior = 1;   // Horario -> Derecha
  else direccionInterior = (distDer > distIzq) ? 1 : -1;

  bool inmunidadInicial = (millis() - tiempoInicioNavegacion < 300);

  // =========================================================
  // 1. ESCAPE DE EMERGENCIA (ÚNICA Y EXCLUSIVAMENTE FRONTAL)
  // =========================================================
  static int faseEscape = 0; 
  static unsigned long tiempoInicioFase = 0;
  static unsigned long tiempoUltimoEscape = 0;
  static int ladoLibre = -1; 
  static int lecturasPeligro = 0;

  // REGLA DE ORO: Solo activa escape si la pared FRONTAL está a menos de 22cm.
  // Jamás se activa por los sensores laterales.
  bool peligroFrontal = (distFront > 0 && distFront <= 22);

  if (peligroFrontal && !inmunidadInicial) {
    lecturasPeligro++;
  } else {
    lecturasPeligro = 0;
  }

  // Requiere 3 lecturas seguidas confirmadas para evitar falsos disparos por ruido
  if (lecturasPeligro >= 3 && faseEscape == 0 && (millis() - tiempoUltimoEscape > 1000)) {
    faseEscape = 1; 
    tiempoInicioFase = millis();
    ladoLibre = (distIzq >= distDer) ? -1 : 1; // Apunta al lado con más espacio
    lecturasPeligro = 0;
  }

  // FASE 1: Freno seco (100ms)
  if (faseEscape == 1) {
    if (millis() - tiempoInicioFase < 100) {
      velocidadFinal = 0;
      anguloFinal = CENTRO_SERVO + (-ladoLibre * RANGO_SERVO * DIRECCION_GIRO);
      return;
    } else {
      faseEscape = 2;
      tiempoInicioFase = millis();
    }
  }

  // FASE 2: Reversa cruzando ruedas (600ms)
  if (faseEscape == 2) {
    if (millis() - tiempoInicioFase < 600) {
      velocidadFinal = -120; 
      anguloFinal = CENTRO_SERVO + (-ladoLibre * RANGO_SERVO * DIRECCION_GIRO);
      return;
    } else {
      faseEscape = 3;
      tiempoInicioFase = millis();
    }
  }

  // FASE 3: Acomodo de servo en parado (100ms)
  if (faseEscape == 3) {
    if (millis() - tiempoInicioFase < 100) {
      velocidadFinal = 0;
      anguloFinal = CENTRO_SERVO + (ladoLibre * RANGO_SERVO * DIRECCION_GIRO);
      return;
    } else {
      faseEscape = 4;
      tiempoInicioFase = millis();
    }
  }

  // FASE 4: Salida hacia el lado libre (500ms)
  if (faseEscape == 4) {
    if (millis() - tiempoInicioFase < 500) {
      velocidadFinal = 90; 
      anguloFinal = CENTRO_SERVO + (ladoLibre * RANGO_SERVO * DIRECCION_GIRO);
      return;
    } else {
      faseEscape = 0;
      tiempoUltimoEscape = millis();
    }
  }

  // =========================================================
  // 2. NAVEGACIÓN PD CONTINUA (FLUIDA Y SIN SACUDIDAS)
  // =========================================================
  static long ultimoError = 0;

  // Acotamos valores a un rango de trabajo estable (10cm a 80cm)
  long distIzqPD = constrain(distIzq, 10L, 80L);
  long distDerPD = constrain(distDer, 10L, 80L);

  long errorParedes = distDerPD - distIzqPD;
  long cambioError = errorParedes - ultimoError;
  float steeringPD = (errorParedes * KP_PARED) + (cambioError * KD_PARED);
  ultimoError = errorParedes;

  // Transición suave cuando se acerca a una esquina o curva
  float factorCurva = 0.0f;
  if (distFront < DIST_FRONT_CURVA_LEJOS) {
    factorCurva = (float)(DIST_FRONT_CURVA_LEJOS - distFront) / (float)(DIST_FRONT_CURVA_LEJOS - DIST_FRONT_CURVA_CERCA);
    factorCurva = constrain(factorCurva, 0.0f, 1.0f);
  }

  float sesgoGiro = direccionInterior * RANGO_SERVO;
  
  // Mezcla limpia entre el centrado de paredes y la inclinación de curva
  float steeringTotal = (1.0f - factorCurva) * steeringPD + (factorCurva * sesgoGiro);

#if USAR_CAMARA
  if (HAY_BLOQUES_OBSTACULOS) {
    static unsigned long tiempoVision = 0;
    static int ultimoObjetoValido = 0;

    if (camara.comandoEspecial == 1 || camara.comandoEspecial == 2) {
      ultimoObjetoValido = camara.comandoEspecial;
      tiempoVision = millis();
    }
    
    if (millis() - tiempoVision < 350) {
      int factorProximidad = constrain(map(distFront, 50, 15, 20, 90), 20, 90);
      int offsetObjeto = (ultimoObjetoValido == 1) ? 25 : -25;
      steeringTotal += (offsetObjeto * factorProximidad / 100.0f);
    }
  }
#endif

  int anguloCalculado = CENTRO_SERVO + ((int)steeringTotal * DIRECCION_GIRO);
  anguloFinal = constrain(anguloCalculado, CENTRO_SERVO - RANGO_SERVO, CENTRO_SERVO + RANGO_SERVO);

  // CONTROL DE VELOCIDAD SUAVE POR PROXIMIDAD
  int velBase = VEL_CRUCERO;
  if (distFront < DIST_FRONT_CURVA_LEJOS) {
    velBase = map(distFront, DIST_FRONT_CURVA_LEJOS, DIST_FRONT_CURVA_CERCA, VEL_CRUCERO, 85);
    velBase = constrain(velBase, 85, VEL_CRUCERO);
  }

  // Reducción leve en giros para mantener tracción
  int desviacionTimon = abs(anguloFinal - CENTRO_SERVO);
  if (desviacionTimon > 12) {
    velBase -= (desviacionTimon * 0.15f);
  }

  velocidadFinal = constrain(velBase, 80, VEL_CRUCERO);

  // =========================================================
  // 3. CONTEO DE VUELTAS Y DETENCIÓN
  // =========================================================
  static int contadorVueltas = 0;
  static bool sobreLineaMeta = false;
  static unsigned long tiempoUltimaVuelta = 0;

  if (millis() - tiempoInicioNavegacion > 1500) {
    if (camara.detectaMeta == 1 && !sobreLineaMeta) {
      if (tiempoUltimaVuelta == 0 || (millis() - tiempoUltimaVuelta > 3000)) { 
        contadorVueltas++;
        tiempoUltimaVuelta = millis();
        sobreLineaMeta = true;

        if (contadorVueltas >= 3) {
          velocidadFinal = 0;          // Apaga motor
          anguloFinal = CENTRO_SERVO;  // Centra ruedas
          
          if (MODO_CON_PARQUEO) {
            estado = PARQUEANDO;
          } else {
            estado = FRENO_META;
          }
          return;
        }
      }
    } else if (camara.detectaMeta == 0) {
      sobreLineaMeta = false; 
    }
  }

  // =========================================================
  // 4. MANIOBRA U-TURN
  // =========================================================
  if (camara.comandoEspecial == 3) {
    estado = UTURN;
    anguloFinal = CENTRO_SERVO - (RANGO_SERVO * DIRECCION_GIRO);
    velocidadFinal = 80;
  }
}