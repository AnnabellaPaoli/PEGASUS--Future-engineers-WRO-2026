#ifndef COLOR_VISION_H
#define COLOR_VISION_H

#include "esp_camera.h"

struct DeteccionVision {
  int angulo = 70;         // Centro alineado al MG996R
  int velocidad = 110;
  int detectaMeta = 0;
  int comandoEspecial = 0;
};

// Analiza el frame procesando filtros de color RGB565 dentro de la ROI
DeteccionVision analizarFrame(camera_fb_t *fb);

#endif