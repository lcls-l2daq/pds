libnames := cphw

libsrcs_cphw := RingBuffer.cc 
libsrcs_cphw += AxiVersion.cc
#libsrcs_cphw += Reg.cc
libsrcs_cphw += Reg_rssi.cc
libincs_cphw := cpsw/include cpsw_boost/include yaml/include
