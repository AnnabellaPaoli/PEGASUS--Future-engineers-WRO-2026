# 📓 Bitácora de Desarrollo y Resolución de Conflictos (Engineering Journal)

**Equipo:** PEGASUS  
**Robot:** TROYA  
**Categoría:** WRO 2026 - Futuros Ingenieros (Senior)  

Este documento constituye el diario de desarrollo, pruebas y calibración del robot autónomo **TROYA**. En él se describen de manera cronológica y técnica los problemas detectados en pista, las hipótesis planteadas, la metodología de descarte y las soluciones finales aplicadas tanto en hardware como en software.

---

## 🛠️ Sección 1: Desafíos Mecánicos y de Chasis

### 1.1 Fractura por Fatiga en Mangueta Izquierda (Dirección Ackermann)
*   **Problema Detectado:** Durante las pruebas dinámicas de giro en el suelo, la mangueta izquierda (el pivote que conecta la llanta delantera al sistema de dirección) se fracturó físicamente debido al esfuerzo y torque abrupto del servo motor.
*   **Hipótesis 1 (Fallo):** Reparar la pieza con adhesivo instantáneo de cianoacrilato.  
    *   *Resultado:* La pieza falló de inmediato al primer giro debido a que la vibración y la fuerza de tracción del hule sobre el suelo superaron la resistencia a la tensión del pegamento.
*   **Hipótesis 2 (Fallo):** Reparar la pieza y unirla al manguito del servo con soldadura epóxica líquida de acero (*Pegatanque*).  
    *   *Resultado:* El cuerpo de la mangueta resistió, pero al volverse extremadamente rígida y carecer de flexibilidad, la fuerza de torsión mecánica se concentró en la articulación con el servo motor, quebrando esa zona de unión.
*   **Solución de Ingeniería Aplicada:** Evaluamos fabricar la pieza en madera o metal, pero se descartaron por problemas de peso y dificultad de fabricación manual. Descubrimos que las **tarjetas de crédito de PVC vencidas** poseían la relación de rigidez y flexibilidad elástica requerida. El PVC tiene la resistencia necesaria para guiar las ruedas delanteras de forma recta, pero la flexibilidad justa para absorber las sacudidas dinámicas del servo sin quebrarse.
*   **Procedimiento:** Se cortó el plástico remanente de la mangueta original, se lijó la superficie, se moldeó la tarjeta de crédito con calor a la forma exacta requerida, se adaptó y se fijó mediante tornillos autorroscantes directamente al servo. La dirección se comporta de forma elástica y robusta ante cualquier maniobra.

---

## ⚡ Sección 2: Desafíos Eléctricos y de Potencia

### 2.1 Incidente Crítico en Puerto de Carga Integrado (Fallo de Seguridad)
*   **Problema Detectado:** Diseñamos un puerto de carga XT60/Jack en paralelo a la batería para cargar las celdas 18650 directamente en el chasis. Durante una calibración, un fallo de aislamiento y diseño en el puerto de carga provocó un cortocircuito directo. El microcontrolador sufrió daños irreversibles por alta temperatura y el banco de baterías estuvo a punto de explotar por sobrecalentamiento.
*   **Hipótesis (Fallo):** Aislar con silicona caliente el puerto y seguir usándolo de forma integrada.  
    *   *Resultado:* Se descartó por completo debido al alto riesgo de incendio de las celdas de litio durante competencias cerradas de WRO.
*   **Solución de Ingeniería Aplicada:** Retiramos por completo el puerto de carga integrado. Implementamos un sistema de carga seguro por **extracción física**. Diseñamos portabaterías con resortes y retiramos las tres celdas 18650 al final de cada ronda para cargarlas de forma externa y controlada utilizando un cargador de balanceo inteligente con corte automático de corriente por sobrecarga.

### 2.2 Selección de Energía: 3 Celdas Li-ion 18650 vs. Baterías LiPo
*   **La Decisión:** Decidimos utilizar 3 celdas Li-ion 18650 en serie (voltaje nominal de ~11.1V, cargadas a ~12.6V) en lugar de una batería LiPo de aeromodelismo.
*   **La Razón:** Las celdas 18650 son individuales, robustas, sumamente económicas y fáciles de transportar y reemplazar en pits. No sufren de hinchamiento (*swelling*) y, al cargarse externamente de forma individual, eliminan el riesgo de incendios térmicos en el chasis del robot, un factor crítico tras la experiencia del cortocircuito previo.

---

### 💻 Sección 3: Desafíos de Software y Arquitectura de Control

### 3.1 Conflicto de Timer 1 entre la Librería `Servo.h` y el PWM en Pin 10 (`ENA`)
*   **Problema Detectado:** Al cablear originalmente el Puente H L298N, conectamos el pin `ENA` al pin 10 del Arduino para controlar la velocidad del carro por PWM. El motor de tracción se quedaba totalmente quieto cuando el servo de dirección (Pin 9) estaba activo.
*   **Análisis y Causa Raíz:** En la arquitectura AVR del ATmega328P del Arduino Uno, la librería estándar de servos toma el control absoluto del **Timer 1** (un temporizador interno por hardware). Al adueñarse de este reloj, **deshabilita la capacidad de generar señales PWM (`analogWrite()`) en los pines 9 y 10**.
*   **Solución de Ingeniería Aplicada:** Dejamos el jumper negro físico del Puente H puesto permanentemente (lo que mantiene `ENA` siempre en HIGH, o 5V lógicos). Para no perder el control de la velocidad, reescribimos la lógica del software de control del motor para realizar la modulación de velocidad PWM directamente en el **Pin 11 (`IN2`)**, el cual es controlado de manera independiente por el **Timer 2**, eliminando por completo el conflicto con el servo de dirección.

### 3.2 Desbordamiento del Búfer de SoftwareSerial y el Bug de "Coche Zombie"
*   **Problema Detectado:** Al añadir retardos de `delay(15)` entre sensores para evitar ecos acústicos, el carro comenzó a correr en línea recta indefinidamente a máxima velocidad, ignorando las paredes y negándose a frenar.
*   **Análisis y Causa Raíz:** El retardo de los sensores aumentaba el tiempo total del loop principal a 55 ms. La ESP32-Cam transmite datos serie a una alta velocidad de **38400 baudios**. Durante los 55 ms que el Arduino dormía, se acumulaban más de 200 caracteres de datos en la entrada. El búfer físico de `SoftwareSerial` solo soporta **64 bytes**, por lo que **se desbordaba constantemente**. Al desbordarse, la función `sscanf` procesaba datos incompletos o corruptos, provocando una corrupción de pila (Stack Corruption) en la RAM del Arduino. Esto provocaba que el microcontrolador ejecutara las salidas físicas fijas e ignorara las condicionales lógicas de los sensores de distancia.
*   **Solución de Ingeniería Aplicada:** Eliminamos todos los delays intermedios de los ultrasonidos en el bucle principal. Adicionalmente, agregamos un filtro de validación de datos: `if (sscanf(...) == 4)`. Ahora, el Arduino solo actualiza las variables de la cámara si la trama de datos serie contiene exactamente los 4 enteros del formato esperado; de lo contrario, la descarta. Esto mantiene el búfer limpio y el procesador ejecuta el control con total fluidez.

---

### 🔊 Sección 4: Desafíos de Física de Sensores e Interferencia

### 4.1 Fenómeno de Absorción Acústica de la Tela (El Misterio del Cobertor)
*   **Problema Detectado:** Durante las pruebas en casa, el carro esquivaba con precisión cajas de cartón y carpetas duras, pero al aproximarse al cobertor textil de los sillones se estrellaba de frente y se quedaba patinando.
*   **Análisis y Causa Raíz:** Los sensores ultrasónicos (HC-SR04) miden distancia basándose en el rebote de ondas de sonido. Los materiales rígidos (madera, plástico, cartón) reflejan las ondas acústicas perfectamente. Sin embargo, los materiales textiles y blandos (telas, acolchados) actúan como **aislantes acústicos**, absorbiendo la onda de sonido y evitando que rebote hacia el sensor. Al no recibir eco, el Arduino registraba `0` de duración (interpretado por código como infinito o `300 cm`), asumiendo pista libre y acelerando de frente.
*   **Solución de Ingeniería Aplicada:** Se estandarizó el protocolo de pruebas del equipo para utilizar **únicamente obstáculos con superficies duras** (madera, plástico, cartón), simulando fielmente los materiales reales con los que están construidas las paredes y pilares oficiales de la pista de la WRO.

### 4.2 Atasque por Resistencia a la Rodadura en Curvas (Stall a 85 PWM)
*   **Problema Detectado:** En rectas el carro corría bien, pero al entrar a zonas muy angostas del pasillo donde debía corregir constantemente, el robot se detenía por completo y se quedaba en silencio.
*   **Análisis y Causa Raíz:** En una dirección tipo Ackermann, cuando las ruedas delanteras giran con ángulos pronunciados, el chasis experimenta un **aumento drástico en la resistencia a la rodadura (fricción por arrastre)**. En nuestro algoritmo de curvas, permitíamos reducir la velocidad del motor trasero hasta un mínimo de `85` PWM. Esa potencia era incapaz de vencer la fricción de las llantas dobladas y el peso de las celdas 18650, provocando un bloqueo por torque (*stall*).
*   **Solución de Ingeniería Aplicada:** Ajustamos el límite de velocidad mínima en curvas del software a **`105` PWM**. Este incremento proporciona la fuerza exacta para empujar el carro con las ruedas totalmente dobladas en zonas estrechas sin atascarse.

### 4.3 El Bucle de Interferencia de Reversa por Ruido Eléctrico (EMI Loop)
*   **Problema Detectado:** En las pruebas iniciales del algoritmo de escape (reversa), al colocar la mano enfrente en el suelo, el carro se quedaba quieto; pero al levantarlo del suelo con las llantas en el aire, se quedaba pegado en reversa de forma infinita a pesar de ya no tener nada enfrente.
*   **Análisis y Causa Raíz:** Este fallo tenía dos causas combinadas:
    1.  *En el suelo:* Al usar `digitalWrite(IN1, HIGH)` directo para reversa de máxima potencia, el motor demandaba un pico de corriente tan alto bajo peso real que la batería sufría una caída de tensión, **reiniciando continuamente al Arduino** (por eso se quedaba quieto).
    2.  *En el aire:* Al no haber peso, el Arduino no se reiniciaba, pero el chisporroteo eléctrico de las escobillas del motor girando en reversa generaba **interferencia electromagnética (EMI)**. Los cables de los sensores ultrasónicos actuaban como antenas, captando ese ruido estático e interpretándolo como si tuvieran un obstáculo a menos de 20 cm. Esto creaba un bucle infinito en el software que volvía a activar la reversa una y otra vez.
*   **Solución de Ingeniería Aplicada:** 
    1.  Sustituimos la reversa digital del 100% de fuerza por una **reversa PWM suave en el Pin 11 a `-130` PWM**, reduciendo drásticamente el pico de corriente y evitando el reinicio del Arduino.
    2.  Implementamos un **cooldown temporal de 1.5 segundos** (`tiempoFinalizadoEscape`). Al finalizar la reversa de escape, el Arduino ignora por completo el sensor frontal durante 1.5 segundos, dándole tiempo al carro de avanzar hacia adelante, disipar el ruido del motor y estabilizar las ondas ultrasónicas, rompiendo el bucle de retroceso infinito de forma perfecta.