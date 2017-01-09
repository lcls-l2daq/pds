libnames := ibeb

tgtnames := ibeboutlet ibebinlet

ignore_src := $(addsuffix .cc, $(tgtnames))

libsrcs_ibeb := $(filter-out $(ignore_src),$(wildcard *.cc))
libincs_ibeb := pdsdata/include ndarray/include boost/include
libslib_ibeb := ibverbs

tgtsrcs_ibeboutlet := ibeboutlet.cc
tgtincs_ibeboutlet := pdsdata/include ndarray/include boost/include
tgtlibs_ibeboutlet := pdsdata/xtcdata pds/service pds/xtc pds/ibeb
tgtslib_ibeboutlet := ibverbs rt pthread

tgtsrcs_ibebinlet := ibebinlet.cc
tgtincs_ibebinlet := pdsdata/include ndarray/include boost/include
tgtlibs_ibebinlet := pdsdata/xtcdata pds/service pds/xtc pds/ibeb
tgtlibs_ibebinlet += pds/ibeb
tgtslib_ibebinlet := ibverbs rt pthread

