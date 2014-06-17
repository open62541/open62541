AM_CFLAGS = $(GLOBAL_AM_CFLAGS) -I$(top_builddir)/src -I. -I$(top_builddir)/src/util
TOOL_DIR = $(top_builddir)/tools
SCHEMA_DIR = $(top_builddir)/schema
lib_LTLIBRARIES = libopen62541.la
lib_LTLIBRARIES: generate_types generate_namespace_0
libopen62541_la_LDFLAGS = -avoid-version -no-undefined
libopen62541_la_SOURCES = ua_types.c \
						  ua_types_encoding_binary.c \
						  ua_types_generated.c \
						  ua_types_encoding_xml.c \
						  ua_namespace_0.c \
						  util/ua_util.c \
						  util/ua_list.c \
						  util/ua_indexedList.c \
						  util/ua_base64.c \
						  ua_transport.c \
						  ua_transport_binary.c \
						  ua_transport_binary_secure.c \
						  ua_namespace.c \
						  ua_namespace_xml.c \
						  ua_services_attribute.c \
						  ua_services_session.c \
						  ua_services_discovery.c \
						  ua_services_securechannel.c \
						  ua_services_nodemanagement.c \
						  ua_services_view.c \
						  ua_services_subscription.c\
						  ua_services_monitoreditems.c\
						  ua_application.c \
						  ua_xml.c
ua_types.c: ua_namespace_0.h
ua_namespace_0.h: ua_types_generated.h

ua_types_generated.c ua_types_generated.h: $(SCHEMA_DIR)/Opc.Ua.Types.bsd $(TOOL_DIR)/generate_builtin.py
	python $(TOOL_DIR)/generate_builtin.py $(SCHEMA_DIR)/Opc.Ua.Types.bsd ua_types_generated

ua_namespace_0.c ua_namespace_0.h: $(SCHEMA_DIR)/NodeIds.csv $(TOOL_DIR)/generate_namespace.py
	python $(TOOL_DIR)/generate_namespace.py $(SCHEMA_DIR)/NodeIds.csv ua_namespace_0

.PHONY: convenience-link clean-convenience-link

convenience-link: $(lib_LTLIBRARIES)
	@test -e "$(top_builddir)/lib" || mkdir "$(top_builddir)/lib"
	@for soname in `echo | $(EGREP) "^dlname=" $^ | $(SED) -e "s|^dlname='\(.*\)'|\1|"`; do  \
		echo "$$soname: creating convenience link from $(abs_builddir)/.libs to $(top_builddir)/lib"; \
		rm -f $(top_builddir)/lib/$$soname ; \
		test -e "$(abs_builddir)/.libs/$$soname" && \
		cd "$(top_builddir)/lib" && \
		$(LN_S) "$(abs_builddir)/.libs/$$soname" "$$soname" || true;\
	done
	@for aname in `echo | $(EGREP) "^dlname=" $^ | $(SED) -e "s|^dlname='\(.*\)\.\(.*\)'|\1\.a|"`; do  \
		echo "$$aname: creating convenience link from $(abs_builddir)/.libs to $(top_builddir)/lib"; \
		rm -f "$(top_builddir)/lib/$$aname" ; \
		test -e "$(abs_builddir)/.libs/$$aname" && \
		cd "$(top_builddir)/lib" && \
		$(LN_S) "$(abs_builddir)/.libs/$$aname" "$$aname" || true;\
	done

clean-convenience-link:
	rm -rf $(top_builddir)/lib/ || true;

all-local: convenience-link

clean-local: clean-convenience-link
	rm -rf ua_types_generated.c ua_types_generated.h || true;
	rm -rf ua_namespace_0.c ua_namespace_0.h || true;
