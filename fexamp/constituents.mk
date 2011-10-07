libnames := fexamp

libsrcs_fexamp := \
                 FexampStatusRegisters.cc \
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
