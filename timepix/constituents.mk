libnames := timepix

libsrcs_timepix := TimepixManager.cc  TimepixServer.cc timepix_dev.cc TimepixOccurrence.cc

libincs_timepix := relaxd/include/common relaxd/include/src
libincs_timepix += pdsdata/include ndarray/include boost/include 