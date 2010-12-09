CPPFLAGS += -D_LINUX

#tgtnames    := xstatus xreset xcntrst xdebug xloop

#tgtsrcs_xstatus := xstatus.cpp
#tgtincs_xstatus := pgpcard

#tgtsrcs_xreset := xreset.cpp
#tgtincs_xreset := pgpcard

#tgtsrcs_xcntrst := xcntrst.cpp
#tgtincs_xcntrst := pgpcard

#tgtsrcs_xdebug := xdebug.cpp
#tgtincs_xdebug := pgpcard

#tgtsrcs_xloop := xloop.cpp
#tgtincs_xloop := pgpcard



#tgtlibs_xstatus :=
#tgtslib_xstatus :=

#tgtnames := cxistat

#tgtsrcs_cxistat := cxistat.cc


libnames := pgp

libsrcs_pgp := $(wildcard *.cc)
#libsinc_pgp := 
libincs_pgp := pgpcard

