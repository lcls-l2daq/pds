libnames := epix

libsrcs_epix := \
                 EpixDestination.cc \
		 EpixConfigurator.cc \
		 EpixServer.cc \
		 EpixManager.cc
#libsinc_epix :=
libincs_epix := pgpcard pgp pdsdata/include ndarray/include boost/include
CPPFLAGS += -fno-strict-aliasing
CPPFLAGS += -fopenmp
LXFlAGS += -fopenmp
DEFINES += -fopenmp
