libnames := config configdata configdbc

libsrcs_configdbc := DbClient.cc XtcClient.cc PdsDefs.cc DeviceEntry.cc
libincs_configdbc := pdsdata/include ndarray/include boost/include

libsrcs_config := CfgCache.cc CfgClientNfs.cc
libincs_config := pdsdata/include 

libsrcs_configdata := $(filter-out $(libsrcs_config) $(libsrcs_configdbc), $(wildcard *.cc))
libincs_configdata := pdsdata/include
libincs_configdata += ndarray/include boost/include 
CPPFLAGS += -fno-strict-aliasing

