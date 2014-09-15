libnames := genericpgp

libsrcs_genericpgp := \
                 Destination.cc \
		 Configurator.cc \
		 PeriodMonitor.cc \
		 Server.cc \
		 Manager.cc
libincs_genericpgp := pgpcard pgp pdsdata/include ndarray/include boost/include
CPPFLAGS += -fno-strict-aliasing
#CPPFLAGS += -fopenmp
#LXFlAGS += -fopenmp
#DEFINES += -fopenmp
