# Makefile for examples
# This Makefile is not recursive. Rather it includes Makefiles from below

dir = $(shell dirname $(lastword $(MAKEFILE_LIST)))

SRC := $(wildcard $(dir)/*.cpp)
OBJ := $(patsubst %.cpp, %.o, $(SRC))
TGTS := $(patsubst &.cpp, %, $(SRC))

TARGETS += $(TGTS)

SOURCES += $(SRC)
OBJECTS += $(OBJ)
