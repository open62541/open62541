#optimization levels depending on debug
AM_CFLAGS = $(GLOBAL_AM_CFLAGS) -I$(top_builddir)/include -I$(top_builddir)/src -I. -I$(top_builddir)/src/util


TOOL_DIR = ../tool
#__top_builddir__bin_stackTest_out_SOURCES =	opcuaServer.c\
#											opcua_builtInDatatypes.c\
#											opcua_binaryEncDec.c\
#											ua_transportLayer.c\
#											opcua_builtInDatatypes.h\
#											opcua_binaryEncDec.h\
#											ua_transportLayer.h\
#											opcua_advancedDatatypes.h\
#											opcua_types.h\
#											opcua_connectionHelper.h\
#											tcp_layer.h

#lib_LTLIBRARIES = libstack.la
#libstack_la_SOURCES = ua_transportLayer.c\
#					  ua_transportLayer.h\
#					  opcua_advancedDatatypes.h\
#					  opcua_connectionHelper.h	
lib_LTLIBRARIES = libopen62541.la
libopen62541_la_LDFLAGS = -avoid-version -no-undefined
#libopen62541_la_SOURCES = 			opcua.c\
#						ua_basictypes.c\
#						ua_namespace_0.c\
#						ua_transportLayer.c\
#						ua_secureLayer.c\
#						tcp_layer.c\
#						ua_transportLayer.h\
#						opcua_connectionHelper.h\
#						opcua_encodingLayer.h\
#						ua_secureLayer.h\
#						tcp_layer.h\
#						util/ua_list.c\
#						util/ua_indexedList.c\
#						ua_stackInternalTypes.c\
#						ua_namespace.h\
#						ua_namespace.c

#libopen62541_ladir = $(top_builddir)/include . $(top_builddir)/src . $(top_builddir)/src/util					

#libopen62541_la_HEADERS = opcua.h\
#						ua_basictypes.h\
#						ua_namespace.h\
#						ua_transportLayer.h\
#						opcua_connectionHelper.h\
#						opcua_encodingLayer.h\
#						ua_secureLayer.h\
#						ua_namespace.h\
#						ua_connection.h\
#						ua_stackInternalTypes.h\
#						ua_list.h\
#						ua_indexedList.h

libopen62541_la_SOURCES = opcua.c\
						ua_basictypes.c\
						ua_namespace_0.c\
						util/ua_list.c\
						util/ua_indexedList.c\
						ua_transport.c\
						ua_transport_binary.c\
						ua_transport_binary_secure.c\
						ua_namespace.c\
						ua_services_attribute.c\
						ua_services_session.c\
						ua_services_discovery.c\
						ua_services_securechannel.c\
						ua_services_view.c\
						ua_application.c

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

