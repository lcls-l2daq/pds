libnames := epicsArch

libsrcs_epicsArch := $(wildcard *.cc)
#libsinc_epicsArch := 
libincs_epicsArch := epics/include epics/include/os/Linux
libincs_epicsArch += pdsdata/include ndarray/include