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
libincs_xamps := pgpcard
CPPFLAGS += -fno-strict-aliasing
#CPPFLAGS += -fopenmp
#LXFlAGS += -fopenmp
#DEFINES += -fopenmp
