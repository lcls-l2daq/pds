libnames := cspad2x2

libsrcs_cspad2x2 := \
                 Cspad2x2LinkCounters.cc \
                 Cspad2x2LinkRegisters.cc \
                 Cspad2x2Destination.cc \
                 Cspad2x2QuadRegisters.cc \
                 Cspad2x2ConcentratorRegisters.cc \
		 Cspad2x2Configurator.cc \
		 Processor.cc \
		 Cspad2x2Server.cc \
		 Cspad2x2Manager.cc
#libsinc_cspad2x2 :=
libincs_cspad2x2 := pgpcard
CPPFLAGS += -fno-strict-aliasing
#CPPFLAGS += -fopenmp
#LXFlAGS += -fopenmp
#DEFINES += -fopenmp
