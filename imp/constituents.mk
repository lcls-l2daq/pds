libnames := imp

libsrcs_imp := \
                 ImpStatusRegisters.cc \
                 ImpDestination.cc \
		 ImpConfigurator.cc \
		 Processor.cc \
		 ImpServer.cc \
		 ImpManager.cc
#libsinc_imp :=
libincs_imp := pgpcard pdsdata/include ndarray/include
CPPFLAGS += -fno-strict-aliasing
#CPPFLAGS += -fopenmp
#LXFlAGS += -fopenmp
#DEFINES += -fopenmp
