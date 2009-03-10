libnames := management

libsrcs_management := $(wildcard *.cc)

#ifneq ($(findstring -opt,$(tgt_arch)),)
#DEFINES += -DBUILD_ZCP
#endif
