lib_LTLIBRARIES = libopen62541.la
libopen62541_la_LDFLAGS = -avoid-version -no-undefined
libopen62541_la_SOURCES = opcua_builtInDatatypes.c\
						opcua_binaryEncDec.c\
						opcua_transportLayer.c\
						opcua_encodingLayer.c\
						opcua_secureChannelLayer.c\
						opcua_memory.c\
						opcua_time.c\
						tcp_layer.c\
						opcua_builtInDatatypes.h\
						opcua_binaryEncDec.h\
						opcua_transportLayer.h\
						opcua_advancedDatatypes.h\
						opcua_types.h\
						opcua_connectionHelper.h\
						opcua_encodingLayer.h\
						opcua_secureChannelLayer.h\
						opcua_memory.h\
						opcua_time.h\
						tcp_layer.h						

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

#optimization levels depending on debug
if DEBUG
        AM_CFLAGS = -O0
else
        AM_CFLAGS = -O2
endif

