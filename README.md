# PEGASUS - WRO Future Engineers 2026

Este repositorio contiene el desarrollo del robot autónomo **TROYA** de nuestro equipo **PEGASUS**, diseñado para competir en la categoría de Futuros Ingenieros en la subcategoría **SENIOR** de la WRO 2026.

## Estructura del Proyecto

*   `/src`: Códigos fuente para la ESP32-Cam (visión artificial) y Arduino Uno (control y actuadores).
*   `/scheme`: Diagramas de conexión y diseño de la distribución eléctrica.
*   `/models`: Archivos de diseño en 3D del chasis y piezas personalizadas.
*   `/t-photos`: Registro fotográfico del equipo PEGASUS.
*   `/v-photos`: Registro fotográfico del robot TROYA.
*   `/video`: Archivos y enlaces del video demostrativo de funcionamiento autónomo.
*   `/documentation`: Reportes de ingeniería detallados, bitácora de pruebas y calibración.

---

## Arquitectura del Sistema

**TROYA** utiliza una arquitectura de procesamiento distribuido para maximizar la eficiencia de los recursos de bajo costo:

*   **ESP32-Cam:** Dedicada exclusivamente al procesamiento de visión computacional y toma de decisiones lógicas de alto nivel.
*   **Arduino Uno:** Dedicado al control en tiempo real de los actuadores (motor DC y servo dirección) y a la lectura síncrona de los sensores de distancia ultrasónicos.

### Diagrama de Bloques del Hardware 

El siguiente diagrama detalla la distribución de energía (partiendo de una configuración de 3 baterías 18650 que entregan un voltaje nominal de ~11.1V) y el flujo de señales de control:

```mermaid
graph TD
    Bateria[Batería 18650 x 3 ~11.1V] --> |Alimentación directa| PuenteH[Puente H L298N / TB6612FNG]
    Bateria --> |Carga| PuertoCarga[Puerto de Carga]
    Bateria --> |Línea de Potencia| Regulador[Regulador de Voltaje LM2596]
    Regulador --> |5V Regulados| Arduino[Arduino Uno]
    Regulador --> |5V Regulados| ESP32[ESP32-Cam]
    
    ESP32 --> |UART Serial TX/RX| Arduino
    
    %% Sensores Ultrasónicos conectados al Arduino
    US_Izq[Ultrasonico Izquierdo] --> |Trigger/Echo| Arduino
    US_Cent[Ultrasonico Frontal] --> |Trigger/Echo| Arduino
    US_Der[Ultrasonico Derecho] --> |Trigger/Echo| Arduino
    
    Arduino --> |Señal PWM| Servo[Servo Motor Dirección]
    Arduino --> |Señal PWM y Dirección| PuenteH
    PuenteH --> |Energía| MotorDC[Motor DC Tracción]

### Lógica de Control y Navegación Autónomo

El sistema de control de **TROYA** utiliza un enfoque de **fusión de sensores** y **control proporcional (P)** para resolver de forma estable los desafíos de navegación, evasión de obstáculos y estacionamiento en un chasis con dirección tipo Ackermann.

---

### 1. Fusión de Sensores y Prioridades de Control

Para optimizar el uso de los recursos de hardware limitados (Arduino Uno y ESP32-Cam), se implementó una arquitectura de control distribuida con asignación de prioridades:

*   **Procesamiento de Visión (ESP32-Cam):** Se encarga de la clasificación de color de obstáculos (rojo/verde) y de proponer trayectorias de giro basadas en el color detectado
*   **Control de Seguridad y Distancia (Arduino Uno):** Lee en tiempo real los 3 sensores ultrasónicos para el centrado milimétrico respecto a las paredes laterales y actúa como un sistema de freno autónomo de emergencia si la cámara experimenta retraso (lag).

El flujo de prioridades se ejecuta bajo el siguiente criterio:

graph TD
    A[Lectura de Sensores Ultrasónicos] --> B{¿Obstáculo frontal < 20cm?}
    B -- Sí --> C[Prioridad 1: Detener Motor / Seguridad]
    B -- No --> D{¿Pared lateral muy cercana < 10cm?}
    D -- Sí --> E[Prioridad 2: Corrección Física por Ultrasónicos]
    D -- No --> F[Prioridad 3: Seguir Trayectoria propuesta por Cámara]

Si no hay riesgos físicos inmediatos detectados por los sensores, el control del movimiento se delega a las decisiones lógicas procesadas por la cámara.

flowchart TD
    Start([Inicio]) --> LeerSensores[Leer 3 Sensores Ultrasónicos]
    LeerSensores --> LeerCamara[Recibir Datos de la Cámara]
    
    %% Decisión de Seguridad Frontal
    LeerSensores --> Det_Frente{¿Obstáculo Frontal < 15cm?}
    Det_Frente -- Sí --> EvasionUrgente[Maniobra de Evasión / Frenado]
    
    %% Navegación Normal
    Det_Frente -- No --> AnalizarCamara{¿Cámara detecta bloque o carril?}
    
    AnalizarCamara -- Sí (Rojo/Verde) --> PlanificarGiro[Calcular trayectoria de evasión]
    AnalizarCamara -- No --> MantenerCarril[Centrado en Carril usando Paredes Izq/Der]
    
    EvasionUrgente --> EnviarActuadores[Enviar señales a Servo y Motor]
    PlanificarGiro --> EnviarActuadores
    MantenerCarril --> EnviarActuadores
    EnviarActuadores --> LeerSensores

El robot combina las lecturas visuales de la ESP32-Cam con las distancias medidas por los tres sensores ultrasónicos para navegar de forma segura.

