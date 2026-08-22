#ifndef MANEUVERS_H
#define MANEUVERS_H

#include "RobotState.h"
#include "CameraLink.h"

void ejecutarUTurn(bool entrando, long distFront,
                    int &anguloFinal, int &velocidadFinal, EstadoRobot &estado);

void ejecutarParqueo(bool entrando, long distIzq, long distDer, long distFront,
                     const CameraData &camara,
                     int &anguloFinal, int &velocidadFinal, EstadoRobot &estado);

// Agregamos esta línea para declarar el freno del reto libre:
void ejecutarFrenoMeta(bool entrando, long distFront, int &anguloFinal, int &velocidadFinal, EstadoRobot &estado);

void ejecutarDetenido(int &anguloFinal, int &velocidadFinal);

#endif