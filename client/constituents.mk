libnames := client
libsrcs_client := $(filter-out FrameCompApp.cc,$(wildcard *.cc))

libnames += clientcompress
libsrcs_clientcompress := FrameCompApp.cc