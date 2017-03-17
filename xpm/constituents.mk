libnames := xpm xpmbase

#libsrcs_xpmbase := Module.cc ClockControl.cc JitterCleaner.cc
libsrcs_xpm2base := Module.cc 

libsrcs_xpm2 := $(wildcard *.cc)
libincs_xpm2 := pdsdata/include ndarray/include boost/include
libincs_xpm2 += epics/include epics/include/os/Linux
