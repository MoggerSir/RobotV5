# VEXcode makefile 2019_03_26_01

# VERBOSE=0 muestra un resumen. Con 1 aparecen las ordenes completas del compilador.
VERBOSE = 0

# Carga las rutas, herramientas y opciones oficiales para compilar para V5.
include vex/mkenv.mk

# Busca archivos C++ (.cpp) y C (.c) en src y un nivel de subcarpetas.
SRC_C  = $(wildcard src/*.cpp) 
SRC_C += $(wildcard src/*.c)
SRC_C += $(wildcard src/*/*.cpp) 
SRC_C += $(wildcard src/*/*.c)

# Calcula el nombre de cada archivo .o que corresponde a una fuente.
# Los .o son piezas intermedias, no programas que se ejecuten directamente.
OBJ = $(addprefix $(BUILD)/, $(addsuffix .o, $(basename $(SRC_C))) )

# Si cambia un archivo de include, vuelve a compilar los archivos que usan estas reglas.
SRC_H  = $(wildcard include/*.h)

# Cambiar este makefile tambien obliga a recompilar.
SRC_A  = makefile

# Carpeta donde buscar nuestras cabeceras: archivos .h con ajustes y declaraciones.
INC_F  = include

# Objetivo principal: generar el .bin que se puede cargar al Brain.
all: $(BUILD)/$(PROJECT).bin

# Carga las instrucciones que convierten fuentes en objetos, ejecutable y binario.
include vex/mkrules.mk
