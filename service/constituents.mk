libnames := service

ignore_src := BitMaskArray.cc RingPoolW.cc KStream.cc TStream.cc SpinLock.cc

libsrcs_service := $(filter-out $(ignore_src),$(wildcard *.cc))
libincs_service := pdsdata/include ndarray/include cpsw_boost/include

#
#  LCLS-II development
#
#libsrcs_service := Pool.cc
