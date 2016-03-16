libnames := pvdaq

libsrcs_pvdaq := $(wildcard *.cc)
libincs_pvdaq := pdsdata/include ndarray/include boost/include 
libincs_pvdaq += epics/include epics/include/os/Linux