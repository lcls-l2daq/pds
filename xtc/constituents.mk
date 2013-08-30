libnames := xtc

ignore_src := ZcpDatagram.cc ZcpDatagramIterator.cc
libsrcs_xtc := $(filter-out $(ignore_src),$(wildcard *.cc))
libincs_xtc := pdsdata/include