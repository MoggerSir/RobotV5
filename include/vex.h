// PUERTA DE ENTRADA A LAS BIBLIOTECAS: reune herramientas usadas por el proyecto.
// Las bibliotecas del SDK de VEX se instalan aparte; este archivo solo las incluye.
// Herramientas de calculo, entrada/salida de texto, utilidades generales y manejo de texto.
// Los nombres entre < > pertenecen a bibliotecas; los nombres entre comillas son archivos incluidos por ruta.
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Definiciones y clases oficiales del Brain, mando y motores VEX V5.
#include "v5.h"
#include "v5_vcs.h"


// Macro de ayuda del proyecto oficial: repite una espera hasta que se cumpla la condicion.
// Las barras al final de linea hacen que la macro continue en la linea siguiente. No se usa en nuestro main.
#define waitUntil(condition)                                                   \
  do {                                                                         \
    wait(5, msec);                                                             \
  } while (!(condition))

// Otra macro de ayuda: repite un bloque un numero de veces. Tampoco se usa en nuestro main.
#define repeat(iterations)                                                     \
  for (int iterator = 0; iterator < iterations; iterator++)
