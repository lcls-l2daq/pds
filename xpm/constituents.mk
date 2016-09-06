libnames := xpm xpmbase

#libsrcs_xpmbase := Module.cc ClockControl.cc JitterCleaner.cc
libsrcs_xpmbase := Module.cc 

libsrcs_xpm := $(wildcard *.cc)
libincs_xpm := pdsdata/include ndarray/include boost/include
libincs_xpm += epics/include epics/include/os/Linux
