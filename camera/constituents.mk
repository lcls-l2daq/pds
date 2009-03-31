libnames := camera

libsrcs_camera := DmaSplice.cc \
		  FrameHandle.cc \
		  Frame.cc \
		  LvCamera.cc \
		  PicPortCL.cc \
		  Opal1kCamera.cc \
		  TwoDMoments.cc \
		  TwoDGaussian.cc \
	          FexFrameServer.cc \
	          Opal1kManager.cc
#	          FrameServer.cc \
libsinc_camera := /usr/include/lvsds
libincs_camera := leutron/include

#tgtnames := camreceiver
#tgtnames += camsend
tgtnames :=

ifneq ($(findstring -opt,$(tgt_arch)),)
tgtnames := camsend camreceiver serialcmd
endif

tgtsrcs_camsend := camsend.cc
tgtlibs_camsend := pds/service pds/collection pds/utility pds/config pds/camera pds/client pds/xtc
tgtlibs_camsend += pds/vmon pds/mon
tgtlibs_camsend += pdsdata/xtcdata pdsdata/camdata pdsdata/opal1kdata
tgtincs_camsend := pds/zerocopy/kmemory pds/camera
tgtlibs_camsend += leutron/lvsds
tgtlibs_camsend += leutron/LvCamDat.34.32
tgtlibs_camsend += leutron/LvSerialCommunication.34.32
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

