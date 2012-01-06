#libnames := 

#libsrcs_sid4 := Sid4Server.cc \
		  Sid4Manager.cc
#liblibs_sid4 := libdc1394/lib
#libincs_sid4 := libdc1394/include


tgtnames := query_camera grab_gray_image grab_gray_images
tgtsrcs_query_camera := query_camera.c
tgtincs_query_camera := libdc1394/include
tgtlibs_query_camera += libdc1394/dc1394
tgtsrcs_grab_gray_image := grab_gray_image.c
tgtincs_grab_gray_image := libdc1394/include
#tgtlibs_grab_gray_image += libdc1394/raw1394
tgtlibs_grab_gray_image += libdc1394/dc1394
#tgtslib_grab_gray_image += /usr/local/lib/raw1394
#tgtslib_grab_gray_image += /usr/local/lib/dc1394
tgtsrcs_grab_gray_images := grab_gray_images.c
tgtincs_grab_gray_images := libdc1394/include
tgtlibs_grab_gray_images += libdc1394/raw1394
tgtlibs_grab_gray_images += libdc1394/dc1394
CPPFLAGS += -fno-strict-aliasing


