cimport copenua


cdef class Server:
    cdef copenua.UA_ServerNetworkLayer _c_nl
    cdef copenua.UA_ServerConfig _c_conf
    cdef copenua.UA_Server* _c_server

    def __cinit__(self):
        c_con_conf = copenua.UA_ConnectionConfig_standard
        self._c_nl = copenua.UA_ServerNetworkLayerTCP(c_con_conf, 4840) 
        self._c_conf = copenua.UA_ServerConfig_standard
        self._c_conf.networkLayers = &self._c_nl
        self._c_conf.networkLayersSize = 1
        self._c_server = copenua.UA_Server_new(self._c_conf);


