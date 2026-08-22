#ifndef CAMERA_LINK_H
#define CAMERA_LINK_H

struct CameraData {
  int angulo = 70;          // Inicializado en ángulo neutro
  int velocidad = 110;
  int detectaMeta = 0;
  int comandoEspecial = 0;
};

extern CameraData camara;

void setupCameraLink();
void resetEventosCamara();
void leerCamara();

#endif