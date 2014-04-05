#optimization levels depending on debug
AM_CFLAGS = $(GLOBAL_AM_CFLAGS) -I$(top_builddir)/include -I$(top_builddir)/src -I. -I$(top_builddir)/src/util


TOOL_DIR = ../tool
#__top_builddir__bin_stackTest_out_SOURCES =	opcuaServer.c\
#											opcua_builtInDatatypes.c\
#											opcua_binaryEncDec.c\
#											opcua_transportLayer.c\
#											opcua_builtInDatatypes.h\
#											opcua_binaryEncDec.h\
#											opcua_transportLayer.h\
#											opcua_advancedDatatypes.h\
#											opcua_types.h\
#											opcua_connectionHelper.h\
#											tcp_layer.h

#lib_LTLIBRARIES = libstack.la
#libstack_la_SOURCES = opcua_transportLayer.c\
#					  opcua_transportLayer.h\
#					  opcua_advancedDatatypes.h\
#					  opcua_connectionHelper.h	
lib_LTLIBRARIES = libopen62541.la
libopen62541_la_LDFLAGS = -avoid-version -no-undefined
#libopen62541_la_SOURCES = 			opcua.c\
#						opcua_basictypes.c\
#						opcua_namespace_0.c\
#						opcua_transportLayer.c\
#						opcua_secureLayer.c\
#						tcp_layer.c\
#						opcua_transportLayer.h\
#						opcua_connectionHelper.h\
#						opcua_encodingLayer.h\
#						opcua_secureLayer.h\
#						tcp_layer.h\
#						util/UA_list.c\
#						util/UA_indexedList.c\
#						UA_stackInternalTypes.c\
#						opcua_namespace.h\
#						opcua_namespace.c
						
#libopen62541_ladir = $(top_builddir)/include . $(top_builddir)/src . $(top_builddir)/src/util					

#libopen62541_la_HEADERS = opcua.h\
#						opcua_basictypes.h\
#						opcua_namespace.h\
#						opcua_transportLayer.h\
#						opcua_connectionHelper.h\
#						opcua_encodingLayer.h\
#						opcua_secureLayer.h\
#						opcua_namespace.h\
#						UA_connection.h\
#						UA_stackInternalTypes.h\
#						UA_list.h\
#						UA_indexedList.h
						
libopen62541_la_SOURCES = opcua.c\
						opcua_basictypes.c\
						opcua_namespace_0.c\
						UA_stackInternalTypes.c\
						opcua_transportLayer.c\
						opcua_secureLayer.c\
						util/UA_list.c\
						util/UA_indexedList.c\
						opcua_namespace.c\
						UA_stack.c
						
						

#bin_PROGRAMS= $(top_builddir)/bin/open62541.out
#__top_builddir__bin_libOpen62541_out_SOURCES = opcuaServer.c
#libOpen62541_la_LIBADD = ../lib/libOpen62541.la		  										

.PHONY: convenience-link clean-convenience-link

convenience-link: $(lib_LTLIBRARIES)
	@test -e $(top_builddir)/lib || mkdir $(top_builddir)/lib
	@for soname in `echo | $(EGREP) "^dlname=" $^ | $(SED) -e "s|^dlname='\(.*\)'|\1|"`; do  \
		echo "$$soname: creating convenience link from $(abs_builddir)/.libs to $(top_builddir)/lib"; \
		rm -f $(top_builddir)/lib/$$soname ; \
		test -e $(abs_builddir)/.libs/$$soname && \
		cd $(top_builddir)/lib && \
		$(LN_S) $(abs_builddir)/.libs/$$soname $$soname || true;\
	done
	@for aname in `echo | $(EGREP) "^dlname=" $^ | $(SED) -e "s|^dlname='\(.*\)\.\(.*\)'|\1\.a|"`; do  \
		echo "$$aname: creating convenience link from $(abs_builddir)/.libs to $(top_builddir)/lib"; \
		rm -f $(top_builddir)/lib/$$aname ; \
		test -e $(abs_builddir)/.libs/$$aname && \
		cd $(top_builddir)/lib && \
		$(LN_S) $(abs_builddir)/.libs/$$aname $$aname || true;\
	done

clean-convenience-link:
	rm -rf $(top_builddir)/lib/ || true;

all-local: convenience-link

clean-local: clean-convenience-link

