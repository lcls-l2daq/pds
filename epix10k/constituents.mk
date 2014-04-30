libnames := epix10k

libsrcs_epix10k := \
                 Epix10kDestination.cc \
		 Epix10kConfigurator.cc \
		 Epix10kServer.cc \
		 Epix10kManager.cc
#libsinc_epix10k :=
libincs_epix10k := pgpcard pgp pdsdata/include ndarray/include boost/include
CPPFLAGS += -fno-strict-aliasing
CPPFLAGS += -fopenmp
LXFlAGS += -fopenmp
DEFINES += -fopenmp
