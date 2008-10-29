#
#  Only build for i386-linux.  (rdcds211)
#
ifneq ($(findstring i386-linux-,$(tgt_arch)),)

libnames := camera

libsrcs_camera := Camera.cc  Opal1000.cc  PicPortCL.cc
libsinc_camera := /usr/include/lvsds

tgtnames          := camsend camreceiver serialcmd

#tgtsrcs_camsend := camsend_old.cc
tgtsrcs_camsend := camsend.cc
#tgtsrcs_camsend := camsendt.cc
tgtlibs_camsend := pds/camera
tgtslib_camsend := /usr/lib/lvsds
tgtincs_camsend := pds/zerocopy/kmemory pds/camera
tgtsinc_camsend := /usr/include/lvsds

tgtsrcs_camreceiver := camreceiver.c display.cc
tgtslib_camreceiver := /usr/lib/qt4/lib/QtGui /usr/lib/qt4/lib/QtCore
tgtsinc_camreceiver := /usr/lib/qt4/include

tgtsrcs_serialcmd := serialcmd.cc
tgtslib_serialcmd := /usr/lib/lvsds
tgtsinc_serialcmd := /usr/include/lvsds

endif
