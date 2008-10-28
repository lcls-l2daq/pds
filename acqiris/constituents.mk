libnames := acqiris

libsrcs_acqiris := $(wildcard *.cc)

CPPFLAGS += -D_ACQIRIS -D_LINUX
