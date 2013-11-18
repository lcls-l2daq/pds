libnames := xamps

libsrcs_xamps := \
		 XampsExternalRegisters.cc \
                 XampsInternalRegisters.cc \
                 XampsDestination.cc \
		 XampsConfigurator.cc \
		 Processor.cc \
		 XampsServer.cc \
		 XampsManager.cc
#libsinc_xamps :=
libincs_xamps := pgpcard pdsdata/include ndarray/include boost/include 
CPPFLAGS += -fno-strict-aliasing
#CPPFLAGS += -fopenmp
#LXFlAGS += -fopenmp
#DEFINES += -fopenmp
