libnames := eb
tgtnames := ft_pingpong ft_server ft_client

#libsrcs_eb := $(wildcard *.cc)
libsrcs_eb := fiTransport.cc
libincs_eb := pdsdata/include ndarray/include boost/include
libincs_eb += libfabric/include

tgtsrcs_ft_pingpong := ft_pingpong.cc
tgtincs_ft_pingpong := pdsdata/include ndarray/include boost/include
tgtincs_ft_pingpong += libfabric/include pds/eb
tgtlibs_ft_pingpong := libfabric/fabric pds/eb
tgtslib_ft_pingpong := pthread

tgtsrcs_ft_server	:= ft_server.cc Endpoint.cc
tgtincs_ft_server := libfabric/include pds/eb
tgtlibs_ft_server := libfabric/fabric pds/eb

tgtsrcs_ft_client := ft_client.cc Endpoint.cc
tgtincs_ft_client := libfabric/include pds/eb
tgtlibs_ft_client := libfabric/fabric pds/eb
