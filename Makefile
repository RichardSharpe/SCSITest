# Main Makefile for iSCSILibWrapper
# This Makefile is not recursive. Rather it includes Makefiles from below

TOPDIR := $(shell pwd)

SOURCES :=

OBJECTS :=

CINCLUDES := -I libiscsi/include 

TARGETS :=

DIRS = src examples

INCLUDES := $(patsubst %, $(TOPDIR)/%/Makefile, $(DIRS))

LIBS := iscsi \
        boost_date_time \
        boost_thread \
        boost_system

# Include all we need ...
# $(warning INCLUDES = $(INCLUDES))
include $(INCLUDES)

CPPFLAGS = $(CINCLUDES)
CFLAGS = -g $(CPPFLAGS)

#$(warning OBJECTS = $(OBJECTS))
$(warning SOURCES = $(SOURCES))
#$(warning CINCLUDES = $(CINCLUDES))
#$(warning CFLAGS = $(CFLAGS))
#$(warning TARGETS = $(TARGETS))

.PHONY: all
# Gotta figure out a better method ... maybe shared libraries
all:	$(OBJECTS) $(TARGETS)

# Generate .d files from .c files
%.d: %.cpp
	@set -e; rm -f $@; \
	$(CC) -M $(CPPFLAGS) $< > $@.$$$$; \
	sed 's,\($*\)\.o[ :]*,\1.o $@ : ,g' < $@.$$$$ > $@; \
	rm -f $@.$$$$

include $(SOURCES:.cpp=.d)

$(TARGETS): %: %.o
	g++ -o $@ $< $(filter-out $<, $(OBJECTS)) -L libiscsi/lib \
		$(addprefix -l, $(LIBS))

$(OBJECTS): %.o: %.cpp
	g++ $(CFLAGS) -c $< -o $@

.PHONY:	clean
clean:
	rm -f $(OBJECTS) $(TARGETS)

# Should have a dist-clean as well to delete the .d files
