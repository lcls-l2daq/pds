libnames := camera

libsrcs_camera := FrameHandle.cc Camera.cc  Opal1000.cc  PicPortCL.cc
libincs_camera := leutron/include
#libsinc_camera := /usr/include/lvsds

ifneq ($(findstring -dbg,$(tgt_arch)),)
  tgtnames := camreceiver
else
  tgtnames := camsend camreceiver serialcmd
endif

tgtsrcs_camsend := camsend.cc
tgtlibs_camsend := pds/camera
tgtincs_camsend := pds/zerocopy/kmemory pds/camera
tgtlibs_camsend += leutron/lvsds.34.32
tgtlibs_camsend += leutron/LvCamDat.34.32
tgtlibs_camsend += leutron/LvSerialCommunication.34.32
tgtincs_camsend += leutron/include
#tgtslib_camsend += /usr/LeutronVision/bin/2.00.001/lvsds
#tgtslib_camsend += /usr/LeutronVision/bin/2.00.001/LvSerialCommunication
#tgtsinc_camsend += /usr/LeutronVision/include/2.00.001

tgtsrcs_camreceiver := camreceiver.c display.cc
tgtslib_camreceiver := /pcds/package/qt-4.3.4/lib/QtGui /pcds/package/qt-4.3.4/lib/QtCore
tgtsinc_camreceiver := /pcds/package/qt-4.3.4/include
#tgtslib_camreceiver := /usr/lib/qt4/lib/QtGui /usr/lib/qt4/lib/QtCore
#tgtsinc_camreceiver := /usr/lib/qt4/include

tgtsrcs_serialcmd := serialcmd.cc
tgtlibs_serialcmd := leutron/lvsds
tgtlibs_serialcmd += leutron/LvSerialCommunication
tgtincs_serialcmd := leutron/include

