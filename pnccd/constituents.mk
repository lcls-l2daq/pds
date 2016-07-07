libnames := pnccd pnccdFrameV0

libsrcs_pnccdFrameV0 := \
		 FrameV0.cc
libincs_pnccdFrameV0 := pdsdata/include ndarray/include boost/include 

libsrcs_pnccd := \
		 pnCCDConfigurator.cc \
		 Processor.cc \
		 pnCCDServer.cc \
		 pnCCDManager.cc \
		 pnCCDTrigMonitor.cc
#libsinc_pnccd :=
libincs_pnccd := pgpcard pdsdata/include ndarray/include boost/include 
libincs_pnccd += epics/include epics/include/os/Linux
CPPFLAGS += -fno-strict-aliasing
#CPPFLAGS += -fopenmp
#LXFlAGS += -fopenmp
#DEFINES += -fopenmp
