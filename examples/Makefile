# Main Makefile for iSCSILibWrapper
# This Makefile is not recursive. Rather it includes Makefiles from below

DIRS = src examples

INCLUDES = $(patsubst %,%/Makefile, $(DIRS))

.PHONY:	all

all:
	echo $(INCLUDES)	
