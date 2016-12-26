from libc.stdint cimport uint32_t, uint16_t


cdef extern from "ua_connection.h":

    ctypedef struct UA_ConnectionConfig:
        pass

    UA_ConnectionConfig UA_ConnectionConfig_standard;

cdef extern from "ua_job.h":
    ctypedef struct UA_Server:
        pass

cdef extern from "ua_server.h":

    ctypedef struct UA_ServerNetworkLayer:
        pass

    ctypedef struct UA_ServerConfig:
        UA_ServerNetworkLayer *networkLayers;
        size_t networkLayersSize;

    UA_Server * UA_Server_new(const UA_ServerConfig config);

cdef extern from "ua_config_standard.h":

    UA_ServerConfig UA_ServerConfig_standard

cdef extern from "ua_network_tcp.h":

    UA_ServerNetworkLayer UA_ServerNetworkLayerTCP(UA_ConnectionConfig conf, uint16_t port);

