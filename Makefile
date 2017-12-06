################################################################
# Common for all architecture and compiler options
################################################################
GCCWARNINGS = -Wimplicit -Wreturn-type -Wswitch -Wcomment -Wformat \
              -Wparentheses -Wpointer-arith -Wcast-qual -Wconversion \
              -Wreorder -Wwrite-strings -Wsynth -Wchar-subscripts \
              -Wuninitialized -pedantic -W
INC_DIR = ./headers
SRC_DIR = ./src
OBJ_DIR = .


#PVMDIR = $(PVM_ROOT)
#PVMLIBDIR = -L$(PVM_ROOT)/lib/$(PVM_ARCH)
#PVMINCLUDE = $(PVM_ROOT)/include

# Pick the appropriate platform and libraries from the following switches
################################################################
# NOTE that the Gadget input library needs to be compiled
# first - see the Makefile for Gadget to create libgadgetinput.a
# and the location of the gadget directory needs to be set here

GADGET_H = ../gadget/headers
GADGET_L = ../gadget/
###############################################################
# 1. Linux, mpic++ Wrapper Compiler
CXX = mpic++
LIBDIRS = -L$(GADGET_L)
LIBRARIES = -lgadgetinput -lm
#DEFINE_FLAGS = -D DEBUG -D GADGET_NETWORK -g -O
DEFINE_FLAGS = -D NDEBUG -D GADGET_NETWORK -O3
CXXFLAGS = $(GCCWARNINGS) $(DEFINE_FLAGS) -I$(GADGET_H) -I$(INC_DIR)
###############################################################

_OBJECTS =  paramin.o processmanager.o netdatacontrol.o netgradient.o \
	netcommunication.o linesearch.o netinterface.o netdatainterface.o \
	netcomminterface.o condition.o datascaler.o dataconverter.o \
	datastructure.o slavecommunication.o netdata.o pvmconstants.o \
	commandlineinfo.o optimizer.o paraminsearch.o paraminhooke.o \
	paraminsimann.o paraminbfgs.o armijo.o lineseeker.o vector.o

OBJECTS = $(patsubst %,$(SRC_DIR)/%,$(_OBJECTS))

LDFLAGS = $(CXXFLAGS) $(LIBDIRS) $(LIBRARIES)

paramin	:	$(OBJECTS)
	$(CXX) -o paramin $(OBJECTS) $(LDFLAGS)

clean:
	rm -f $(OBJECTS)

# DO NOT DELETE THIS LINE -- make depend depends on it.
