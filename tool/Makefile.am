opcua.c opcua.h: Opc.Ua.Types.bsd generate_builtin.py
	python generate_builtin.py Opc.Ua.Types.bsd opcua
	
opcua_namespace_0.c opcua_namespace_0.h: NodeIds.csv generate_namespace.py 
	python generate_namespace.py NodeIds.csv opcua_namespace_0

bin_PROGRAMS = test_tool
test_tool_SOURCES = main.c opcua_basictypes.c opcua.c opcua_namespace_0.c opcua.h opcua_namespace_0.h

#optimization levels depending on debug
if DEBUG
        AM_CFLAGS = -O0 $(INCLUDE)
else
        AM_CFLAGS = -O2 $(INCLUDE)
endif
