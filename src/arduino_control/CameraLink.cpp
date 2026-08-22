#include <Arduino.h>
#include <SoftwareSerial.h>
#include "Config.h"
#include "CameraLink.h"

CameraData camara;

#if USAR_CAMARA
SoftwareSerial espSerial(PIN_RX_CAMARA, PIN_TX_CAMARA);

static char serialBuffer[32];
static int bufferIndex = 0;
static bool datosNuevos = false;
#endif

void setupCameraLink() {
#if USAR_CAMARA
  espSerial.begin(9600); // Corregido de 38400 a 9600 baudios
  pinMode(PIN_RX_CAMARA, INPUT_PULLUP);
#endif
}

void resetEventosCamara() {
  camara.detectaMeta = 0;
  camara.comandoEspecial = 0;
}

void leerCamara() {
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

  if (datosNuevos) {
    int camAng, camVel, camMeta, camEsp;
    if (sscanf(serialBuffer, "%d,%d,%d,%d", &camAng, &camVel, &camMeta, &camEsp) == 4) {
      camara.angulo = camAng;
      camara.velocidad = camVel;
      camara.detectaMeta = camMeta;
      camara.comandoEspecial = camEsp;
    }
    datosNuevos = false;
  }
#endif
}