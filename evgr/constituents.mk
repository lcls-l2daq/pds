libnames := evgr

#libsrcs_evgr := $(filter-out EvrSimManager.cc,$(wildcard *.cc))
libsrcs_evgr := $(filter-out EvgManager.cc,$(wildcard *.cc))
#libsrcs_evgr := $(wildcard *.cc)
libincs_evgr := evgr
libincs_evgr += pdsdata/include ndarray/include boost/include 