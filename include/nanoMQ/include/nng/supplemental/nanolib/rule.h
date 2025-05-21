#ifndef RULE_H
#define RULE_H
#include <stdint.h>
#include "nng/nng.h"
#include "nng/supplemental/util/platform.h"

typedef enum {
	RULE_QOS,
	RULE_ID,
	RULE_TOPIC,
	RULE_CLIENTID,
	RULE_USERNAME,
	RULE_PASSWORD,
	RULE_TIMESTAMP,
	RULE_PAYLOAD_ALL,
	RULE_PAYLOAD_FIELD,
} rule_type;

typedef enum {
	RULE_CMP_NONE,              // compare type init value
	RULE_CMP_EQUAL,             // compare symbol '='
	RULE_CMP_UNEQUAL,           // compare symbol '!=' or '<>'
	RULE_CMP_GREATER,           // compare symbol '>'
	RULE_CMP_LESS,              // compare symbol '<'
	RULE_CMP_GREATER_AND_EQUAL, // compare symbol '>='
	RULE_CMP_LESS_AND_EQUAL,    // compare symbol '<='
} rule_cmp_type;

typedef enum {
	RULE_FORWORD_SQLITE,
	RULE_FORWORD_FDB,
	RULE_FORWORD_MYSQL,
	RULE_FORWORD_REPUB,
	RULE_FORWORD_POSTGRESQL,
	RULE_FORWORD_TIMESCALEDB,
} rule_forword_type;

typedef struct {
	char        **psa;      // payload string array, for multi level json
	char         *pas;      // payload field string or alias
	char         *filter;   // payload field related filter
	rule_cmp_type cmp_type; // payload field compare type
	bool          is_store; // payload value is store or not
	uint8_t       type;     // payload field value type
	void         *value;    // payload field value
} rule_payload;

typedef struct {
	bool    flag[9];
	bool    auto_inc;
	uint8_t type;
	void   *value;
	char  **key_arr;
} rule_key;

typedef struct {
	char    *address;
	uint8_t  proto_ver;
	char    *clientid;
	bool     clean_start;
	char    *username;
	char    *password;
	uint16_t keepalive;
	char    *topic;
	void    *sock;
} repub_t;

typedef struct {
	char *table;
	char *host;
	char *username;
	char *password;
	void *conn;
} rule_mysql;

typedef struct {
	char *table;
	char *host;
	char *username;
	char *password;
	void *conn;
} rule_postgresql;

typedef struct {
	char *table;
	char *host;
	char *username;
	char *password;
	void *conn;
} rule_timescaledb;

typedef struct {
	/*
	** flag[0] == RULE_QOS,
	** flag[1] == RULE_ID,
	** flag[2] == RULE_TOPIC,
	** flag[3] == RULE_CLIENTID,
	** flag[4] == RULE_USERNAME,
	** flag[5] == RULE_PASSWORD,
	** flag[6] == RULE_TIMESTAMP,
	** flag[7] == RULE_PAYLOAD_ALL,
	** flag[8] == RULE_PAYLOAD_FIELD,
	*/
	bool              flag[9]; // if this field need to store
	bool              enabled; // if this rule is enabled
	rule_cmp_type     cmp_type[8];    // filter compare type
	rule_forword_type forword_type;   // forword type
	char          *topic;          // topic parse from sql 'from'
	char          *as[8];          // if field string as a new string
	rule_payload **payload;        // this is for payload info
	char         **filter;         // filter parse from sql 'where'
	char	     *sqlite_table;
	char	     *raw_sql;
	uint32_t          rule_id;
	rule_key         *key;
	repub_t          *repub;
	rule_mysql       *mysql;
	rule_postgresql  *postgresql;
	rule_timescaledb  *timescaledb;
} rule;

typedef struct {
	/*
	** 00000000 == OFF,
	** 00000001 == Sqlite ON,
	** 00000010 == Fdb ON,
	** 00000100 == MySOL ON
	** 00001000 == Repub ON
	** 00010000
	** 00100000
	** 01000000
	*/
	uint8_t option;
	/*
	** rdb[0] == Sqlite
	** rdb[1] == Fdb
	** rdb[2] == MySOL
	** rdb[3] == RePub
	** rdb[4] == PostgreSQL
	** rdb[5] == timescaledb
	*/
	void *rdb[5];
	rule *rules;
	char *sqlite_db;
	char *mysql_db;
	char *postgresql_db;
	char *timescale_db;
	nng_mtx *rule_mutex;
} conf_rule;

int         rule_find_key(const char *str, size_t len);
uint32_t    rule_generate_rule_id(void);
char       *rule_get_key_arr(char *p, rule_key *key);
bool        rule_sql_parse(conf_rule *cr, char *sql);
repub_t    *rule_repub_init(void);
void        rule_repub_free(repub_t *repub);
void        rule_free(rule *r);
bool        rule_mysql_check(rule_mysql *mysql);
void        rule_mysql_free(rule_mysql *mysql);
rule_mysql *rule_mysql_init(void);
bool        rule_postgresql_check(rule_postgresql *postgresql);
void        rule_postgresql_free(rule_postgresql *postgresql);
rule_postgresql *rule_postgresql_init(void);
bool        rule_timescaledb_check(rule_timescaledb *timescaledb);
void        rule_timescaledb_free(rule_timescaledb *timescaledb);
rule_timescaledb *rule_timescaledb_init(void);

#endif
