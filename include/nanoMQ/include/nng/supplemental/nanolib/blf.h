#ifndef BLF_H
#define BLF_H
#include "nng/nng.h"
#include "nng/supplemental/nanolib/conf.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct blf_object blf_object;
typedef void (*blf_cb)(blf_object *arg);

typedef enum {
	WRITE_TO_NORMAL1,
	WRITE_TO_TEMP1,
} blf_write_type;

typedef struct {
	uint32_t start_idx;
	uint32_t end_idx;
	char    *filename;
} blf_file_range;

typedef struct {
	blf_file_range **range;
	int              size;
	int              start; // file range start index
} blf_file_ranges;

typedef struct {
	uint8_t *data;
	uint32_t size;
} blf_data_packet;

struct blf_object {
	uint64_t        *keys;
	uint8_t        **darray;
	uint32_t        *dsize;
	uint32_t         size;
	nng_aio         *aio;
	void            *arg;
	blf_file_ranges *ranges;
	blf_write_type   type;
};

blf_object *blf_object_alloc(uint64_t *keys, uint8_t **darray, uint32_t *dsize,
    uint32_t size, nng_aio *aio, void *arg);
void        blf_object_free(blf_object *elem);

int blf_write_batch_async(blf_object *elem);
int blf_write_launcher(conf_blf *conf);

const char  *blf_find(uint64_t key);
const char **blf_find_span(
    uint64_t start_key, uint64_t end_key, uint32_t *size);

#ifdef __cplusplus
}
#endif

#endif
