libnames := management

libsrcs_management := $(wildcard *.cc)
libincs_management := pdsdata/include ndarray/include boost/include 

#
#  LCLS-II development
#

libsrcs_management := CollectionObserver.cc \
		      ControlLevel.cc \
		      ControlStreams.cc \
		      EventAppCallback.cc \
		      MsgAppliance.cc \
		      ObserverLevel.cc \
		      ObserverStreams.cc \
		      PartitionControl.cc \
		      PartitionMember.cc \
		      QualifiedControl.cc \
		      Query.cc \
		      SegmentLevel.cc \
		      SegStreams.cc \
		      SourceLevel.cc \
		      FastSegWire.cc \
		      StdSegWire.cc \
		      VmonClientManager.cc \
		      VmonServerAppliance.cc
