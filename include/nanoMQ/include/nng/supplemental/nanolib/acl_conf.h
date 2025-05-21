#ifndef NANOLIB_ACL_CONF_H
#define NANOLIB_ACL_CONF_H

#include <string.h>
#include "nng/nng.h"
#include "nng/supplemental/nanolib/cJSON.h"

typedef enum {
	ACL_ALLOW,
	ACL_DENY,
} acl_permit;

typedef enum {
	ACL_USERNAME,
	ACL_CLIENTID,
	ACL_IPADDR,
	ACL_AND,
	ACL_OR,
	ACL_NONE,
} acl_rule_type;

typedef enum {
	ACL_PUB,
	ACL_SUB,
	ACL_ALL,
} acl_action_type;

typedef enum {
	ACL_RULE_SINGLE_STRING,
	ACL_RULE_STRING_ARRAY,
	ACL_RULE_INT_ARRAY,
	ACL_RULE_ALL,
} acl_value_type;

typedef struct acl_rule_ct {
	acl_value_type type;
	size_t         count;
	union {
		char * str;
		char **str_array;
		int *  int_array;
	} value;
} acl_rule_ct;

typedef struct {
	acl_rule_type rule_type;
	acl_rule_ct   rule_ct;
} acl_sub_rule;

typedef struct {
	size_t         count;
	acl_sub_rule **rules;
} acl_sub_rules_array;

typedef struct acl_rule {
	size_t        id;
	acl_permit    permit;
	acl_rule_type rule_type;
	union {
		acl_rule_ct         ct;
		acl_sub_rules_array array;
	} rule_ct;
	acl_action_type action;
	size_t          topic_count;
	char **         topics;
} acl_rule;

typedef struct {
	bool       enable;
	size_t     rule_count;
	acl_rule **rules;
} conf_acl;

extern void conf_acl_parse(conf_acl *acl, const char *path);
extern void conf_acl_init(conf_acl *acl);
extern void conf_acl_destroy(conf_acl *acl);
extern bool acl_parse_json_rule(cJSON *obj, size_t id, acl_rule **rule);
extern void print_acl_conf(conf_acl *acl);

#endif /* NANOLIB_ACL_CONF_H */
