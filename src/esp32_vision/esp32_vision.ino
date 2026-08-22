#include "esp_camera.h"
#include "CameraConfig.h"
#include "ColorVision.h"

void setup() {
  // CORRECCIÓN: Sincronizado a 38400 baudios para que el Arduino Uno pueda escucharlo de forma limpia (Desafío 3) [2.1]
  Serial.begin(38400); 

  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer   = LEDC_TIMER_0;
  config.pin_d0       = Y2_GPIO_NUM;
  config.pin_d1       = Y3_GPIO_NUM;
  config.pin_d2       = Y4_GPIO_NUM;
  config.pin_d3       = Y5_GPIO_NUM;
  config.pin_d4       = Y6_GPIO_NUM;
  config.pin_d5       = Y7_GPIO_NUM;
  config.pin_d6       = Y8_GPIO_NUM;
  config.pin_d7       = Y9_GPIO_NUM;
  config.pin_xclk     = XCLK_GPIO_NUM;
  config.pin_pclk     = PCLK_GPIO_NUM;
  config.pin_vsync    = VSYNC_GPIO_NUM;
  config.pin_href     = HREF_GPIO_NUM;
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn     = PWDN_GPIO_NUM;
  config.pin_reset    = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_RGB565;
  config.frame_size   = FRAMESIZE_QQVGA; // 160x120
  config.fb_count     = 1;

  esp_camera_init(&config);

  sensor_t *s = esp_camera_sensor_get();
  if (s != NULL) {
    s->set_whitebal(s, 1);
    s->set_awb_gain(s, 1);
    s->set_gain_ctrl(s, 1);
    s->set_exposure_ctrl(s, 1);
  }
}

void loop() {
  camera_fb_t *fb = esp_camera_fb_get();
  if (!fb) return;

  DeteccionVision resultado = analizarFrame(fb);

  // Transmisión de trama <angulo,velocidad,meta,especial>
  Serial.print("<");
  Serial.print(resultado.angulo);
  Serial.print(",");
  Serial.print(resultado.velocidad);
  Serial.print(",");
  Serial.print(resultado.detectaMeta);
  Serial.print(",");
  Serial.print(resultado.comandoEspecial);
  Serial.println(">");

  esp_camera_fb_return(fb);
  delay(30);
}