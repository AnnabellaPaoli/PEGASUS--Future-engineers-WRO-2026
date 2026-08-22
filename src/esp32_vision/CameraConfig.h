#ifndef CAMERA_CONFIG_H
#define CAMERA_CONFIG_H

#define PWDN_GPIO_NUM     32
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM      0
#define SIOD_GPIO_NUM     26
#define SIOC_GPIO_NUM     27
#define Y9_GPIO_NUM       35
#define Y8_GPIO_NUM       34
#define Y7_GPIO_NUM       39
#define Y6_GPIO_NUM       36
#define Y5_GPIO_NUM       21
#define Y4_GPIO_NUM       19
#define Y3_GPIO_NUM       18
#define Y2_GPIO_NUM        5
#define VSYNC_GPIO_NUM    25
#define HREF_GPIO_NUM     23
#define PCLK_GPIO_NUM     22

// ROI ampliada verticalmente para detectar pilares a mayor distancia
const int ROI_Y_START = 20;
const int ROI_Y_END = 110;

// =============================================================================
// --- PARÁMETROS DE DIRECCIÓN CALIBRADOS PARA TU SERVO FÍSICO REAL (85) ---
// =============================================================================
const int ANGULO_CENTRO = 85;  // Centro alineado al Arduino Uno para ir recto [2]
const int ANGULO_ROJO   = 115; // CORREGIDO: Gira físicamente a la DERECHA ante el pilar rojo (85 + 30) [2]
const int ANGULO_VERDE  = 55;  // CORREGIDO: Gira físicamente a la IZQUIERDA ante el pilar verde (85 - 30) [2]

#endif