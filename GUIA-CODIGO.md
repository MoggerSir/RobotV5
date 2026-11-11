# Cómo leer este código desde cero

Empieza por `include/robot-config.h` para ver los puertos y luego abre
`src/main.cpp`: es el archivo que conecta todas las piezas del programa.
El código no lleva comentarios: esta guía explica qué hace cada parte.

## Qué hace cada archivo

| Archivo o carpeta | Explicación sencilla |
|---|---|
| `src/main.cpp` | Lee el mando, comprueba si puede moverse y da órdenes a los motores. |
| `include/robot-config.h` | Contiene puertos, cartuchos, sentidos y límites de cada mecanismo. |
| `include/drive-logic.h` | Convierte el joystick izquierdo en velocidad de cada lado del robot. |
| `include/mechanism-logic.h` | Decide si brazo y pinza pueden moverse o deben parar por una protección. |
| `include/vex.h` | Reúne las bibliotecas oficiales de VEX y algunas ayudas de la plantilla. |
| `tests/drive-logic-test.cpp` | Simula entradas y comprueba las cuentas en la computadora, sin mover motores. |
| `makefile` | Indica qué archivos se deben compilar y cuál es el resultado principal. |
| `vex/mkenv.mk` | Conserva las rutas y opciones oficiales para compilar para el procesador V5. |
| `vex/mkrules.mk` | Describe cómo convertir fuentes en piezas intermedias, ejecutable y binario. |
| `tools/habilitar-usb.sh` | Configura permisos USB en Linux; no forma parte del programa del robot. |
| `build/` | Resultados generados al compilar: se pueden regenerar desde los archivos anteriores. |
| `.gitignore` | Dice a Git que no guarde la carpeta `build/` como código fuente. |
| `README.md` | Resume mandos, instalación, pruebas y límites que requieren revisión física. |

## Qué ocurre al ejecutar RobotV5

1. Se crean las representaciones del Brain, el mando y los seis motores.
2. `main()` prepara los grupos de ruedas, la protección del brazo y el hilo que dibuja la pantalla.
3. El robot arranca habilitado. **X** frena y bloquea; para rearmar se pulsa **B**,
   con los joysticks centrados y R1/L1/R2 sueltos.
4. El bucle lee el mando y comprueba que sigan presentes el control y los seis motores.
5. Si está habilitado, calcula la velocidad de ruedas, consulta la protección del brazo y mueve la pinza directamente.
6. Comparte el estado con el hilo de pantalla, que lo dibuja cada 200 ms; el control espera 10 milisegundos y repite.

Que el programa compile o que un motor aparezca detectado no demuestra que su
sentido, su montaje o la mecánica sean correctos. La falta de movimiento de la
pinza sigue pendiente de comprobar físicamente.

## Palabras de C++ que aparecen aquí

| Palabra o símbolo | Qué significa en este proyecto |
|---|---|
| `int` | Número entero, por ejemplo una lectura del joystick. |
| `double` | Número que puede tener decimales, útil para rampas de velocidad. |
| `bool`, `true`, `false` | Valor de sí/no; por ejemplo, si el robot está bloqueado. |
| `const`, `constexpr` | Valores que no se modifican después de inicializarse; `constexpr` también permite usarlos como constantes al compilar. |
| `void` | La función realiza una tarea pero no devuelve un resultado. |
| `return` | Entrega un resultado y sale de la función. |
| `struct` | Reúne datos relacionados, como los valores recibidos del mando. |
| `class` | Reúne datos y funciones, como los grupos de ruedas o la protección de un mecanismo. |
| Constructor | Función que prepara un objeto en el momento de crearlo. |
| `public`, `private` | Lo que puede usarse desde fuera de una clase y lo que queda reservado dentro. |
| `namespace` y `::` | Agrupan nombres: `config::armSpeed` es el ajuste `armSpeed` del grupo `config`. |
| `&` en un parámetro | Usa un objeto existente mediante una referencia, sin crear una copia. |
| `*` en `const char*` | Apunta a un texto; se utiliza para los mensajes de diagnóstico. |
| `if`, `else`, `switch` | Eligen qué instrucciones ejecutar según una condición. |
| `while`, `for` | Repiten instrucciones: para el ciclo principal o para las pruebas. |
| `&&`, `!`, `==`, `!=` | Y, no, igual a, distinto de. `=` por sí solo asigna un valor. |
| `condición ? A : B` | Elige A si la condición se cumple; si no, elige B. |
| `%` | Resto de una división; permite reiniciar el contador de pantalla cada diez vueltas. |
| `assert` | Comprueba algo en las pruebas y detiene la ejecución si no se cumple. |

## Archivos de VS Code

Los archivos `.json` se mantienen sin comentarios porque algunos requieren JSON
estricto. Sus campos se explican aquí para poder consultarlos sin romper su formato.

| Archivo | Campos principales |
|---|---|
| `.vscode/vex_project_settings.json` | `platform` indica V5; `language`, C++; `slot`, la ranura 3; `sdkVersion`, la versión de las bibliotecas oficiales. `extension` identifica el formato que usa la extensión. |
| `.vscode/tasks.json` | Define la tarea de Ctrl+Shift+B: ejecuta `make`, da el nombre `P=RobotV5`, la ruta `T` del SDK y añade el compilador a `PATH`. `cwd` es la carpeta de trabajo. `problemMatcher` ayuda a mostrar errores. |
| `.vscode/c_cpp_properties.json` | Configura las rutas de cabeceras, compilador y ayudas del editor para entender el código V5. No contiene las órdenes del mando. |
| `.vscode/extensions.json` | Recomienda instalar las extensiones de VEX y C/C++. |
| `.vscode/settings.json` | Desactiva el arranque automático al descargar y añade una ruta de cabeceras del SDK. |

Las rutas del SDK y del compilador pertenecen a esta computadora. Si se mueve el
proyecto a otra, hay que ajustar el entorno de VS Code.

## Qué significa que el ciclo sea O(1)

En cada vuelta se hace una cantidad fija de trabajo: se leen los mismos mandos,
se revisan seis motores y se aplican las mismas cuentas. No crece una lista de
órdenes ni se reserva memoria nueva en nuestro ciclo de control. Esto no garantiza
una latencia física exacta: las llamadas al SDK y el funcionamiento del Brain también tardan tiempo.

## Qué cambiar y dónde

- Un motor está en otro puerto o gira al revés: revisa `robot-config.h`.
- Aceleración, sprint, reversa o giro: revisa `drive-logic.h`.
- Tiempo de movimiento, atasco o temperatura: los valores están en `robot-config.h`
  y las comprobaciones en `mechanism-logic.h`.
- Botones, ejes, mensajes o condición de rearme: revisa `main.cpp`.
- Los archivos `vex/*.mk` son soporte del SDK; no son ajustes para conducir.

Para cargar cambios de funcionamiento hay que compilar y volver a descargar al
Brain.
