libnames := eb
tgtnames := ft_pingpong ft_server ft_client ft_push ft_request ft_publish ft_sub

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

tgtsrcs_ft_push := ft_push.cc Endpoint.cc
tgtincs_ft_push := libfabric/include pds/eb
tgtlibs_ft_push := libfabric/fabric pds/eb

tgtsrcs_ft_request := ft_request.cc Endpoint.cc
tgtincs_ft_request := libfabric/include pds/eb
tgtlibs_ft_request := libfabric/fabric pds/eb

tgtsrcs_ft_publish := ft_publish.cc Endpoint.cc
tgtincs_ft_publish := libfabric/include pds/eb
tgtlibs_ft_publish := pdsdata/xtcdata pds/service libfabric/fabric pds/eb
tgtslib_ft_publish := pthread

tgtsrcs_ft_sub := ft_sub.cc Endpoint.cc
tgtincs_ft_sub := libfabric/include pds/eb
tgtlibs_ft_sub := libfabric/fabric pds/eb
