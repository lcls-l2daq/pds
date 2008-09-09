#libnames := collection pycollection
libnames := collection

libsrcs_collection := $(wildcard *.cc)

libsrcs_pycollection := collection_sip_wrap.cpp
liblibs_pycollection := pds/service pds/collection
libslib_pycollection := /usr/lib/rt
libsinc_pycollection := /pcds/package/python-2.5.2/include/python2.5

tgtnames        := 
