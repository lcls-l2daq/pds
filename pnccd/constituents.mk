libnames := pnccd pnccdFrameV0

libsrcs_pnccdFrameV0 := \
		 FrameV0.cc

libsrcs_pnccd := \
		 pnCCDConfigurator.cc \
		 Processor.cc \
		 pnCCDServer.cc \
		 pnCCDManager.cc
#libsinc_pnccd :=
libincs_pnccd := pgpcard
CPPFLAGS += -fno-strict-aliasing
#CPPFLAGS += -fopenmp
#LXFlAGS += -fopenmp
#DEFINES += -fopenmp
