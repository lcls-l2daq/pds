libnames := pnccd pnccdFrameV0

libsrcs_pnccdFrameV0 := \
		 FrameV0.cc
libincs_pnccdFrameV0 := pdsdata/include ndarray/include

libsrcs_pnccd := \
		 pnCCDConfigurator.cc \
		 Processor.cc \
		 pnCCDServer.cc \
		 pnCCDManager.cc
#libsinc_pnccd :=
libincs_pnccd := pgpcard pdsdata/include ndarray/include
CPPFLAGS += -fno-strict-aliasing
#CPPFLAGS += -fopenmp
#LXFlAGS += -fopenmp
#DEFINES += -fopenmp
