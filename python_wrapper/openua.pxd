from libc.stdint cimport uint32_t, uint16_t

cdef extern from "ua_connection.h":

    ctypedef struct UA_ConnectionConfig:
        pass

cdef extern from "ua_server.h":
    ctypedef struct UA_ServerNetworkLayer:
        pass

cdef extern from "ua_network_tcp.h":
    UA_ServerNetworkLayer UA_ServerNetworkLayerTCP(UA_ConnectionConfig conf, uint16_t port);

