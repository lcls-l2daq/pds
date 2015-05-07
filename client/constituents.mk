libnames := client
libsrcs_client := $(filter-out FrameCompApp.cc,$(wildcard *.cc))
libincs_client := pdsdata/include ndarray/include boost/include 

libnames += clientcompress
libsrcs_clientcompress := FrameCompApp.cc
libincs_clientcompress := pdsdata/include ndarray/include boost/include 

tgtnames := l3ftest
tgtsrcs_l3ftest := l3ftest.cc
tgtlibs_l3ftest := pdsdata/xtcdata pdsdata/appdata pdsdata/psddl_pdsdata
tgtlibs_l3ftest += pds/client pds/utility pds/service pds/collection pds/vmon pds/mon pds/xtc
tgtslib_l3ftest := $(USRLIBDIR)/rt $(USRLIBDIR)/dl
tgtincs_l3ftest := pdsdata/include