#include "esp_camera.h"
#include <Arduino.h>

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

const int ROI_Y_START = 65; 
const int ROI_Y_END = 115;
const int MIN_DENSIDAD_PIXELES = 40; 

struct ColorBlob {
  long sumX = 0;
  int count = 0;
};

void rgb565_to_hsv(uint16_t rgb, uint8_t &h, uint8_t &s, uint8_t &v) {
  uint8_t r = ((rgb >> 11) & 0x1F) << 3;
  uint8_t g = ((rgb >> 5) & 0x3F) << 2;
  uint8_t b = (rgb & 0x1F) << 3;
  uint8_t minVal = min(r, min(g, b));
  uint8_t maxVal = max(r, max(g, b));
  v = maxVal;
  uint8_t delta = maxVal - minVal;
  s = (maxVal == 0) ? 0 : (255 * delta / maxVal);
  if (s == 0) h = 0;
  else {
    if (r == maxVal) h = 0 + 43 * (g - b) / delta;
    else if (g == maxVal) h = 85 + 43 * (b - r) / delta;
    else h = 171 + 43 * (r - g) / delta;
  }
}

void setup() {
  // Ajustado a 38400 baudios para estabilidad con SoftwareSerial del Arduino
  Serial.begin(38400);

  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_RGB565;
  config.frame_size = FRAMESIZE_QQVGA; 
  config.fb_count = 1;

  esp_camera_init(&config);

  sensor_t * s = esp_camera_sensor_get();
  if (s != NULL) {
    s->set_whitebal(s, 0);       
    s->set_awb_gain(s, 0);       
    s->set_gain_ctrl(s, 0);      
    s->set_exposure_ctrl(s, 0);  
    s->set_aec_value(s, 250);    
    s->set_agc_value(s, 15);     
  }
}

void loop() {
  camera_fb_t * fb = esp_camera_fb_get();
  if (!fb) return;

  ColorBlob rojo, verde, meta;
  uint16_t* buf = (uint16_t*)fb->buf;

  for (int y = ROI_Y_START; y < ROI_Y_END; y += 2) {
    for (int x = 0; x < fb->width; x += 2) {
      uint16_t pixel = buf[y * fb->width + x];
      pixel = (pixel >> 8) | (pixel << 8); 

      uint8_t h, s, v;
      rgb565_to_hsv(pixel, h, s, v);

      if (s > 80 && v > 40) { 
        if ((h < 10 || h > 245)) { 
          rojo.sumX += x;
          rojo.count++;
        }
        else if (h > 65 && h < 95) { 
          verde.sumX += x;
          verde.count++;
        }
        else if (h > 15 && h < 35) { 
          meta.sumX += x;
          meta.count++;
        }
      }
    }
  }

  int anguloCalculado = 90;
  int velocidadCalculada = 110;
  int detectaMeta = 0;
  int comandoEspecial = 0;

  if (meta.count > MIN_DENSIDAD_PIXELES) {
    detectaMeta = 1;
  }

  if (rojo.count > MIN_DENSIDAD_PIXELES && rojo.count > verde.count) {
    int centroideRojo = rojo.sumX / rojo.count;
    anguloCalculado = map(centroideRojo, 0, fb->width, 100, 115);
    comandoEspecial = 1; 
  } 
  else if (verde.count > MIN_DENSIDAD_PIXELES && verde.count > rojo.count) {
    int centroideVerde = verde.sumX / verde.count;
    anguloCalculado = map(centroideVerde, 0, fb->width, 65, 80);
    comandoEspecial = 2;
  }

  // Enviar paquete de datos por el puerto serie físico
  Serial.print("<");
  Serial.print(anguloCalculado);
  Serial.print(",");
  Serial.print(velocidadCalculada);
  Serial.print(",");
  Serial.print(detectaMeta);
  Serial.print(",");
  Serial.print(comandoEspecial);
  Serial.println(">");

  esp_camera_fb_return(fb);
  delay(25); 
}