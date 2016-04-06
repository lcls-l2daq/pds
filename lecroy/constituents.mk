libnames := lecroy

libsrcs_lecroy := $(wildcard *.cc)
libincs_lecroy := pdsdata/include ndarray/include boost/include
libincs_lecroy += epics/include epics/include/os/Linux
