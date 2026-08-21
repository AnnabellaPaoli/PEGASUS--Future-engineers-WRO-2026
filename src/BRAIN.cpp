#include <Servo.h>
#include <SoftwareSerial.h>

Servo miServo;

// =========================================================================
// PARAMETROS DE CALIBRACION DE COMPETENCIA 
#define USAR_CAMARA 1        // 1 = CAmara activada para competencia | 0 = Desactivada para pruebas sin cAmara
const int CENTRO_SERVO = 90; 
int velocidadCamara = 120;   // Velocidad crucero alta en recta para ganar tiempo
// ==========================================

// CONFIGURACIÓN DE PINES ORIGINALES (ESTABLES Y PROBADOS)
const int trigIzq = 2,  echoIzq = 3;   
const int trigFront = 4, echoFront = 5; 
const int trigDer = 6,  echoDer = 7;   

const int pinServo = 9;
const int rxPin = 12; 
const int txPin = 13; 

// Pines del Puente H L298N
const int pinENA = 10; // Jumper puesto físico (permanece en HIGH interno)
const int pinIN1 = 8;  // Dirección digital (LOW avanzar, HIGH retroceder)
const int pinIN2 = 11; // CONTROL DE VELOCIDAD PWM (Pin 11 libre de conflicto)

#if USAR_CAMARA
SoftwareSerial espSerial(rxPin, txPin);
#endif

// Estados de la Máquina de Estados de TROYA
enum EstadoRobot { NAVEGANDO, UTURN, PARQUEANDO, DETENIDO };
EstadoRobot estadoActual = NAVEGANDO;

// Variables de Control de Competencia (3 Vueltas)
int contadorVueltas = 0;
bool sobreLineaMeta = false;
unsigned long tiempoUltimaVuelta = 0;
const unsigned long tiempoFiltroVuelta = 8000; 

// Parser Serie Asíncrono para ESP32-Cam
char serialBuffer[32];
int bufferIndex = 0;
bool datosNuevos = false;

// Variables de Recepción desde ESP32-Cam
int anguloCamara = 90;
int velocidadCamaraRecibida = 120;
int detectaMeta = 0;
int comandoEspecial = 0;

// Constantes de Control PD de Centrado Lateral (Estables)
long ultimoError = 0;
float Kp = 1.4; 
float Kd = 2.0; 

// Temporizadores para maniobras no bloqueantes (millis)
unsigned long tiempoInicioUTurn = 0;
int faseParqueo = 0; 
unsigned long tiempoPasoParqueo = 0;

void setup() {
  Serial.begin(9600);       // Puerto USB físico libre para depurar en la PC
  
  #if USAR_CAMARA
  espSerial.begin(38400);   // Enlace con la ESP32-Cam activo a velocidad de bits estable
  #endif
  
  miServo.attach(pinServo);
  
  pinMode(trigIzq, OUTPUT);   pinMode(echoIzq, INPUT);
  pinMode(trigFront, OUTPUT);  pinMode(echoFront, INPUT);
  pinMode(trigDer, OUTPUT);   pinMode(echoDer, INPUT);
  
  pinMode(pinENA, OUTPUT);
  pinMode(pinIN1, OUTPUT);
  pinMode(pinIN2, OUTPUT);
  
  // PROTECCIÓN DE SEGURIDAD ELÉCTRICA: Pin 10 en HIGH para evitar cortocircuito con el jumper físico
  digitalWrite(pinENA, HIGH); 
  
  controlarMotor(0); // Detener motor al iniciar
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

// CONTROL DE VELOCIDAD POR PIN 11 (PWM SEGURO CON CAPACIDAD DE REVERSA)
void controlarMotor(int velocidad) {
  if (velocidad > 0) {
    // Avanzar (IN1 en LOW, PWM en IN2)
    digitalWrite(pinIN1, LOW);
    analogWrite(pinIN2, velocidad); 
  } 
  else if (velocidad < 0) {
    // Retroceder con PWM en Pin 11 (Evita caídas de tensión que reinician tu placa)
    int velocidadAbs = abs(velocidad);
    velocidadAbs = constrain(velocidadAbs, 100, 150); // Rango de fuerza seguro en suelo
    
    digitalWrite(pinIN1, HIGH);
    analogWrite(pinIN2, 255 - velocidadAbs); // Inversión matemática para reversa PWM dócil
  } 
  else {
    // Detener por completo (Freno electrónico)
    digitalWrite(pinIN1, LOW);
    digitalWrite(pinIN2, LOW);
  }
}

// LECTURA SERIAL ASÍNCRONA
void leerSerialAsincrono() {
  #if USAR_CAMARA
  while (espSerial.available() > 0) {
    char c = espSerial.read();
    if (c == '<') {
      bufferIndex = 0; 
    } else if (c == '>') {
      serialBuffer[bufferIndex] = '\0'; 
      datosNuevos = true;
    } else if (bufferIndex < 31) {
      serialBuffer[bufferIndex++] = c;
    }
  }
  #endif
}

void loop() {
  // LECTURA SIMULTÁNEA DIRECTA CON RETARDO ESTABLE DE 15MS (La de tu versión "excelente")
  long distIzq = obtenerDistancia(trigIzq, echoIzq);
  delay(15); 
  long distFront = obtenerDistancia(trigFront, echoFront);
  delay(15); 
  long distDer = obtenerDistancia(trigDer, echoDer);

  // Variables que se calcularán en la máquina de estados
  int anguloFinal = CENTRO_SERVO;
  int velocidadFinal = 100;

  // Limpieza preventiva de eventos antes de procesar el puerto serie (Evita enganches de ruido estático)
  detectaMeta = 0;
  comandoEspecial = 0;

  // Leer datos desde la ESP32-Cam únicamente si el modo cámara está activo
  #if USAR_CAMARA
  leerSerialAsincrono();
  if (datosNuevos) {
    int camAng, camVel, camMeta, camEsp;
    // FILTRO DE SEGURIDAD: Solo procesa si sscanf lee exactamente los 4 enteros del formato
    if (sscanf(serialBuffer, "%d,%d,%d,%d", &camAng, &camVel, &camMeta, &camEsp) == 4) {
      anguloCamara = camAng;
      velocidadCamaraRecibida = camVel;
      detectaMeta = camMeta;
      comandoEspecial = camEsp;
    }
    datosNuevos = false;
  }
  #endif

  // Máquina de Estados de TROYA
  switch (estadoActual) {

    case NAVEGANDO:
      // Control PD de centrado de carril (Signo '+' probado y funcional)
      long errorParedes = distDer - distIzq; 
      long cambioError = errorParedes - ultimoError;
      int correccionPD = (errorParedes * Kp) + (cambioError * Kd);
      anguloFinal = CENTRO_SERVO + correccionPD; 
      ultimoError = errorParedes;

      #if USAR_CAMARA
      if (comandoEspecial == 1 || comandoEspecial == 2) {
        anguloFinal = anguloCamara;
      }
      #endif

      // MANIOBRA DE ESCAPE INTELIGENTE DE 2 FASES (Sujeta a variables estáticas no bloqueantes)
      static unsigned long tiempoEscape = 0;
      static bool ejecutandoEscape = false;
      static int ladoDespejado = 0; // 0 = Izquierdo libre, 1 = Derecho libre
      static unsigned long tiempoFinalizadoEscape = 0; // Temporizador para evitar bucles de ruido eléctrico

      // DETECCION DE COLISION TRIPLE (Frente < 25 cm, Izquierda < 10 cm, Derecha < 10 cm)
      bool colisionFrente = (distFront < 25);
      bool colisionLados = (distIzq < 10 || distDer < 10);

      // Solo activa el desatasco si hay colisión, no estamos escapando y ya pasó el tiempo de cooldown de 1.5s
      if ((colisionFrente || colisionLados) && !ejecutandoEscape && (millis() - tiempoFinalizadoEscape > 1500)) {
        ejecutandoEscape = true;
        tiempoEscape = millis();
        
        // Registrar en memoria instantánea qué lado de la pista tiene más espacio libre
        if (distDer > distIzq) ladoDespejado = 1; // Derecho libre
        else ladoDespejado = 0; // Izquierdo libre
      }

      if (ejecutandoEscape) {
        // FASE 1: Retroceder lo suficiente para no chocar (durante 1.2 segundos con potencia regulada)
        if (millis() - tiempoEscape < 1200) {
          velocidadFinal = -120; // Reversa dócil y potente en suelo sin caídas de tensión
          
          // Mientras retrocede, gira el timón de forma que la parte delantera apunte al lado libre
          if (ladoDespejado == 1) {
            anguloFinal = CENTRO_SERVO - 40; // Doblar a la izquierda para empujar la cola y apuntar la trompa a la derecha
          } else {
            anguloFinal = CENTRO_SERVO + 40; // Doblar a la derecha para empujar la cola y apuntar la trompa a la izquierda
          }
        }
        // FASE 2: Avanzar hacia adelante girando fuertemente hacia el lado libre (durante 500ms)
        else if (millis() - tiempoEscape < 1500) {
          velocidadFinal = 95; // Avanzar despacio para traccionar bien
          if (ladoDespejado == 1) {
            anguloFinal = CENTRO_SERVO + 40; // Doblar fuerte a la derecha para completar la evasión
          } else {
            anguloFinal = CENTRO_SERVO - 40; // Doblar fuerte a la izquierda para completar la evasión
          }
        }
        // FASE 3: Fin de la maniobra, registrar cooldown y restaurar navegación PD normal
        else {
          ejecutandoEscape = false;
          tiempoFinalizadoEscape = millis(); // Inicia el temporizador de cooldown de 1.5 segundos para estabilizar sensores
          ultimoError = 0; // Resetear historial del PD para un rearranque suave
        }
      } 
      else {
        // CONTROL DE VELOCIDAD PROPORCIONAL POR CERCANÍA FRONTAL (De 25cm a 60cm)
        if (distFront < 60) {
          // La velocidad se reduce gradualmente de 'velocidadCamara' a 95 conforme el muro se acerca
          float factorCercania = (distFront - 25.0) / (60.0 - 25.0);
          factorCercania = constrain(factorCercania, 0.0, 1.0);
          velocidadFinal = 95 + (factorCercania * (velocidadCamara - 95));
        } else {
          velocidadFinal = velocidadCamara; 
        }

        #if USAR_CAMARA
        // Desacelerar si la cámara detecta pilares (Evasión dinámica con ángulo cerrado)
        if (comandoEspecial == 1 || comandoEspecial == 2) {
          velocidadFinal = 100; 
        }
        #endif
        
        // Desacelerar proporcionalmente al ángulo de giro del servo (Curvas físicas cerradas)
        int desviacionTimon = abs(anguloFinal - CENTRO_SERVO);
        if (desviacionTimon > 10) { 
          int reduccion = desviacionTimon * 1.5; 
          velocidadFinal = velocidadFinal - reduccion;
        }

        // Límite de velocidad mínima en curvas para evitar atascos por fricción
        velocidadFinal = max(velocidadFinal, 105); 
      }

      // Conteo e incremento de vueltas con auto-limpieza
      if (detectaMeta == 1 && !sobreLineaMeta) {
        if (millis() - tiempoUltimaVuelta > tiempoFiltroVuelta) {
          contadorVueltas++;
          tiempoUltimaVuelta = millis();
          sobreLineaMeta = true;
          detectaMeta = 0; // Auto-limpieza del evento
          
          if (contadorVueltas >= 3) {
            estadoActual = PARQUEANDO;
            faseParqueo = 0; 
            velocidadFinal = 55;
            anguloFinal = CENTRO_SERVO;
          }
        }
      } else if (detectaMeta == 0) {
        sobreLineaMeta = false;
      }

      // Transición a U-Turn autónomo con auto-limpieza
      if (comandoEspecial == 3) {
        estadoActual = UTURN;
        tiempoInicioUTurn = millis(); 
        comandoEspecial = 0; // Auto-limpieza del evento
        
        anguloFinal = CENTRO_SERVO - 40;
        velocidadFinal = 80;
      }
      break;

    case UTURN:
      if (tiempoInicioUTurn == 0) {
        tiempoInicioUTurn = millis();
      }

      if (millis() - tiempoInicioUTurn < 1300) {
        anguloFinal = CENTRO_SERVO - 40; 
        velocidadFinal = 80; 
        if (distFront < 15) velocidadFinal = 0; 
      } else {
        estadoActual = NAVEGANDO; 
        tiempoInicioUTurn = 0; // Resetear temporizador de U-turn para seguridad
      }
      break;

    case PARQUEANDO:
      // EJECUCIÓN DE MANIOBRA DE PARQUEO EN PARALELO/DIAGONAL REVERSA (ACKERMANN)
      if (faseParqueo == 0) {
        if (distDer > 35 && comandoEspecial == 4) { 
          faseParqueo = 1; 
          tiempoPasoParqueo = millis();
          comandoEspecial = 0; // Auto-limpieza
        } else {
          velocidadFinal = 55; 
          long errParqueo = distDer - distIzq; 
          anguloFinal = CENTRO_SERVO + (errParqueo * 1.1);
        }
      }
      
      else if (faseParqueo == 1) {
        anguloFinal = CENTRO_SERVO; 
        velocidadFinal = 55; 
        if (millis() - tiempoPasoParqueo > 800) { 
          faseParqueo = 2;
          tiempoPasoParqueo = millis();
        }
      }

      else if (faseParqueo == 2) {
        anguloFinal = CENTRO_SERVO + 40; 
        velocidadFinal = -120; // Reversa de potencia controlada para parqueo
        if (millis() - tiempoPasoParqueo > 1200) { 
          faseParqueo = 3;
          tiempoPasoParqueo = millis();
        }
      }

      else if (faseParqueo == 3) {
        anguloFinal = CENTRO_SERVO - 40; 
        velocidadFinal = -120; // Reversa de potencia controlada para parqueo
        
        if (millis() - tiempoPasoParqueo > 1000 || distFront > 70) { 
          faseParqueo = 4;
        }
      }

      else if (faseParqueo == 4) {
        estadoActual = DETENIDO; 
      }
      break;

    case DETENIDO:
      velocidadFinal = 0;
      anguloFinal = CENTRO_SERVO;
      break;
  }

  // Restringir el ángulo del servo a los límites físicos del chasis (Simétrico al CENTRO_SERVO)
  anguloFinal = constrain(anguloFinal, CENTRO_SERVO - 40, CENTRO_SERVO + 40);

  // LECTURA DE TELEMETRÍA (Imprime los valores finales calculados en PC)
  Serial.print("Izq: "); Serial.print(distIzq);
  Serial.print("cm | Der: "); Serial.print(distDer);
  Serial.print("cm | Front: "); Serial.print(distFront);
  Serial.print("cm | Ang_Final: "); Serial.println(anguloFinal);

  // Enviar las órdenes físicas finales a los actuadores
  miServo.write(anguloFinal);
  controlarMotor(velocidadFinal); 

  delay(25); // Delay estable de 25ms para compatibilidad acústica
}