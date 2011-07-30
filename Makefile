# Main Makefile for iSCSILibWrapper
# This Makefile is not recursive. Rather it includes Makefiles from below

TOPDIR := $(shell pwd)

SOURCES :=

OBJECTS :=

CINCLUDES := -I libiscsi/include 

TARGETS :=

DIRS = src examples

INCLUDES := $(patsubst %, $(TOPDIR)/%/Makefile, $(DIRS))

# Include all we need ...
# $(warning INCLUDES = $(INCLUDES))
include $(INCLUDES)

CFLAGS = -g $(CINCLUDES)

#$(warning OBJECTS = $(OBJECTS))
#$(warning SOURCES = $(SOURCES))
$(warning CINCLUDES = $(CINCLUDES))
$(warning CFLAGS = $(CFLAGS))
$(warning TARGETS = $(TARGETS))

# Gotta figure out a better method ... maybe shared libraries
all:	$(OBJECTS) $(TARGETS)

$(TARGETS): %: %.o
	g++ -o $@ $< $(OBJECTS) -L libiscsi/lib -l iscsi

$(OBJECTS): %.o: %.cpp
	g++ $(CFLAGS) -c $< -o $@

.PHONY:	clean
clean:
	rm -f $(OBJECTS) $(TARGETS)
