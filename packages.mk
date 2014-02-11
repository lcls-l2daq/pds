# List of packages (low level first)
packages := service collection xtc
packages += config confignfs configsql mon vmon
packages += utility management client offlineclient
packages += ipimb camera evgr epicstools pgp imp
packages += pnccd epicsArch
packages += oceanoptics fli andor usdusb
packages += cspad cspad2x2
packages += ioc
packages += rayonix
packages += epixSampler
packages += epix

ifneq ($(findstring x86_64,$(tgt_arch)),)
  packages += firewire
##  No DDL
#  packages += phasics
else
  packages += encoder acqiris \
              princeton gsc16ai timepix
endif
##  No DDL
#  packages += xamps
#  packages += fexamp
