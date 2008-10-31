libnames := acqiris

libsrcs_acqiris := $(wildcard *.cc)
libincs_acqiris := acqiris

CPPFLAGS += -D_ACQIRIS -D_LINUX
