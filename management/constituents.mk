libnames := management

libsrcs_management := $(wildcard *.cc)

#DEFINES += -DBUILD_SLOW_DISABLE -DBUILD_LARGE_STREAM_BUFFER # for princeton camera
DEFINES += -DBUILD_SLOW_DISABLE        # for long exposure

#ifneq ($(findstring -opt,$(tgt_arch)),)
#DEFINES += -DBUILD_ZCP
#endif
