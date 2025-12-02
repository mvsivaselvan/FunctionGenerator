
DEFINES = kNIOSLinux
DEBUG_MODE = 0

# Path to ni_modelframework.h
NI_INCLUDE_PATH := C:\VeriStand\2025\ModelInterface


# These variables control the compiler and linker flags. Change them as appropriate.
CC	:= x86_64-nilrt-linux-gcc.exe

# Path to GCC root
# GCCSYSROOTPATH=C:/build/17.0/x64/sysroots/core2-64-nilrt-linux
CFLAGS =  -MD -MP -fdollars-in-identifiers -Wall -fPIC --sysroot=$(subst \,/,$(GCCSYSROOTPATH))

# File remove command
RM	:= cs-rm -rf

ifeq ($(DEBUG_MODE),1)
CFLAGS += -g
else
CFLAGS += -Os -fstrength-reduce -fno-builtin -fno-strict-aliasing
endif

# If you have other .a files to reference, list them here.
LIBS = 
LIBPATH = 

# Include files
INCLUDES = -I. -I$(NI_INCLUDE_PATH)

# List all the *compiled* object files here.
# Make will locate the source file and compile it.
OBJECTS := user_code.o RampAndFunctionGenerator.o ni_modelframework.o

# This is the name of the output shared library.
PROJECT_TARGETS := ./libfunctiongenerator64.so

LDFLAGS =  $(CFLAGS)

all: $(PROJECT_TARGETS) clean

# Build the shared library
$(PROJECT_TARGETS) : $(OBJECTS)
	@echo Building target: $@
	$(CC) $(LDFLAGS) -D$(DEFINES) -shared -m64 -Wl,-soname,"$@" -o "$@" $(OBJECTS) $(LIBPATH) $(LIBS) 
	
# Explicitly listing all source files
user_code.o : user_code.c constants.h model.h RampAndFunctionGenerator.h
	$(CC) $(CFLAGS) $(INCLUDES) -D$(DEFINES) -o "$@" -c "$<"

RampAndFunctionGenerator.o : RampAndFunctionGenerator.c RampAndFunctionGenerator.h
	$(CC) $(CFLAGS) $(INCLUDES) -D$(DEFINES) -o "$@" -c "$<"
	
ni_modelframework.o : $(NI_INCLUDE_PATH)/custom/src/ni_modelframework.c
	$(CC) $(CFLAGS) $(INCLUDES) -D$(DEFINES) -o "$@" -c "$<"	

clean:
	$(RM) $(OBJECTS) *.d
