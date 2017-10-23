libnames := eb
tgtnames := ft_pingpong ft_server ft_client ft_push ft_request ft_publish ft_sub
tgtnames += tstEbContributor tstEbBuilder

libsrcs_eb := Batch.cc BatchManager.cc EventBuilder.cc EbEpoch.cc EbEvent.cc
libsrcs_eb += FtInlet.cc FtOutlet.cc
libincs_eb := pdsdata/include ndarray/include boost/include libfabric/include
liblibs_eb := libfabric/fabric

tgtsrcs_ft_pingpong := ft_pingpong.cc fiTransport.cc
tgtincs_ft_pingpong := pdsdata/include ndarray/include boost/include
tgtincs_ft_pingpong += libfabric/include
tgtlibs_ft_pingpong := libfabric/fabric
tgtslib_ft_pingpong := pthread

tgtsrcs_ft_server	:= ft_server.cc Endpoint.cc
tgtincs_ft_server := libfabric/include
tgtlibs_ft_server := libfabric/fabric

tgtsrcs_ft_client := ft_client.cc Endpoint.cc
tgtincs_ft_client := libfabric/include
tgtlibs_ft_client := libfabric/fabric

tgtsrcs_ft_push := ft_push.cc Endpoint.cc
tgtincs_ft_push := libfabric/include
tgtlibs_ft_push := libfabric/fabric

tgtsrcs_ft_request := ft_request.cc Endpoint.cc
tgtincs_ft_request := libfabric/include
tgtlibs_ft_request := libfabric/fabric

tgtsrcs_ft_publish := ft_publish.cc Endpoint.cc
tgtincs_ft_publish := libfabric/include
tgtlibs_ft_publish := pdsdata/xtcdata pds/service libfabric/fabric
tgtslib_ft_publish := pthread

tgtsrcs_ft_sub := ft_sub.cc Endpoint.cc
tgtincs_ft_sub := libfabric/include
tgtlibs_ft_sub := libfabric/fabric

tgtsrcs_tstEbContributor := tstEbContributor.cc Endpoint.cc
tgtincs_tstEbContributor := pdsdata/include ndarray/include boost/include
tgtincs_tstEbContributor += libfabric/include pds/eb
tgtlibs_tstEbContributor := pdsdata/xtcdata pds/service pds/xtc pds/eb
tgtlibs_tstEbContributor += libfabric/fabric

tgtsrcs_tstEbBuilder := tstEbBuilder.cc Endpoint.cc
tgtincs_tstEbBuilder := pdsdata/include ndarray/include boost/include
tgtincs_tstEbBuilder += libfabric/include pds/eb
tgtlibs_tstEbBuilder := pdsdata/xtcdata pds/service pds/xtc pds/eb
tgtlibs_tstEbBuilder += libfabric/fabric
