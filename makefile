# VEXcode makefile 2019_03_26_01

VERBOSE = 0

include vex/mkenv.mk

SRC_C  = $(wildcard src/*.cpp) 
SRC_C += $(wildcard src/*.c)
SRC_C += $(wildcard src/*/*.cpp) 
SRC_C += $(wildcard src/*/*.c)

OBJ = $(addprefix $(BUILD)/, $(addsuffix .o, $(basename $(SRC_C))) )

SRC_H  = $(wildcard include/*.h)

SRC_A  = makefile

INC_F  = include

all: $(BUILD)/$(PROJECT).bin

include vex/mkrules.mk
