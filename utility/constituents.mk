libnames := utility

#############################
# -DBUILD_PRINCETON      for increasing event/transition timeouts
# -DBUILD_PACKAGE_SPACE  for solving older switch problem, not used for now
#############################

CXXFLAGS += -DBUILD_READOUT_GROUP -DBUILD_PRINCETON -DBUILD_PACKAGE_SPACE # for princeton camera and the switch problem
#CXXFLAGS += -DBUILD_PRINCETON      # for princeton camera
#CXXFLAGS += -DBUILD_READOUT_GROUP  # for running devices with different readout rate

libsrcs_utility := $(wildcard *.cc)
