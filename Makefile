##########################################################################
# Common for all architecture and compiler options
##########################################################################
GCCWARNINGS = -Wimplicit -Wreturn-type -Wswitch -Wcomment -Wformat=2 \
              -Wparentheses -Wpointer-arith -Wcast-qual -Wconversion \
              -Wreorder -Wwrite-strings -Wsynth -Wchar-subscripts \
              -Wuninitialized -W

#DEFINE_FLAGS = -D DEBUG -g -O
DEFINE_FLAGS = -D NDEBUG -O3

PVMDIR = $(PVM_ROOT)
PVMLIBDIR = -L$(PVM_ROOT)/lib/$(PVM_ARCH)
PVMINCLUDE = $(PVM_ROOT)/include

# Pick the appropriate platform and libraries from the following switches
##########################################################################
# For option 2, the Gadget input library needs to be compiled first (see
# the Makefile for Gadget for information on compiling libgadgetinput.a)
# and the location of the gadget directory needs to be set here
GADGETDIR = ../source
##########################################################################
# 1. Linux or Solaris, g++ compiler, no gadget library
#CXX = g++
#LIBDIRS = $(PVMLIBDIR)
#LIBRARIES = -lm -lnsl -lsocket -lpvm3 -lgpvm3
#CXXFLAGS = $(GCCWARNINGS) $(DEFINE_FLAGS) -I$(PVMINCLUDE)
##########################################################################
# 2. Linux, g++ compiler, with gadget library
#CXX = g++
#LIBDIRS = $(PVMLIBDIR) -L. -L$(GADGETDIR)
#LIBRARIES = -lm -lnsl -lsocket -lpvm3 -lgpvm3 -lgadgetinput
#GADGET_FLAGS = -D GADGET_NETWORK -D GADGET_INLINE
#CXXFLAGS = $(GCCWARNINGS) $(DEFINE_FLAGS) $(GADGET_FLAGS) -I$(PVMINCLUDE) -I$(GADGETDIR)
##########################################################################
# 3. Solaris, g++ compiler, with gadget library
CXX = g++
LIBDIRS = $(PVMLIBDIR) -L. -L$(GADGETDIR)
LIBRARIES = -lm -lnsl -lpvm3 -lgpvm3 -lgadgetinput
GADGET_FLAGS = -D GADGET_NETWORK -D GADGET_INLINE
CXXFLAGS = $(GCCWARNINGS) $(DEFINE_FLAGS) $(GADGET_FLAGS) -I$(PVMINCLUDE) -I$(GADGETDIR)
##########################################################################
OBJECTS =  paramin.o processmanager.o netdatacontrol.o netgradient.o \
	netcommunication.o linesearch.o netinterface.o netdatainterface.o \
	netcomminterface.o condition.o datascaler.o dataconverter.o \
	vector.o datastructure.o search.o slavecommunication.o \
	netdata.o pvmconstants.o

LDFLAGS = $(CXXFLAGS) $(LIBDIRS) $(LIBRARIES)

paramin	:	$(OBJECTS)
	$(CXX) -o paramin $(OBJECTS) $(LDFLAGS)

clean:
	rm -f $(OBJECTS)

# DO NOT DELETE THIS LINE -- make depend depends on it.
