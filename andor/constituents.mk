libnames := pdsandor andorutil

libsrcs_pdsandor := $(wildcard *.cc)
#libsinc_pdsandor := 
libincs_pdsandor := pdsdata/include ndarray/include boost/include epics/include epics/include/os/Linux

libsrcs_andorutil := AndorErrorCodes.cc
