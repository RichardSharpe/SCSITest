# Makefile for SCSI

# This gets the directory where we are included from.
dir := $(shell dirname $(lastword $(MAKEFILE_LIST)))

SRC := $(wildcard $(dir)/*.cpp)
OBJ := $(patsubst %.cpp, %.o, $(SRC))

SOURCES += $(SRC)
OBJECTS += $(OBJ)

