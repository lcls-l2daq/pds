libnames := camera

libsrcs_camera := Camera.cc  Opal1000.cc  PicPortCL.cc
libincs_camera := leutron/include
#libsinc_camera := /usr/include/lvsds

tgtnames := camsend camreceiver serialcmd

tgtsrcs_camsend := camsend_old.cc
#tgtsrcs_camsend := camsend.cc
#tgtsrcs_camsend := camsendt.cc
tgtlibs_camsend := pds/camera
tgtincs_camsend := pds/zerocopy/kmemory pds/camera
tgtlibs_camsend += leutron/lvsds
tgtlibs_camsend += leutron/LvSerialCommunication
tgtincs_camsend += leutron/include

tgtsrcs_camreceiver := camreceiver.c display.cc
tgtslib_camreceiver := /usr/lib/qt4/lib/QtGui /usr/lib/qt4/lib/QtCore
tgtsinc_camreceiver := /usr/lib/qt4/include

tgtsrcs_serialcmd := serialcmd.cc
tgtlibs_serialcmd := leutron/lvsds
tgtlibs_serialcmd += leutron/LvSerialCommunication
tgtincs_serialcmd := leutron/include

