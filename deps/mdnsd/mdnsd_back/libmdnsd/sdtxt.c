#include "sdtxt.h"
#include <stdlib.h>
#include <string.h>

static size_t _sd2txt_len(const char *key, char *val)
{
	size_t ret = strlen(key);

	if (!*val)
		return ret;

	ret += strlen(val);
	ret++;

	return ret;
}

static void _sd2txt_count(xht_t *h, char *key, void *val, void *arg)
{
	int *count = (int *)arg;

	*count += (int)_sd2txt_len(key, (char *)val) + 1;
}

static void _sd2txt_write(xht_t *h, char *key, void *val, void *arg)
{
	unsigned char **txtp = (unsigned char **)arg;
	char *cval = (char *)val;

	/* Copy in lengths, then strings */
	**txtp = (unsigned char)_sd2txt_len(key, (char *)val);
	(*txtp)++;
	memcpy(*txtp, key, strlen(key));
	*txtp += strlen(key);
	if (!*cval)
		return;

	**txtp = '=';
	(*txtp)++;
	memcpy(*txtp, cval, strlen(cval));
	*txtp += strlen(cval);
}

unsigned char *sd2txt(xht_t *h, int *len)
{
	unsigned char *buf, *raw;

	*len = 0;

	xht_walk(h, _sd2txt_count, (void *)len);
	if (!*len) {
		*len = 1;
		buf = (unsigned char *)MDNSD_malloc(1);
		*buf = 0;
		return buf;
	}

	raw = buf = (unsigned char *)MDNSD_malloc((size_t)(*len));
	xht_walk(h, _sd2txt_write, &buf);

	return raw;
}

xht_t *txt2sd(const unsigned char *txt, int len)
{
	char key[256];
	xht_t *h = 0;

	if (txt == 0 || len == 0 || *txt == 0)
		return 0;

	h = xht_new(23);

	/* Loop through data breaking out each block, storing into hashtable */
	for (; *txt <= len && len > 0; len -= *txt, txt += *txt + 1) {
		char* val;
		if (*txt == 0)
			break;

		memcpy(key, txt + 1, *txt);
		key[*txt] = 0;
		if ((val = strchr(key, '=')) != 0) {
			*val = 0;
			val++;
		}
		if (val != NULL)
			xht_store(h, key, (int)strlen(key), val, (int)strlen(val));
		if (*txt +1 > len)
		    break;
	}

	return h;
}
