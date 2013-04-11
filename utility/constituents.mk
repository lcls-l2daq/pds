libnames := utility

#############################
# -DBUILD_PRINCETON      for increasing event/transition timeouts
# -DBUILD_PACKAGE_SPACE  for solving older switch problem, not used for now
#############################

#CXXFLAGS += -DBUILD_READOUT_GROUP -DBUILD_PRINCETON -DBUILD_PACKAGE_SPACE # for princeton camera and the switch problem
#CXXFLAGS += -DBUILD_READOUT_GROUP  # for running devices with different readout rate

ignore_src := ZcpEb.cc ZcpEbC.cc ZcpEbS.cc ZcpEbEvent.cc
libsrcs_utility := $(filter-out $(ignore_src),$(wildcard *.cc))
