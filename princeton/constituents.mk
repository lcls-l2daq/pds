libnames := princeton princetonutil

libsrcs_princeton := $(wildcard *.cc)
#libsinc_princeton := 
libincs_princeton := pdsdata/include ndarray/include boost/include 

libsrcs_princetonutil := PrincetonUtils.cc
