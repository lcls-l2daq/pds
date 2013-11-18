libnames := encoder

libsrcs_encoder := $(wildcard *.cc) driver/pci3e-wrapper.cc
libincs_encoder := pdsdata/include ndarray/include boost/include 
