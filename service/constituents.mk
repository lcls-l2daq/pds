libnames := service

libsrcs_service := $(filter-out BitMaskArray.cc,$(wildcard *.cc))
