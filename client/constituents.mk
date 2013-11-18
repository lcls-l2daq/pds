libnames := client
libsrcs_client := $(filter-out FrameCompApp.cc,$(wildcard *.cc))
libincs_client := pdsdata/include ndarray/include boost/include 

libnames += clientcompress
libsrcs_clientcompress := FrameCompApp.cc
libincs_clientcompress := pdsdata/include ndarray/include boost/include 