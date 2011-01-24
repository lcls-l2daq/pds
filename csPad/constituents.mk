libnames := csPad

libsrcs_csPad := \
                 CspadLinkCounters.cc \
                 CspadLinkRegisters.cc \
                 CspadDirectRegisterReader.cc \
                 CspadQuadRegisters.cc \
                 CspadConcentratorRegisters.cc \
		 CspadConfigurator.cc \
		 CspadServer.cc \
		 CspadManager.cc
#libsinc_csPad :=
libincs_csPad := pgpcard/include
CPPFLAGS += -fno-strict-aliasing
