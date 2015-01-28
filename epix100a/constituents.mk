libnames := epix100a

libsrcs_epix100a := \
                 Epix100aDestination.cc \
		 Epix100aConfigurator.cc \
		 Epix100aServer.cc \
		 Epix100aManager.cc
#libsinc_epix100a :=
libincs_epix100a := pgpcard pgp pdsdata/include ndarray/include boost/include
CPPFLAGS += -fno-strict-aliasing
CPPFLAGS += -fopenmp
LXFlAGS += -fopenmp
DEFINES += -fopenmp
