libnames := pdsandor andorutil

libsrcs_pdsandor := $(wildcard *.cc)
#libsinc_pdsandor := 
libincs_pdsandor := pdsdata/include ndarray/include

libsrcs_andorutil := AndorErrorCodes.cc
