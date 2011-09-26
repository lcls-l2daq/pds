libnames := fexamp

libsrcs_fexamp := \
		 FexampExternalRegisters.cc \
                 FexampInternalRegisters.cc \
                 FexampDestination.cc \
		 FexampConfigurator.cc \
		 Processor.cc \
		 FexampServer.cc \
		 FexampManager.cc
#libsinc_fexamp :=
libincs_fexamp := pgpcard
CPPFLAGS += -fno-strict-aliasing
#CPPFLAGS += -fopenmp
#LXFlAGS += -fopenmp
#DEFINES += -fopenmp
