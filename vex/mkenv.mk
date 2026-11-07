# VEXcode mkenv.mk 2022_06_26_01

# Estas ayudas permiten manejar rutas que tienen espacios.
sp = $() $()
qs = $(subst ?, ,$1)
sq = $(subst $(sp),?,$1)

# Destino VEX V5 y carpeta donde se guardan los archivos generados.
PLATFORM  = vexv5
BUILD     = build

# Version de las cabeceras del compilador Clang que acompana al SDK.
ifneq ("$(origin HEADERS)", "command line")
HEADERS = 8.0.0
endif

# P=Nombre permite que VS Code indique el nombre del programa.
ifeq ("$(origin P)", "command line")
PROJECT  := $(P)
else
PROJECT  := $(call qs,$(notdir $(call sq,${CURDIR})))
endif

# El nombre del programa no puede tener espacios: las reglas de compilacion lo rechazan.
ifneq (1,$(words $(PROJECT)))
$(error Project name cannot contain whitespace: $(PROJECT))
endif

# T=ruta permite indicar donde esta el SDK, las bibliotecas oficiales de VEX.
# Si no se recibe T, puede usarse la variable de entorno VEX_SDK_PATH.
ifeq ("$(origin T)", "command line")
VEX_SDK_PATH = $(T)
endif
# Ruta de respaldo si no se indico otra ubicacion del SDK.
VEX_SDK_PATH ?= ${HOME}/sdk

# Opcion heredada del SDK para incluir soporte de numeros decimales en printf.
ifeq ("$(origin PRINTF_FLOAT)", "command line")
PRINTF_FLAG = -u_printf_float
endif

# V=1 pide mostrar las ordenes de compilacion completas.
ifeq ("$(origin V)", "command line")
BUILD_VERBOSE=$(V)
endif

# El makefile puede decidir cuanto detalle mostrar si no hay otra opcion.
ifndef VERBOSE
BUILD_VERBOSE ?= 0
else
BUILD_VERBOSE ?= $(VERBOSE)
endif

# @ oculta la orden que se ejecuta, pero no sus mensajes ni errores.
ifeq ($(BUILD_VERBOSE),0)
Q = @
else
Q =
endif

# Programas usados: compilar, enlazar piezas, crear binario y medir su tamano.
CC      = clang
CXX     = clang
OBJCOPY = arm-none-eabi-objcopy
SIZE    = arm-none-eabi-size
LINK    = arm-none-eabi-ld
ARCH    = arm-none-eabi-ar
ECHO    = @echo
DEFINES = -DVexV5

# Adapta las ordenes de crear/borrar carpetas a Windows o a Linux/macOS.
ifeq ($(OS),Windows_NT)
$(info windows build for platform $(PLATFORM))
SHELL = cmd.exe
MKDIR = md "$(@D)" 2> nul || :
RMDIR = rmdir /S /Q
CLEAN = $(RMDIR) $(BUILD) 2> nul || :
else
# Consulta el sistema operativo para mostrar que entorno se esta usando.
UNAME := $(shell sh -c 'uname -sm 2>/dev/null || Unknown')
$(info unix build for platform $(PLATFORM) on $(UNAME))
MKDIR = mkdir -p "$(@D)" 2> /dev/null || :
RMDIR = rm -rf
CLEAN = $(RMDIR) $(BUILD) 2> /dev/null || :
endif

# Rutas de cabeceras y bibliotecas necesarias para construir programas V5.
TOOL_INC  = -I"$(VEX_SDK_PATH)/$(PLATFORM)/clang/$(HEADERS)/include" -I"$(VEX_SDK_PATH)/$(PLATFORM)/gcc/include/c++/4.9.3"  -I"$(VEX_SDK_PATH)/$(PLATFORM)/gcc/include/c++/4.9.3/arm-none-eabi/armv7-ar/thumb" -I"$(VEX_SDK_PATH)/$(PLATFORM)/gcc/include"
TOOL_LIB  = -L"$(VEX_SDK_PATH)/$(PLATFORM)/gcc/libs"

# Opciones oficiales: procesador del Brain, calculos numericos, avisos y version de C/C++.
# Se conservan tal como vienen de VEX: no son ajustes de velocidad del robot.
CFLAGS_CL = -target thumbv7-none-eabi -fshort-enums -Wno-unknown-attributes -U__INT32_TYPE__ -U__UINT32_TYPE__ -D__INT32_TYPE__=long -D__UINT32_TYPE__='unsigned long' 
CFLAGS_V7 = -march=armv7-a -mfpu=neon -mfloat-abi=softfp
CFLAGS    = ${CFLAGS_CL} ${CFLAGS_V7} -Os -Wall -Werror=return-type -ansi -std=gnu99 $(DEFINES)
CXX_FLAGS = ${CFLAGS_CL} ${CFLAGS_V7} -Os -Wall -Werror=return-type -fno-rtti -fno-threadsafe-statics -fno-exceptions  -std=gnu++11 -ffunction-sections -fdata-sections $(DEFINES)

# Une nuestro codigo con el SDK y coloca el programa en la memoria que espera el Brain.
LNK_FLAGS = -nostdlib -T "$(VEX_SDK_PATH)/$(PLATFORM)/lscript.ld" -R "$(VEX_SDK_PATH)/$(PLATFORM)/stdlib_0.lib" -Map="$(BUILD)/$(PROJECT).map" --gc-section -L"$(VEX_SDK_PATH)/$(PLATFORM)" ${TOOL_LIB}

# Nombre y opciones para crear una biblioteca .a, si se usa ese objetivo.
PROJECTLIB = lib$(PROJECT)
ARCH_FLAGS = rcs

# Bibliotecas oficiales de VEX y de C/C++ que se unen al programa.
LIBS =  --start-group -lv5rt -lstdc++ -lc -lm -lgcc --end-group

# Agrega las carpetas donde el compilador debe buscar los archivos incluidos con #include.
INC += $(addprefix -I, ${INC_F})
INC += -I"$(VEX_SDK_PATH)/$(PLATFORM)/include"
INC += ${TOOL_INC}
