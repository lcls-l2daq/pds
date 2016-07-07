#
#  Build a "monreq" library
#
libnames := monreq

libsrcs_monreq := MonReqServer.cc ConnectionManager.cc ConnectionRequestor.cc ServerConnection.cc ReceivingConnection.cc
#libsrcs_monreq := MonReqServer.cc
libincs_monreq := pdsdata/include ndarray/include boost/include
