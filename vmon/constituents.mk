libnames := vmon

libsrcs_vmon := $(filter-out vmonreaderdump.cc, $(wildcard *.cc))
libincs_vmon := pdsdata/include ndarray/include

tgtnames := vmonreaderdump

tgtsrcs_vmonreaderdump += vmonreaderdump.cc
tgtlibs_vmonreaderdump := pds/vmon pds/mon pds/service pds/collection 
tgtlibs_vmonreaderdump += pdsdata/xtcdata
tgtslib_vmonreaderdump := $(USRLIBDIR)/rt
tgtincs_vmonreaderdump := pdsdata/include
