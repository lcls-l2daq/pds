libnames := epixsampler

libsrcs_epixsampler := \
                  EpixSamplerDestination.cc \
		 EpixSamplerConfigurator.cc \
		 Processor.cc \
		 EpixSamplerServer.cc \
		 EpixSamplerManager.cc
#libsinc_epixsampler :=
libincs_epixsampler := pgpcard pgp pdsdata/include ndarray/include boost/include
CPPFLAGS += -fno-strict-aliasing
#CPPFLAGS += -fopenmp
#LXFlAGS += -fopenmp
#DEFINES += -fopenmp
