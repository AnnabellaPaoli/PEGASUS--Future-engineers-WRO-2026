#include <Arduino.h>
#include "ColorVision.h"
#include "CameraConfig.h"

struct ColorBlob {
  int count = 0;
};

void rgb565_to_hsv(uint16_t rgb, uint8_t &h, uint8_t &s, uint8_t &v) {
  int16_t r = ((rgb >> 11) & 0x1F) << 3;
  int16_t g = ((rgb >> 5) & 0x3F) << 2;
  int16_t b = (rgb & 0x1F) << 3;

  int16_t minVal = min(r, min(g, b));
  int16_t maxVal = max(r, max(g, b));
  v = maxVal;

  int16_t delta = maxVal - minVal;
  s = (maxVal == 0) ? 0 : (255 * delta / maxVal);

  if (s == 0) {
    h = 0;
  } else if (r == maxVal) {
    int16_t h_calc = 43 * (g - b) / delta;
    if (h_calc < 0) h_calc += 256;
    h = h_calc;
  } else if (g == maxVal) {
    h = 85 + 43 * (b - r) / delta;
  } else {
    h = 171 + 43 * (r - g) / delta;
  }
}

DeteccionVision analizarFrame(camera_fb_t *fb) {
  ColorBlob rojo, verde, meta, parqueoA, parqueoB;
  uint16_t *buf = (uint16_t *)fb->buf;

  // Recorrido de la Región de Interés (ROI)
  for (int y = ROI_Y_START; y < ROI_Y_END; y += 2) {
    for (int x = 0; x < fb->width; x += 2) {
      uint16_t pixel = buf[y * fb->width + x];
      
      // Inversión de bytes para formato RGB565 de ESP32-CAM
      pixel = (pixel >> 8) | (pixel << 8);

      // Extracción de canales R, G, B (0 - 255)
      uint8_t r = ((pixel >> 11) & 0x1F) * 255 / 31;
      uint8_t g = ((pixel >> 5)  & 0x3F) * 255 / 63;
      uint8_t b = (pixel & 0x1F)        * 255 / 31;

      // --- FILTROS DIRECTOS RGB ---
      // 1. ROJO
      if (r > 80 && r > (g + 25) && r > (b + 25)) {
        rojo.count++;
      }
      // 2. VERDE
      else if (g > 80 && g > (r + 20) && g > (b + 20)) {
        verde.count++;
      }
      // 3. META (Naranja / Amarillo)
      else if (r > 110 && g > 75 && b < 70 && abs(r - g) < 60) {
        meta.count++;
      }
      // 4. GARAJE A (Azul)
      else if (b > 100 && b > (r + 30) && b > (g + 20)) {
        parqueoA.count++;
      }
      // 5. GARAJE B (Morado/Magenta)
      else if (r > 90 && b > 90 && g < 60) {
        parqueoB.count++;
      }
    }
  }

  DeteccionVision resultado;
  // CORRECCIÓN: Vinculado a ANGULO_CENTRO (70) de tu CameraConfig.h [2]
  resultado.angulo = ANGULO_CENTRO; 
  resultado.velocidad = 120;
  resultado.detectaMeta = 0;
  resultado.comandoEspecial = 0;

  const int UMBRAL_PIXELES = 25;

  // Prioridad 1: Meta
  if (meta.count > (UMBRAL_PIXELES * 2)) {
    resultado.detectaMeta = 1;
  }

  // Prioridad 2: Pilares (CORRECCIÓN: Vinculados a ANGULO_ROJO y ANGULO_VERDE de tu CameraConfig.h) [2]
  if (rojo.count > UMBRAL_PIXELES && rojo.count > verde.count) {
    resultado.angulo = ANGULO_ROJO;   // Gira físicamente a la derecha ante el rojo (95° en tu carro) [2]
    resultado.comandoEspecial = 1;
  } 
  else if (verde.count > UMBRAL_PIXELES && verde.count > rojo.count) {
    resultado.angulo = ANGULO_VERDE;  // Gira físicamente a la izquierda ante el verde (45° en tu carro) [2]
    resultado.comandoEspecial = 2;
  } 
  // Prioridad 3: Garajes (Mapeado a 4 para que coincida perfectamente con el Arduino)
  else if (parqueoB.count > UMBRAL_PIXELES) { 
    resultado.comandoEspecial = 4; // Magenta (Zona de parqueo oficial)
  } 
  else if (parqueoA.count > UMBRAL_PIXELES) {
    resultado.comandoEspecial = 5; // Azul (Secundario o libre)
  }

  return resultado;
}