#ifndef PARQUET_H
#define PARQUET_H
#include "nng/nng.h"
#include "nng/supplemental/nanolib/conf.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct parquet_object parquet_object;
typedef void (*parquet_cb)(parquet_object *arg);

typedef enum {
	WRITE_TO_NORMAL,
	WRITE_TO_TEMP,
} parquet_write_type;

typedef struct {
	uint32_t start_idx;
	uint32_t end_idx;
	char    *filename;
} parquet_file_range;

typedef struct {
	parquet_file_range **range;
	int                  size;
	int                  start; // file range start index
} parquet_file_ranges;

typedef struct {
	uint8_t *data;
	uint32_t size;
} parquet_data_packet;

struct parquet_object {
	uint64_t            *keys;
	uint8_t            **darray;
	uint32_t            *dsize;
	uint32_t             size;
	nng_aio             *aio;
	void	        *arg;
	parquet_file_ranges *ranges;
	parquet_write_type   type;
	char                *topic;
};

parquet_object *parquet_object_alloc(uint64_t *keys, uint8_t **darray,
    uint32_t *dsize, uint32_t size, nng_aio *aio, void *arg);
void            parquet_object_free(parquet_object *elem);

parquet_file_range *parquet_file_range_alloc(uint32_t start_idx, uint32_t end_idx, char *filename);
void parquet_file_range_free(parquet_file_range *range);

void parquet_object_set_cb(parquet_object *obj, parquet_cb cb);
int  parquet_write_batch_async(parquet_object *elem);
// Write a batch to a temporary Parquet file, utilize it in scenarios where a single 
// file is sufficient for writing, sending, and subsequent deletion.
int  parquet_write_batch_tmp_async(parquet_object *elem);
int  parquet_write_launcher(conf_parquet *conf);

const char  *parquet_find(uint64_t key);
const char **parquet_find_span(
    uint64_t start_key, uint64_t end_key, uint32_t *size);

parquet_data_packet *parquet_find_data_packet(conf_parquet *conf, char *filename, uint64_t key);

parquet_data_packet **parquet_find_data_packets(conf_parquet *conf, char **filenames, uint64_t *keys, uint32_t len);

parquet_data_packet **parquet_find_data_span_packets(conf_parquet *conf, uint64_t start_key, uint64_t end_key, uint32_t *size, char *topic);

#ifdef __cplusplus
}
#endif

#endif
