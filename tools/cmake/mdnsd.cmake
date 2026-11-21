# Prepare the bundled mdnsd sources and register the mDNS discovery driver.
# Included after lib_headers and lib_sources are initialized.

if(UA_ENABLE_DRIVER_MDNS_MDNSD)
    if(NOT UA_ARCHITECTURE_POSIX)
        message(FATAL_ERROR "The bundled mdnsd discovery driver requires UA_ARCHITECTURE=posix")
    endif()

    # Copy mdnsd libmdnsd files into src_generated/mdnsd/.
    set(MDNSD_GENERATED_DIR "${PROJECT_BINARY_DIR}/src_generated/mdnsd")
    file(MAKE_DIRECTORY "${MDNSD_GENERATED_DIR}")

    set(MDNSD_LIBMDNSD_HEADERS 1035.h xht.h sdtxt.h inet.h mdnsd.h)
    set(MDNSD_LIBMDNSD_SOURCES 1035.c xht.c sdtxt.c inet.c mdnsd.c log.c)

    foreach(_f IN LISTS MDNSD_LIBMDNSD_HEADERS MDNSD_LIBMDNSD_SOURCES)
        configure_file(
            "${PROJECT_SOURCE_DIR}/deps/mdnsd/libmdnsd/${_f}"
            "${MDNSD_GENERATED_DIR}/${_f}"
            COPYONLY)
        file(READ "${MDNSD_GENERATED_DIR}/${_f}" _content)
        # Ensure open62541/config.h (which sets _GNU_SOURCE etc.) is included
        # before any system header, then remove any original config.h include.
        string(PREPEND _content "#include \"open62541/config.h\"\n")
        string(REGEX REPLACE "#include \"config\\.h\"\n?" "" _content "${_content}")
        string(REGEX REPLACE "([^A-Za-z0-9_])class([^A-Za-z0-9_])" "\\1clazz\\2"
               _content "${_content}")
        string(REGEX REPLACE "([^A-Za-z0-9_])class([^A-Za-z0-9_])" "\\1clazz\\2"
               _content "${_content}")
        string(REGEX REPLACE "([^A-Za-z0-9_])new([^A-Za-z0-9_])" "\\1new_\\2"
               _content "${_content}")
        string(REGEX REPLACE "([^A-Za-z0-9_])new([^A-Za-z0-9_])" "\\1new_\\2"
               _content "${_content}")
        if(_f STREQUAL "mdnsd.h")
            string(REPLACE "#define DBG(fmt, args...)  mdnsd_log(LOG_DEBUG, \"%s(): \" fmt, __func__, ##args)"
                           "#define DBG(fmt, ...)  mdnsd_log(LOG_DEBUG, \"%s(): \" fmt, __func__, __VA_ARGS__)"
                           _content "${_content}")
            string(REPLACE "#define INFO(fmt, args...) mdnsd_log(LOG_INFO, \"%s(): \" fmt, __func__, ##args)"
                           "#define INFO(fmt, ...) mdnsd_log(LOG_INFO, \"%s(): \" fmt, __func__, __VA_ARGS__)"
                           _content "${_content}")
            string(REPLACE "#define NOTE(fmt, args...) mdnsd_log(LOG_NOTICE, fmt, ##args)"
                           "#define NOTE(...) mdnsd_log(LOG_NOTICE, __VA_ARGS__)"
                           _content "${_content}")
            string(REPLACE "#define WARN(fmt, args...) mdnsd_log(LOG_WARNING, fmt, ##args)"
                           "#define WARN(...) mdnsd_log(LOG_WARNING, __VA_ARGS__)"
                           _content "${_content}")
            string(REPLACE "#define ERR(fmt, args...)  mdnsd_log(LOG_ERR, fmt, ##args)"
                           "#define ERR(...)  mdnsd_log(LOG_ERR, __VA_ARGS__)"
                           _content "${_content}")
        endif()
        if(_f STREQUAL "xht.c")
            string(REPLACE
[=[	_xht_set(h, ckey, cval, 1);]=]
[=[	if (!_xht_set(h, ckey, cval, 1)) {
		free(ckey);
		free(cval);
	}]=]
                _content "${_content}")
        elseif(_f STREQUAL "mdnsd.c")
            string(REPLACE
[=[		c->rr.rdname = strdup(r->known.ns.name);]=]
[=[		if (!r->known.ns.name) {
			free(c->rr.rdata);
			free(c->rr.name);
			free(c);
			return 1;
		}
		c->rr.rdname = strdup(r->known.ns.name);]=]
                _content "${_content}")
            string(REPLACE
[=[		c->rr.rdname = strdup(r->known.srv.name);]=]
[=[		if (!r->known.srv.name) {
			free(c->rr.rdata);
			free(c->rr.name);
			free(c);
			return 1;
		}
		c->rr.rdname = strdup(r->known.srv.name);]=]
                _content "${_content}")
            string(REPLACE
[=[	memset(buf, 0, sizeof(buf));]=]
[=[	memset(buf, 0, sizeof(buf));
	memset(&from, 0, sizeof(from));]=]
                _content "${_content}")
            string(REPLACE
[=[static bool _is_local_ipv4(mdns_daemon_t *d, struct in_addr ip)]=]
[=[static bool _is_local_ipv4(mdns_daemon_t *d, const struct in_addr *ip)]=]
                _content "${_content}")
            string(REPLACE
[=[	if (ip.s_addr == d->addr.s_addr)]=]
[=[	if (ip->s_addr == d->addr.s_addr)]=]
                _content "${_content}")
            string(REPLACE
[=[		if (sin->sin_addr.s_addr == ip.s_addr)]=]
[=[		if (sin->sin_addr.s_addr == ip->s_addr)]=]
                _content "${_content}")
            string(REPLACE
[=[	return _is_local_ipv4(d, ((const struct sockaddr_in *)from)->sin_addr);]=]
[=[	if (inet_family(from) != AF_INET)
		return false;

	const struct sockaddr_in *sin4 = (const struct sockaddr_in *)from;
	return _is_local_ipv4(d, &sin4->sin_addr);]=]
                _content "${_content}")
        endif()
        file(WRITE "${MDNSD_GENERATED_DIR}/${_f}" "${_content}")
    endforeach()

    file(WRITE "${MDNSD_GENERATED_DIR}/config.h"
         "#include \"open62541/config.h\"\n")

    # Suppress warnings from the generated (upstream) mdnsd C files.
    set_source_files_properties(
        ${MDNSD_GENERATED_DIR}/1035.c
        ${MDNSD_GENERATED_DIR}/xht.c
        ${MDNSD_GENERATED_DIR}/sdtxt.c
        ${MDNSD_GENERATED_DIR}/inet.c
        ${MDNSD_GENERATED_DIR}/mdnsd.c
        ${MDNSD_GENERATED_DIR}/log.c
        PROPERTIES COMPILE_FLAGS
            "-Wno-c++-compat -Wno-variadic-macros -Wno-pedantic -Wno-implicit-function-declaration -Wno-nested-externs -Wno-int-to-pointer-cast -Wno-format-nonliteral -Wno-pointer-sign")
    set_source_files_properties(
        ${PROJECT_SOURCE_DIR}/drivers/discovery_mdns_mdnsd.c
        PROPERTIES COMPILE_FLAGS
            "-Wno-variadic-macros")

    list(APPEND lib_headers
        ${MDNSD_GENERATED_DIR}/1035.h
        ${MDNSD_GENERATED_DIR}/xht.h
        ${MDNSD_GENERATED_DIR}/sdtxt.h
        ${MDNSD_GENERATED_DIR}/inet.h
        ${MDNSD_GENERATED_DIR}/mdnsd.h)
    list(APPEND lib_sources
        ${PROJECT_SOURCE_DIR}/drivers/discovery_mdns_mdnsd.c
        ${MDNSD_GENERATED_DIR}/1035.c
        ${MDNSD_GENERATED_DIR}/xht.c
        ${MDNSD_GENERATED_DIR}/sdtxt.c
        ${MDNSD_GENERATED_DIR}/inet.c
        ${MDNSD_GENERATED_DIR}/mdnsd.c
        ${MDNSD_GENERATED_DIR}/log.c)
endif()
