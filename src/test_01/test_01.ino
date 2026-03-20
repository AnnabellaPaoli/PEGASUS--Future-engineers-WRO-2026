#include <Servo.h>

const int trigPin = 9;
const int echoPin = 10;
const int motorPWM = 5;
const int motorDir = 6;
const int servoPin = 3;

Servo direccionServo;

const int distanciaCurva = 70;  

//Variables para contar las 3 vueltas
int esquinasSuperadas = 0;
int vueltasCompletadas = 0;
bool enCurva = false; //Bandera para saber si el robot está girando actualmente

//Ángulos del servomotor 
const int anguloCentro = 90;    //Ruedas rectas
const int anguloGiro = 135;     //Girar a la izquierda (sentido antihorario)

void setup(){
  Serial.begin(9600);
  
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
  pinMode(motorPWM, OUTPUT);
  pinMode(motorDir, OUTPUT);
  
  direccionServo.attach(servoPin);
  direccionServo.write(anguloCentro);
  
  delay(2000); //Tiempo de espera antes de arrancar la ronda
  Serial.println("Iniciando Reto Abierto: Objetivo 3 Vueltas");
}

void loop(){
  // ---------------------------------------------------------
  // CONDICIÓN DE VICTORIA: 3 Vueltas (12 esquinas)
  // ---------------------------------------------------------
  if(vueltasCompletadas >= 3){
    direccionServo.write(anguloCentro);
    avanzarMotor(120);
    delay(1000); 
    
    detenerMotor();
    while(true); //Bucle infinito para finalizar la ronda(detención total de 30 seg o más)
  }

  // Leer sensor
  long distancia = medirDistancia();

  // ---------------------------------------------------------
  // LÓGICA DE NAVEGACIÓN EN LA PISTA
  // ---------------------------------------------------------
  if(distancia < distanciaCurva){
    //Si vemos el muro a menos de 70cm, estamos entrando a una sección de curva
    direccionServo.write(anguloGiro); //Girar dirección
    avanzarMotor(100);                //Reducir velocidad para no derrapar
    enCurva = true;                   //Marcamos que estamos doblando
    
  }else{
    //Camino libre por delante (> 70cm) significa que estamos en una recta
    direccionServo.write(anguloCentro); //Enderezar dirección
    avanzarMotor(150);                  //Acelerar en la recta

    //Si estábamos en una curva y ahora vemos la recta, ¡Superamos una esquina!
    if(enCurva == true){
      esquinasSuperadas++;
      enCurva = false; //Apagamos la bandera para no contar doble
      
      Serial.print("Esquina superada #");
      Serial.println(esquinasSuperadas);
      
      //Cada 4 esquinas equivale a 1 vuelta completa al tablero cuadrado
      if(esquinasSuperadas % 4 == 0){
        vueltasCompletadas++;
        Serial.print("¡Vuelta completada! Llevamos: ");
        Serial.println(vueltasCompletadas);
      }
    }
  }
}

//--- FUNCIONES AUXILIARES ---

long medirDistancia(){
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  
  long duracion = pulseIn(echoPin, HIGH);
  long distanciaCm = duracion * 0.034 / 2; 
  
  //Evitar falsos positivos. Si lee 0 o más de 3 metros (300cm), ignorar.
  if(distanciaCm == 0 || distanciaCm > 300){
    return 300; 
  }
  return distanciaCm;
}

void avanzarMotor(int velocidad){
  digitalWrite(motorDir, HIGH); 
  analogWrite(motorPWM, velocidad);
}

void detenerMotor(){
  analogWrite(motorPWM, 0); 
}
