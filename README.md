# Robot VEX V5: joystick, sprint, brazo y garra

Codigo C++ para VS Code y la extension oficial VEX Robotics.

Para entender el programa desde cero, abre [GUIA-CODIGO.md](GUIA-CODIGO.md).
El código se publica sin comentarios; GUIA-CODIGO.md y este README explican su funcionamiento.

Ultima carga verificada el 6 de noviembre de 2026 en el Brain V5, programa
RobotV5, ranura 3, mediante el control conectado por USB y enlace inalambrico.
Transferencia al 100 % y presencia del programa actualizado en el Brain verificadas.
Incluye pantalla de diagnostico de joysticks, botones y los seis motores.
No se arranco el programa durante la carga. Las ranuras 1 y 2 se conservaron.

Actualizacion del 6 de noviembre cargada en la ranura 3: sentido del brazo
invertido y diagnostico de pinza (R1/L1, temperatura y motivo de bloqueo).
Transferencia al 100 % verificada; no se ejecuto el programa. La falta de movimiento
de la pinza aun no tiene causa confirmada.

## Mandos

| Mando | Accion |
|---|---|
| Joystick izquierdo arriba/abajo | Avanzar / reversa, con velocidad proporcional |
| Joystick izquierdo derecha/izquierda | Girar; tambien permite giro parado |
| R2 + joystick izquierdo hacia adelante | Sprint |
| A mantenido | Manejo lento; tiene prioridad sobre R2 |
| X | Frenar ruedas y bloquear el manejo; brazo y garra quedan en freno pasivo |
| B, con ambos joysticks centrados y R1/L1/R2 sueltos | Rearmar tras X/desconexion (al iniciar el programa ya queda habilitado) |
| Joystick derecho arriba/abajo | Subir / bajar brazo, con velocidad proporcional |
| R1 / L1 mantenidos | Abrir / cerrar garra |

R2 solo no acelera y no aumenta la velocidad de reversa. L2 queda libre.
Soltar el joystick izquierdo frena las ruedas al instante con freno pasivo (brake), sin consumo de bateria.
Si se pierde el enlace o falta uno de los seis motores, se bloquea el manejo.
B rearma solo al recuperar el enlace y con los mandos en reposo.

Limites de orden de velocidad: normal 50 %, sprint 100 % (solo con R2), reversa 40 %, lento 35 %,
giro parado 40 %. Al empezar un giro o invertir la marcha hay un impulso de 250 ms
hasta 75 %, con 500 ms de descanso; A lo cancela y se desactiva si una rueda llega a 50 grados C. Zona muerta central y curva progresiva en los joysticks.
La aceleracion usa una rampa; al invertir marcha la orden longitudinal pasa por cero.
La velocidad fisica real depende de los motores, carga, ruedas y bateria.
El robot gira por diferencia de velocidad entre lados; no tiene direccion de automovil.

## Motores

| Funcion | Puerto | Cartucho | Sentido configurado |
|---|---|---|---|
| Delantera izquierda | 8 | Verde, 200 rpm | Normal |
| Trasera izquierda | 9 | Verde, 200 rpm | Normal |
| Delantera derecha | 3 | Verde, 200 rpm | Invertido |
| Trasera derecha | 4 | Verde, 200 rpm | Invertido |
| Brazo arriba/abajo | 10 | Rojo (fuerza) confirmado, 100 rpm | Invertido = subir |
| Garra abrir/cerrar | 5 | Rojo confirmado, 100 rpm | Normal = abrir |

Asignacion de puertos confirmada: drivetrain 8,9,3,4; brazo 10; pinza 5.
Las ruedas siguen agrupadas por el orden indicado: 8/9 izquierda y 3/4 derecha.
Los lados y sentidos de montaje requieren comprobacion fisica.
Ajusta todo en `include/robot-config.h`; brazo y garra tienen ajustes independientes.

## Brazo y garra

Brazo: hasta 35 % de velocidad, cuatro segundos de movimiento continuo y torque del 70 % al moverse; al soltar el mando se bloquea con hold al 50 %.
Al volver a mover el mando el bloqueo se libera y el motor sigue la orden.
Garra: 80 % de velocidad y 100 % de torque, sin limite de tiempo, atasco ni temperatura;
al soltar R1/L1 retiene con hold al 80 %.
En el brazo, una velocidad medida inferior a 5 % durante 400 ms lo detiene en freno pasivo.
Una temperatura de al menos 50 grados C bloquea movimiento y usa freno en lugar de
retencion. Tras un bloqueo, devuelve el mando a reposo; si hay temperatura elevada,
espera tambien a que enfrie. Invertir el mando sin soltarlo no evade el bloqueo.

Estos valores requieren calibracion con la mecanica real. No hay finales de carrera
ni limites de posicion absoluta: las protecciones de tiempo y atasco no garantizan
que el mecanismo no llegue a un tope ni limitan de forma fiable la fuerza de apriete.
La retencion puede consumir corriente; X es una parada de software, no un corte electrico.

## Archivos

- `include/robot-config.h`: cableado, cartuchos, sentidos y limites.
- `include/drive-logic.h`: curva de joystick, rampas, sprint y mezcla de ruedas.
- `include/mechanism-logic.h`: proteccion comun del brazo y la garra.
- `src/main.cpp`: lectura del control, motores y bloqueo/rearme.
- `tests/drive-logic-test.cpp`: pruebas de conduccion y protecciones de mecanismos.

La logica de control esta separada del SDK y del hardware. No se agregaron interfaces
virtuales ni jerarquias de clases. Nuestro ciclo utiliza tiempo O(1), memoria O(1)
y ninguna asignacion dinamica. Espera 10 ms entre iteraciones (hasta 100 Hz); la pantalla
se dibuja en un hilo aparte cada 200 ms. El tiempo de respuesta fisico depende del SDK y firmware.

## Compilar y probar

En VS Code: Ctrl+Shift+B o `VEX | Project: Build`.

Pruebas locales de logica desde la terminal del proyecto:

```bash
g++ -std=c++11 -Wall -Wextra -Werror -I include tests/drive-logic-test.cpp -o /tmp/robot-v5-test
/tmp/robot-v5-test
```

Se verificaron limites, simetria del giro, zona muerta, rampas, inversion pasando por
cero, sprint solo hacia adelante, prioridad del modo lento y 100 000 combinaciones
de entradas; tambien atasco, tiempo maximo, temperatura y rearme de mecanismos.

## Cargar mas tarde

1. Verifica el cableado, en especial los lados de las ruedas.
2. Conecta por USB el Brain encendido o el control enlazado al Brain encendido.
3. Si Linux bloquea el dispositivo, ejecuta `sudo sh tools/habilitar-usb.sh` y reconecta.
4. Compila y selecciona una ranura libre en VEX. Download reemplaza la ranura elegida.
5. Para la primera prueba, eleva las ruedas y deja espacio libre alrededor del brazo.
   Ejecuta el programa desde el Brain y pulsa B con los mandos centrados.
6. Comprueba los sentidos a baja velocidad antes de probar sprint. Usa X para parar.

No se ejecuto ningun motor durante la carga ni se reemplazaron los programas de las ranuras 1 y 2.
El codigo viejo no se pudo revisar porque no se proporciono su proyecto fuente.

## Entorno

VS Code, VEX Robotics, VEX Feedback, Microsoft C/C++, SDK oficial
`V5_20240802_15_00_00`, Clang 8 y VEXcom 1.0.2b2.
Linux no tiene soporte oficial VEX. Debian 13 requiere la biblioteca libtinfo5
extraida dentro del toolchain y cargada por el lanzador de Clang. Reinstalar el
toolchain puede requerir restaurar ese ajuste. Las tareas usan rutas de esta PC.
