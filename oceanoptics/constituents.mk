libnames := oopt oceanoptics 

libsrcs_oopt        := $(wildcard liboopt/*.cc)
libsrcs_oceanoptics := $(wildcard *.cc)
libincs_oceanoptics := pdsdata/include ndarray/include