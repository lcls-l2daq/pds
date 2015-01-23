libnames := epixS

libsrcs_epixS := \
                 EpixSDestination.cc \
		 EpixSConfigurator.cc \
		 EpixSServer.cc \
		 EpixSManager.cc
#libsinc_epixS :=
libincs_epixS := pgpcard pgp pdsdata/include ndarray/include boost/include
CPPFLAGS += -fno-strict-aliasing
CPPFLAGS += -fopenmp
LXFlAGS += -fopenmp
DEFINES += -fopenmp
