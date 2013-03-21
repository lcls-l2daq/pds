libnames := service

libsrcs_service := $(filter-out BitMaskArray.cc RingPool.cc RingPoolW.cc,$(wildcard *.cc))
