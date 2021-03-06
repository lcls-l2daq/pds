ifneq ($(findstring x86_64,$(tgt_arch)),)
libnames := camera camedt
else
libnames := camera camleutron
endif

libsrcs_camera := CameraBase.cc \
		  CameraManager.cc \
		  VmonCam.cc \
		  FexCameraManager.cc \
		  FrameHandle.cc \
		  TwoDMoments.cc \
		  TwoDGaussian.cc \
		  Frame.cc \
	          FrameServer.cc \
	          FexFrameServer.cc \
	          FccdFrameServer.cc \
	          AdimecCommander.cc \
		  Opal1kCamera.cc \
		  Opal1kManager.cc \
		  QuartzCamera.cc \
		  QuartzManager.cc \
		  OrcaCamera.cc \
		  OrcaManager.cc \
		  TM6740Camera.cc \
		  TM6740Manager.cc \
		  FccdCamera.cc \
		  FccdManager.cc \
		  PimManager.cc \
		  OrcaCamera.cc \
		  OrcaManager.cc

libincs_camera := pdsdata/include ndarray/include boost/include 

libsrcs_camleutron := PicPortCL.cc
libsinc_camleutron := /usr/include/lvsds
libincs_camleutron := leutron/include
libincs_camleutron += pdsdata/include

libsrcs_camedt := EdtPdvCL.cc
libincs_camedt := edt/include pdsdata/include

#tgtnames := camreceiver
#tgtnames += camsend
tgtnames :=

ifneq ($(findstring x86_64,$(tgt_arch)),)
tgtnames := pdvserialcmd pdvcamsend camreceiver
else
#tgtnames := camsend camreceiver serialcmd fccdcmd
tgtnames := camsend serialcmd fccdcmd
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
tgtincs_camsend := pds/camera
tgtlibs_camsend := pds/service pds/collection pds/utility pds/client pds/camera pds/xtc
tgtlibs_camsend += pds/vmon pds/mon pds/camleutron pds/configdata
tgtlibs_camsend += pds/config pds/configdbc pds/confignfs pds/configsql
tgtlibs_camsend += pdsdata/xtcdata pdsdata/psddl_pdsdata offlinedb/mysqlclient
tgtlibs_camsend += $(leutron_libs)
tgtincs_camsend += leutron/include
tgtincs_camsend += pdsdata/include ndarray/include boost/include 
#tgtslib_camsend += $(USRLIBDIR)/mysql/mysqlclient

tgtsrcs_camsendm := camsendm.cc
tgtlibs_camsendm := pds/service pds/collection pds/utility pds/config pds/camera pds/client pds/xtc
tgtlibs_camsendm += pds/vmon pds/mon pds/camleutron
tgtlibs_camsendm += pdsdata/xtcdata pdsdata/psddl_pdsdata
tgtincs_camsendm := pds/zerocopy/kmemory pds/camera
tgtlibs_camsendm += $(leutron_libs)
tgtincs_camsendm += leutron/include

tgtsrcs_camreceiver := camreceiver.c display.cc
tgtincs_camreceiver := $(qtincdir)
tgtlibs_camreceiver := $(qtlibdir)
tgtslib_camreceiver := $(USRLIBDIR)/rt $(qtslibdir) $(USRLIBDIR)/pthread

tgtsrcs_serialcmd := serialcmd.cc
tgtlibs_serialcmd := $(leutron_libs)
tgtincs_serialcmd := leutron/include

tgtsrcs_fccdcmd := fccdcmd.cc
tgtlibs_fccdcmd := $(leutron_libs)
tgtincs_fccdcmd := leutron/include

tgtsrcs_pdvserialcmd := pdvserialcmd.cc
tgtincs_pdvserialcmd := edt/include
tgtlibs_pdvserialcmd := edt/pdv pds/service pdsdata/xtcdata
tgtslib_pdvserialcmd := $(USRLIBDIR)/rt dl

tgtsrcs_pdvcamsend := pdvcamsend.cc
tgtincs_pdvcamsend := edt/include pdsdata/include ndarray/include boost/include 
tgtlibs_pdvcamsend := pds/service pds/collection pds/utility pds/config pds/client pds/xtc
tgtlibs_pdvcamsend += pds/vmon pds/mon pds/camera pds/camedt pds/configdata
tgtlibs_pdvcamsend += pds/configdbc pds/confignfs pds/configsql
tgtlibs_pdvcamsend += pdsdata/xtcdata pdsdata/psddl_pdsdata offlinedb/mysqlclient
tgtlibs_pdvcamsend += edt/pdv
tgtslib_pdvcamsend := $(USRLIBDIR)/rt dl
#tgtslib_pdvcamsend += $(USRLIBDIR)/mysql/mysqlclient

