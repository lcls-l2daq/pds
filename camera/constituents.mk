libnames := camera

libsrcs_camera := Camera.cc  Opal1000.cc  PicPortCL.cc
libsinc_camera := /usr/include/lvsds

tgtnames          := camsend camreceiver

tgtsrcs_camsend := camsend.cc
tgtlibs_camsend := pds/camera
tgtslib_camsend := /usr/lib/lvsds
tgtincs_camsend := pds/zerocopy/kmemory pds/camera
tgtsinc_camsend := /usr/include/lvsds

tgtsrcs_camreceiver := camreceiver.c display.cc
tgtslib_camreceiver := /usr/lib/qt4/lib/QtGui /usr/lib/qt4/lib/QtCore
tgtsinc_camreceiver := /usr/lib/qt4/include
