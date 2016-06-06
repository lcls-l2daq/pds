libnames := tprdsbase tprds

libsrcs_tprdsbase := Module.cc
libincs_tprdsbase := evgr

libsrcs_tprds := $(wildcard *.cc)
libincs_tprds := evgr pdsdata/include ndarray/include boost/include