libnames := cphw cphwr

libsrcs_cphw := RingBuffer.cc 
libsrcs_cphw += AxiVersion.cc
#libsrcs_cphw += Reg.cc
libsrcs_cphw += Reg_rssi.cc
libsrcs_cphw += AmcTiming.cc
libsrcs_cphw += XBar.cc
libincs_cphw := cpsw/include cpsw_boost/include yaml/include

libsrcs_cphwr := RingBuffer.cc 
libsrcs_cphwr += AxiVersion.cc
libsrcs_cphwr += Reg.cc
libsrcs_cphwr += AmcTiming.cc
libsrcs_cphwr += XBar.cc
libincs_cphwr := cpsw/include cpsw_boost/include yaml/include

