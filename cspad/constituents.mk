libnames := cspad

libsrcs_cspad := \
                 CspadLinkCounters.cc \
                 CspadLinkRegisters.cc \
                 CspadDestination.cc \
                 CspadQuadRegisters.cc \
                 CspadConcentratorRegisters.cc \
		 CspadConfigurator.cc \
		 Processor.cc \
		 CspadServer.cc \
		 CspadManager.cc
#libsinc_cspad :=
libincs_cspad := pgpcard
CPPFLAGS += -fno-strict-aliasing
#CPPFLAGS += -fopenmp
#LXFlAGS += -fopenmp
#DEFINES += -fopenmp
