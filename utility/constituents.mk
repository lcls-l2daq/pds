libnames := utility

#CXXFLAGS += -DBUILD_PRINCETON -DBUILD_PACKAGE_SPACE
CXXFLAGS += -DBUILD_PRINCETON

libsrcs_utility := $(wildcard *.cc)
