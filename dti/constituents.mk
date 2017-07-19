libnames := dti dtibase

libsrcs_dtibase := Module.cc

libsrcs_dti := $(wildcard *.cc)
libincs_dti := pdsdata/include ndarray/include boost/include
libincs_dti += epics/include epics/include/os/Linux
