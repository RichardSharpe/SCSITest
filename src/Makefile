# Main Makefile for iSCSILibWrapper
# This Makefile is not recursive. Rather it includes Makefiles from below

DIRS = SCSI iSCSI

dir = $(shell dirname $(lastword $(MAKEFILE_LIST)))

MY_INCLUDES = $(patsubst %, $(dir)/%/Makefile, $(DIRS))

CINCLUDES += $(patsubst %, -I $(dir)/%, $(DIRS))

include $(MY_INCLUDES)
