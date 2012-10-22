libnames := camera camleutron camedt

libsrcs_camera := CameraBase.cc \
      CameraManager.cc \
      FrameHandle.cc \
      Frame.cc \
      TwoDMoments.cc \
      TwoDGaussian.cc \
            FrameServer.cc \
            FexFrameServer.cc \
            FccdFrameServer.cc \
      Opal1kCamera.cc \
      Opal1kManager.cc \
      QuartzCamera.cc \
      QuartzManager.cc \
      TM6740Camera.cc \
      TM6740Manager.cc \
      FccdCamera.cc \
      FccdManager.cc \
      PimManager.cc

libsrcs_camleutron := PicPortCL.cc
libsinc_camleutron := /usr/include/lvsds
libincs_camleutron := leutron/include

libsrcs_camedt := EdtPdvCL.cc
libincs_camedt := edt/include

#tgtnames := camreceiver
#tgtnames += camsend
tgtnames :=

ifneq ($(findstring x86_64-linux,$(tgt_arch)),)
tgtnames := pdvserialcmd pdvcamsend camreceiver
else
#tgtnames := camsend camreceiver serialcmd fccdcmd pdvserialcmd pdvcamsend camsendm
tgtnames := camsend camreceiver serialcmd fccdcmd pdvserialcmd pdvcamsend 
endif

# ifeq ($(shell uname -m | egrep -c '(x86_|amd)64$$'),1)
# ARCHCODE=64
# else
# ARCHCODE=32
# endif

ifneq ($(findstring i386,$(tgt_arch)),)
ARCHCODE=32
else
ARCHCODE=64
endif

leutron_libs := leutron/lvsds.34.${ARCHCODE}
leutron_libs += leutron/LvCamDat.34.${ARCHCODE}
leutron_libs += leutron/LvSerialCommunication.34.${ARCHCODE}

tgtsrcs_camsend := camsend.cc
tgtincs_camsend := pds/zerocopy/kmemory pds/camera
tgtlibs_camsend := pds/service pds/collection pds/utility pds/config pds/camera pds/client pds/xtc
tgtlibs_camsend += pds/vmon pds/mon pds/camleutron
tgtlibs_camsend += pdsdata/xtcdata pdsdata/camdata pdsdata/opal1kdata pdsdata/quartzdata pdsdata/pulnixdata pdsdata/fccddata
tgtlibs_camsend += pdsdata/cspaddata pdsdata/cspad2x2data pdsdata/timepixdata
tgtlibs_camsend += $(leutron_libs)
tgtincs_camsend += leutron/include

tgtsrcs_camsendm := camsendm.cc
tgtlibs_camsendm := pds/service pds/collection pds/utility pds/config pds/camera pds/client pds/xtc
tgtlibs_camsendm += pds/vmon pds/mon pds/camleutron
tgtlibs_camsendm += pdsdata/xtcdata pdsdata/camdata pdsdata/opal1kdata pdsdata/pulnixdata pdsdata/fccddata
tgtincs_camsendm := pds/zerocopy/kmemory pds/camera
tgtlibs_camsendm += $(leutron_libs)
tgtincs_camsendm += leutron/include

tgtsrcs_camreceiver := camreceiver.c display.cc
tgtincs_camreceiver := qt/include_${ARCHCODE}
tgtlibs_camreceiver := qt/QtGui qt/QtCore

tgtsrcs_serialcmd := serialcmd.cc
tgtlibs_serialcmd := $(leutron_libs)
tgtincs_serialcmd := leutron/include

tgtsrcs_fccdcmd := fccdcmd.cc
tgtlibs_fccdcmd := $(leutron_libs)
tgtincs_fccdcmd := leutron/include

tgtsrcs_pdvserialcmd := pdvserialcmd.cc
tgtincs_pdvserialcmd := edt/include
tgtlibs_pdvserialcmd := edt/pdv
tgtslib_pdvserialcmd := $(USRLIBDIR)/rt dl

tgtsrcs_pdvcamsend := pdvcamsend.cc
tgtincs_pdvcamsend := edt/include
tgtlibs_pdvcamsend := pds/service pds/collection pds/utility pds/config pds/client pds/xtc
tgtlibs_pdvcamsend += pds/vmon pds/mon pds/camera pds/camedt
tgtlibs_pdvcamsend += pdsdata/xtcdata pdsdata/camdata pdsdata/opal1kdata pdsdata/quartzdata pdsdata/pulnixdata pdsdata/fccddata
tgtlibs_pdvcamsend += pdsdata/cspaddata pdsdata/cspad2x2data pdsdata/timepixdata
tgtlibs_pdvcamsend += edt/pdv
tgtslib_pdvcamsend := $(USRLIBDIR)/rt dl

