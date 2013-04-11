libnames := service

ignore_src := BitMaskArray.cc RingPool.cc RingPoolW.cc KStream.cc TStream.cc

libsrcs_service := $(filter-out $(ignore_src),$(wildcard *.cc))
