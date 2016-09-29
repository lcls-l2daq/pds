libnames := service

ignore_src := BitMaskArray.cc RingPool.cc RingPoolW.cc KStream.cc TStream.cc

libsrcs_service := $(filter-out $(ignore_src),$(wildcard *.cc))
libincs_service := pdsdata/include ndarray/include cpsw_boost/include

#
#  LCLS-II development
#
#libsrcs_service := Pool.cc
