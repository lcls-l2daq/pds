libnames := epicstools

# List source files for each library
libsrcs_epicstools := EpicsCA.cc PvServer.cc

libincs_epicstools := epics/include epics/include/os/Linux


tgtnames := epicsmonitor
tgtsrcs_epicsmonitor := epicsmonitor.cc
tgtlibs_epicsmonitor := pds/epicstools
tgtlibs_epicsmonitor += epics/ca epics/Com
tgtincs_epicsmonitor := epics/include epics/include/os/Linux
