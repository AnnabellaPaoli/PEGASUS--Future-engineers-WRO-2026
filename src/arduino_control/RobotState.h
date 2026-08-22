#ifndef ROBOT_STATE_H
#define ROBOT_STATE_H

enum EstadoRobot {
  NAVEGANDO,
  UTURN,
  PARQUEANDO,
  FRENO_META, // <--- NUEVO: Estado para asegurar que entre completo a la zona de meta
  DETENIDO
};

enum SentidoPista {
  DESCONOCIDO,
  HORARIO,
  ANTIHORARIO
};

#endif