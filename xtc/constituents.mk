libnames := xtc

libsrcs_xtc := $(filter-out $(ignore_src),$(wildcard *.cc))
libincs_xtc := pdsdata/include
