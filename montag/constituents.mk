libnames := montag

libsrcs_montag := $(wildcard *.cc)
libincs_montag := pdsdata/include ndarray/include

tgtnames := montagtest
tgtsrcs_montagtest := montagtest.cc
tgtlibs_montagtest := pds/service pds/montag pdsdata/xtcdata
tgtslib_montagtest := $(USRLIBDIR)/rt
