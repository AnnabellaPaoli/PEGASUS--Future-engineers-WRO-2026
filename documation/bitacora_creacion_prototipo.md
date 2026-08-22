# 📓 Engineering Notebook & Developmental Log

**Team:** PEGASUS  
**Robot:** TROYA  
**Category:** WRO 2026 - Future Engineers (Senior)  

This document serves as the chronological developmental diary, test log, and calibration record for the autonomous robot **TROYA**. It details the technical challenges encountered on the track, hypotheses tested, debugging methodologies, and final hardware/software solutions.

---

## 🛠️ Section 1: Mechanical & Structural Challenges

### 1.1 Fatigue Fracture of the Left Steering Knuckle (Ackermann Steering)
*   **Problem Detected:** During high-speed physical testing on the floor, the sudden torque and mechanical stress exerted by the steering servo fractured the left steering knuckle (the plastic pivot connecting the wheel to the steering rod).
*   **Hypothesis 1 (Failure):** Repair the part using instant cyanoacrylate adhesive.  
    *   *Result:* The joint failed immediately during the first test turn because the vibration and shear force of the rubber tires on the floor exceeded the adhesive's tensile strength.
*   **Hypothesis 2 (Failure):** Repair the knuckle body and join it to the servo linkage using liquid steel epoxy (*Pegatanque*).  
    *   *Result:* The knuckle body held, but because the epoxy was extremely rigid and lacked elastic dampening, the mechanical stress concentrated at the servo horn joint, fracturing that junction.
*   **Engineering Solution Applied:** We evaluated manufacturing a replacement part from wood or aluminum, but discarded those due to weight constraints and manual machining limitations. We discovered that **expired PVC credit cards** possessed the ideal ratio of structural rigidity and elastic flexibility. The PVC has the required strength to keep the wheels aligned on the straight, yet provides the necessary elastic flex to absorb sudden mechanical shocks from the servo without snapping.
*   **The Process:** We cut off the fractured remnants of the old knuckle, sanded down the surfaces, shaped the credit card piece with heat, and secured it tightly to the servo linkages using self-tapping screws. The steering mechanism now behaves elastically and robustly during high-speed track navigation.

### 1.2 Steering Center Calibration after Knuckle Repair (The 64° Offset)
*   **Problem Detected:** After successfully repairing the right steering knuckle with the PVC credit card bracket, we noticed during floor tests that the vehicle had a severe permanent bias to steer towards the right, causing it to crash into the right wall repeatedly.
*   **Analyse & Root Cause:** We conducted a physical alignment audit and discovered that the new geometry of our handmade PVC knuckle had shifted the physical straight center of the wheels. When the servo was commanded to its mathematical center of `90°` in the software, the physical wheels were actually turned 26 degrees to the right. 
*   **Engineering Solution Applied:** Instead of disassembling the steering linkage and servo horn, we resolved this misalignment purely in software. We calibrated and redefined the main offset constant **`CENTRO_SERVO = 64;`** as our true physical center. Because our steering constraints, camera maps, and parallel parking states are programmed dynamically, the entire system immediately adapted its limits to `[44, 84]` (normal driving) and `[39, 89]` (manoeuvres), completely eliminating the right-steering bias and stabilizing the straight path.
---

## 🔋 Section 2: Hardware & Power Decisions

### 2.1 Distributed Microcontrollers: Arduino Uno + ESP32-Cam vs. Single-Board Computer
*   **The Decision:** Rather than using a costly, power-hungry single-board computer (like Raspberry Pi or OpenMV), we implemented a distributed low-cost architecture featuring an **Arduino Uno** for low-level control and an **ESP32-Cam** for computer vision.
*   **The Reason:** This architecture meets 100% of the WRO competition requirements (color-based pillar and meta line detection) at a fraction of the cost and power consumption of commercial alternatives. It is a highly viable, economical, and easily repairable prototype in case of component failure in the pit lane.

### 2.2 Power Source: 3 Li-ion 18650 Cells (~11.1V) vs. LiPo Batteries
*   **The Decision:** We opted for a series-connected bank of 3 Lithium-Ion 18650 cells (11.1V nominal, 12.6V fully charged) instead of a standard Polymer Lithium (LiPo) battery.
*   **The Reason:** 18650 cells are highly economical, robust, easy to transport, and easily swappable in pits. They do not suffer from swelling (*swelling*) and, because they are extracted and charged individually, they eliminate the risk of thermal runaway (fire) within the robot's chassis—a critical safety priority following our previous electrical short-circuit incident.

### 2.3 Servo Failure and 22 AWG Heavy-Duty Wiring Upgrade
*   **Problem Detected:** During aggressive steering stress testing, the original plastic-gear steering servo suffered internal mechanical failure. We replaced it with a metal-gear servo. However, during the replacement, we noticed that when powered under load on the ground, the servo suffered from slight twitches and torque starvation.
*   **Analyse & Root Cause:** We analyzed the power delivery and realized that the thin breadboard jumper wires used to power the servo had a high internal electrical resistance ($R$). When the servo demanded high current ($I$) to steer the vehicle's weight on the high-friction floor, a severe voltage drop occurred ($V = I \times R$) across the thin wires, starving the servo's internal motor and causing it to lose torque or jitter.
*   **Engineering Solution Applied:** We completely eliminated the thin jumper wires. We desoldered the power cables of the new servo and soldered **heavy-duty 22 AWG (calibre 22)** copper wires directly from the output of the LM2596 buck regulator to the servo motor. This substantial decrease in wire resistance guaranteed maximum current delivery, providing the steering servo with massive, stable torque and eliminating any voltage drops or electrical jitters.

---

## ⚡ Section 3: The Electrical Incident & Safety Lessons

### 3.1 Short-Circuit on the Integrated Charging Port
Originally, we designed an onboard charging port connected in parallel with the 18650 battery bank for convenience. However, an insulation failure within the port's wiring caused a **massive short-circuit**. The microcontroller suffered irreversible thermal damage, and the battery bank nearly suffered a catastrophic thermal explosion.
*   **Safety Decision:** This incident made us deeply aware of the dangers of high-current power buses without proper fusing or insulation. We decided to **completely remove the integrated charging port** from this physical version of the robot.
*   **Current Safety Protocol:** To charge the 18650 cells safely, we physically extract them from their spring-loaded holders and charge them off-board using an external intelligent balanced smart charger with automatic overcharge cutoff.

---

## 💻 Section 4: Software & Architectural Challenges

### 4.1 Timer 1 Conflict between `Servo.h` and PWM on Pin 10 (`pinENA`)
When wiring the L298N driver, we originally connected `pinENA` to Pin 10 of the Arduino Uno for speed control. However, the drive motor failed to spin entirely whenever the steering servo (Pin 9) was active.
*   **Technical Root Cause:** In the AVR architecture of the ATmega328P (Arduino Uno), the standard `Servo.h` library takes exclusive control of **Timer 1** (a hardware timer) to generate precise servo pulses. In doing so, it **completely disables `analogWrite()` (PWM) functionality on Pins 9 and 10**. The Arduino was simply failing to output any voltage to the ENA pin.
*   **The Solution:** We kept the physical ENA jumper ON (tying ENA permanently to 5V on the board). To maintain speed control, we remapped the drive motor PWM control to **Pin 11 (`IN2`)**, which uses **Timer 2** and has no conflict with the steering servo. In `setup()`, Pin 10 is configured as `OUTPUT` and written `HIGH` as a safety measure to prevent short circuits with the physical jumper.

### 4.2 SoftwareSerial Buffer Overflow & "Zombie Robot" Bug
During ultrasonic testing, we added `delay(15)` between sensor readings to prevent acoustic interference. However, this caused the car to drive blindly straight forward, ignoring all walls and crashing into obstacles.
*   **Technical Root Cause:** The sensor delays increased the main `loop()` execution time to over 55 ms. At 38400 baud, the ESP32-Cam transmits serial data extremely fast. During the 55 ms the Arduino was sleeping, more than 200 bytes accumulated. The `SoftwareSerial` receive buffer only holds **64 bytes**, causing it to **overflow constantly**. This corrupted the packets, leading `sscanf` to parse garbage data and corrupt the Arduino's RAM (Stack Corruption), forcing the vehicle into a frozen "zombie" state where it ignored all distance sensor conditional checks.
*   **The Solution:** We removed all `delay()` blocks between the sensors. Additionally, we implemented a strict packet validation filter: `if (sscanf(...) == 4)`. The Arduino now only updates the camera variables if the incoming serial packet contains exactly 4 correctly parsed integers; otherwise, it discards the frame, preventing buffer overflows and RAM corruption.

---

## 🔊 Section 5: Sensor Physics & Interference

### 5.1 Acoustic Absorption of Textiles (The Furniture Cover Mystery)
During front-wall testing, the robot avoided cardboard boxes and wooden walls perfectly, but **crashed blindly into soft fabric furniture covers and got stuck**.
*   **Technical Root Cause:** Ultrasonic sensors (HC-SR04) measure distance by emitting high-frequency sound waves and waiting for their echo. Rigid materials (wood, plastic, cardboard) reflect these acoustic waves perfectly. However, soft and porous textiles (fabric covers, blankets, curtains) act as **acoustic absorbers**, absorbing the sound wave instead of reflecting it. Lacking an echo, the sensor timed out and returned `0` (which we map to `300 cm`), assuming a clear path and driving directly into the fabric.
*   **The Solution:** We standardized our testing protocol to use **only hard surfaces** (cardboard, wooden boards, books), which accurately mimic the physical characteristics of WRO competition walls.

### 5.2 Motor Stall due to Rolling Resistance in Narrow Lanes (Stall at 85 PWM)
In narrow lanes where the robot had to make continuous steering corrections, the vehicle would suddenly stall and stop moving entirely.
*   **Technical Root Cause:** In an Ackermann-steering vehicle, turning the wheels sharply increases the **rolling resistance** (friction) significantly. In our curves code, we allowed the motor speed to drop as low as `85` PWM. This low power was unable to overcome both the vehicle's weight and the high rolling resistance of the turned wheels, causing the motor to stall.
*   **The Solution:** We set the minimum speed limit during curves to **`105` PWM**. This extra voltage provides the necessary torque to push the vehicle through tight turns without stalling.

### 5.3 Electromagnetic Interference (EMI) Feedback Loop in Reverse
During initial tests of our desatasco (escape) maneuver on the ground, the vehicle stayed still, but in the air, it reversed endlessly.
*   **Technical Root Cause:** This issue had two combined causes:
    1.  *On the ground:* Using `digitalWrite(IN1, HIGH)` directly for reverse commanded 100% battery voltage instantly. Under load, this drew a massive current spike, dropping the battery voltage and **resetting the Arduino Uno repeatedly** (keeping it still).
    2.  *In the air:* Since there was no load, the Arduino didn't reset, but the motor's brush sparks generated massive **electromagnetic interference (EMI)**. The long sensor wires acted as antennas, picking up this noise and registering false front obstacles (< 25cm), keeping the robot trapped in the `ESCAPE` state indefinitely.
*   **The Solution:** 
    1.  We replaced the 100% digital reverse with a **smooth PWM reverse on Pin 11 at `-130` PWM**, reducing the current spike and preventing Arduino reboots.
    2.  We implemented a **1.5-second cooldown timer** (`tiempoFinalizadoEscape`). Upon exiting reverse, the Arduino ignores the front sensor for 1.5 seconds, giving the car time to drive forward, turn off the motor noise, and stabilize the sensors, breaking the EMI feedback loop perfectly.
