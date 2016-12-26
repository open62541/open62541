cimport openua

cdef class Server:
    # cdef cqueue.Queue* _c_queue
    def __cinit__(self):
        #self._c_conf = openua.UA_ConnectionConfig
        self._c_nl = openua.UA_ServerNetworkLayerTCP(openua.UA_ConnectionConfig_standard, 4840) 


