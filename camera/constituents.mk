libnames := camera

libsrcs_camera := DmaSplice.cc \
		  FrameHandle.cc \
		  Frame.cc \
		  Camera.cc \
		  Opal1000.cc \
		  PicPortCL.cc \
		  TwoDMoments.cc
libincs_camera := leutron/include
#libsinc_camera := /usr/include/lvsds

tgtnames := camreceiver

ifneq ($(findstring -opt,$(tgt_arch)),)
tgtnames := camsend camreceiver serialcmd
endif

tgtsrcs_camsend := camsend.cc
tgtlibs_camsend := pds/service pds/camera
tgtincs_camsend := pds/zerocopy/kmemory pds/camera
tgtlibs_camsend += leutron/lvsds
tgtlibs_camsend += leutron/LvCamDat.34.32
tgtlibs_camsend += leutron/LvSerialCommunication
tgtincs_camsend += leutron/include

tgtsrcs_camreceiver := camreceiver.c display.cc
tgtslib_camreceiver := /pcds/package/qt-4.3.4/lib/QtGui /pcds/package/qt-4.3.4/lib/QtCore
tgtsinc_camreceiver := /pcds/package/qt-4.3.4/include
#tgtslib_camreceiver := /usr/lib/qt4/lib/QtGui /usr/lib/qt4/lib/QtCore
#tgtsinc_camreceiver := /usr/lib/qt4/include

tgtsrcs_serialcmd := serialcmd.cc
tgtlibs_serialcmd := leutron/lvsds
tgtlibs_serialcmd += leutron/LvCamDat.34.32
tgtlibs_serialcmd += leutron/LvSerialCommunication
tgtincs_serialcmd := leutron/include

