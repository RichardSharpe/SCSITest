# Main Makefile for iSCSILibWrapper
# This Makefile is not recursive. Rather it includes Makefiles from below

TOPDIR = $(shell pwd)

SOURCES :=

OBJECTS :=

CINCLUDES := -I libiscsi/include 

TARGETS :=

DIRS = src examples

INCLUDES = $(patsubst %, $(TOPDIR)/%/Makefile, $(DIRS))


$(warning INCLUDES = $(INCLUDES))
include $(INCLUDES)

CFLAGS := -g $(CINCLUDES)

$(warning OBJECTS = $(OBJECTS))
$(warning SOURCES = $(SOURCES))
$(warning CINCLUDES = $(CINCLUDES))

# Gotta figure out a better method ... maybe shared libraries
all: $(OBJECTS) $(TARGETS)

# This does not handle .c files that depend on .h files ...
$(OBJECTS): %.o: %.cpp
	g++ -c $(CFLAGS) $< -o $@

.PHONY:	clean
clean:
	rm -f $(OBJECTS)
