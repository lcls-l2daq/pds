libnames := xpm xpmbase

libsrcs_xpmbase := Module.cc

libsrcs_xpm := $(wildcard *.cc)
libincs_xpm := pdsdata/include ndarray/include boost/include