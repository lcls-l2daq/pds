libnames := management

libsrcs_management := $(wildcard *.cc)
libincs_management := pdsdata/include ndarray/include

#DEFINES += -DBUILD_SLOW_DISABLE -DBUILD_LARGE_STREAM_BUFFER # for princeton camera
#DEFINES += -DBUILD_SLOW_DISABLE        # for long exposure. No need if princeton runs with "delay shots"

#ifneq ($(findstring -opt,$(tgt_arch)),)
#DEFINES += -DBUILD_ZCP
#endif
