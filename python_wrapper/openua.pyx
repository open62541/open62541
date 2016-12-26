cimport copenua


cdef class Server:
    cdef copenua.UA_ServerNetworkLayer _c_nl
    cdef copenua.UA_ServerConfig _c_conf
    cdef copenua.UA_Server* _c_server
    cdef copenua.UA_Boolean _c_running

    def run(self, int port=4840):
        c_con_conf = copenua.UA_ConnectionConfig_standard
        self._c_nl = copenua.UA_ServerNetworkLayerTCP(c_con_conf, port) 
        self._c_conf = copenua.UA_ServerConfig_standard
        self._c_conf.networkLayers = &self._c_nl
        self._c_conf.networkLayersSize = 1
        self._c_server = copenua.UA_Server_new(self._c_conf)
        self._c_running = True
        with nogil:
            status = copenua.UA_Server_run(self._c_server, &self._c_running)
        copenua.UA_Server_delete(self._c_server)
        self._c_nl.deleteMembers(&self._c_nl)
        return status

    def stop(self):
        self._c_running = False


