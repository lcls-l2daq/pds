libnames := acqiris

libsrcs_acqiris := $(wildcard *.cc)
libincs_acqiris := acqiris pdsdata/include ndarray/include boost/include 
libincs_acqiris += epics/include epics/include/os/Linux

CPPFLAGS += -D_ACQIRIS -D_LINUX
