#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

// =============================================================================
// --- PINES DE HARDWARE (TROYA WRO Senior) ---
// =============================================================================

// Sensores Ultrasónicos (HC-SR04)
const int PIN_TRIG_IZQ   = 2;
const int PIN_ECHO_IZQ   = 3;
const int PIN_TRIG_FRONT = 4;
const int PIN_ECHO_FRONT = 10; 
const int PIN_TRIG_DER   = 6;
const int PIN_ECHO_DER   = 7;

// Pin del Servo de Dirección (MG996R)
const int PIN_SERVO      = 9;

// Comunicación con ESP32-Cam (SoftwareSerial)
const int PIN_RX_CAMARA  = A0; 
const int PIN_TX_CAMARA  = 12; 

// Puente H L298N
const int PIN_ENA = 5;  
const int PIN_IN1 = 8;
const int PIN_IN2 = 11;

// --- PARÁMETROS DE DIRECCIÓN (MG996R) ---
const int CENTRO_SERVO = 85;  // Centro calibrado físico
const int RANGO_SERVO = 40;   // Permitido de 45° a 125°

const int CLAMP_NAV      = 20; // Rango para centrado seguro en recta (70°-110°)
const int CLAMP_MANIOBRA = 25; // Rango máximo seguro para maniobras (65°-115°)

// --- PARÁMETROS NAVEGACIÓN Y PD ---
const float KP_PARED = 1.5f;
const float KD_PARED = 0.8f;
const int DIRECCION_GIRO = 1;

// CONFIGURACIÓN DE PISTA: 0 = Auto, 1 = Antihorario (Curvas Izq), 2 = Horario (Curvas Der)
const int TIPO_PISTA = 2; 

const int VEL_CRUCERO = 130;
const unsigned long TIEMPO_FILTRO_VUELTA = 8000;
const bool USAR_CAMARA = true;

// --- CONSTANTES DE CALIBRACIÓN DE COMPETENCIA ---
const int VEL_MIN_CURVA = 105;
const int VEL_EVASION = 100;
const int VEL_ESCAPE = 150;           // CORREGIDO: Subida potencia de reversa para un retroceso firme en suelo [2]
const int VEL_UTURN = 80;
const int VEL_BUSQUEDA_PARQUEO = 55;
const int VEL_REVERSA_PARQUEO = 130;

const unsigned long HOLD_EVASION_MS = 300;
const unsigned long ESCAPE_REVERSA_MS = 1600; // CORREGIDO: Aumentada duración del retroceso a 1.6s para retroceder lo suficiente [2]
const unsigned long UTURN_MS = 1300;
const unsigned long PARQUEO_FASE1_MS = 800;
const unsigned long PARQUEO_FASE2_MS = 1200;
const unsigned long PARQUEO_FASE3_MS = 1000;
const unsigned long COOLDOWN_REVERSA_MS = 1500;
const unsigned long PERIODO_SENSOR_MS = 15;

const long DIST_FRONT_CURVA_LEJOS = 95; // Arranca el frenado y guiado de curva a los 95cm (Antes 60) [2]
const long DIST_FRONT_CURVA_CERCA = 25; 
const long DIST_FRONT_ESCAPE      = 25; // Activa reversa a los 25cm
const long DIST_LATERAL_ESCAPE    = 10; // Activa reversa si raspa las paredes laterales a menos de 10cm
const long DIST_DER_CAJON         = 35;
const long DIST_FRONT_PARQUEO_FIN = 15;
const unsigned long ECO_TIMEOUT_US = 20000UL;

// RESTAURADO: Mapeo de la cámara original en fase con tu servo (Rojo=100-115 Derecha, Verde=65-80 Izquierda) [2]
const int ANGULO_ROJO_MIN = 100,  ANGULO_ROJO_MAX = 115;   
const int ANGULO_VERDE_MIN = 65, ANGULO_VERDE_MAX = 80;  

// Selector de modo de competencia
const bool MODO_CON_PARQUEO = false;   
const bool HAY_BLOQUES_OBSTACULOS = false; 

#endif