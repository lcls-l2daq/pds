libnames := config configdata

libsrcs_config := CfgCache.cc CfgClientNet.cc CfgClientNfs.cc CfgPorts.cc 
libincs_config := pdsdata/include 

libsrcs_configdata := $(filter-out $(libsrcs_config), $(wildcard *.cc))
libincs_configdata := pdsdata/include
libincs_configdata += ndarray/include boost/include 
CPPFLAGS += -fno-strict-aliasing

