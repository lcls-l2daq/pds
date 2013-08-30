libnames := acqiris

libsrcs_acqiris := $(wildcard *.cc)
libincs_acqiris := acqiris pdsdata/include ndarray/include

CPPFLAGS += -D_ACQIRIS -D_LINUX
