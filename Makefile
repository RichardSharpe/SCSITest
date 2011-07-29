# Main Makefile for iSCSILibWrapper
# This Makefile is not recursive. Rather it includes Makefiles from below

TOPDIR = $(shell pwd)

SOURCES :=

OBJECTS :=

CINCLUDES := -I libiscsi/include 

DIRS = src examples

INCLUDES = $(patsubst %, $(TOPDIR)/%/Makefile, $(DIRS))

include $(INCLUDES)

CFLAGS := -g $(CINCLUDES)

$(warning OBJECTS = $(OBJECTS))
$(warning SOURCES = $(SOURCES))
$(warning CINCLUDES = $(CINCLUDES))

all: $(OBJECTS)

# This does not handle .c files that depend on .h files ...
$(OBJECTS): %.o: %.cpp
	g++ -c $(CFLAGS) $< -o $@

.PHONY:	clean
clean:
	rm -f $(OBJECTS)
