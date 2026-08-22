#ifndef NAVIGATION_H
#define NAVIGATION_H

#include "RobotState.h"
#include "CameraLink.h"

void ejecutarNavegando(long distIzq, long distFront, long distDer,
                        const CameraData &camara,
                        int &anguloFinal, int &velocidadFinal,
                        EstadoRobot &estado);

#endif