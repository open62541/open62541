/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information.
 *
 * Copyright 2023 (c) Fraunhofer IOSB (Author: Kai Huebl)
 *
 */

#include "san_mbedtls.h"

#ifdef UA_ENABLE_ENCRYPTION_MBEDTLS


#include <mbedtls/platform.h>
#include <mbedtls/asn1write.h>
#include <mbedtls/oid.h>
#include <mbedtls/asn1.h>

#define MBEDTLS_SAN_MAX_LEN    64

#define SAN_CHK_ASN1_ADD(s, b, f) 						\
    do                                                  \
    {                                                   \
        if((ret = (f)) < 0) {                       	\
			mbedtls_free(b);                            \
            return ret;                                 \
        } else {                                        \
            (s) += (size_t)ret;                         \
        }												\
} while( 0 )


static san_mbedtls_san_list_entry_t* san_mbedtls_san_list_entry_new(void)
{
	san_mbedtls_san_list_entry_t* san_list_entry = NULL;

	san_list_entry = (san_mbedtls_san_list_entry_t*)mbedtls_calloc(1, sizeof(san_mbedtls_san_list_entry_t));
	memset(san_list_entry, 0x00, sizeof(san_mbedtls_san_list_entry_t));

	return san_list_entry;
}

void san_mbedtls_san_list_entry_free(san_mbedtls_san_list_entry_t* san_list_entry)
{
	/* Check parameter */
	if (san_list_entry == NULL) return;

	/* Delete all entries in chain */
	san_mbedtls_san_list_entry_t* cur = san_list_entry;
	while (cur != NULL) {
		san_list_entry = cur;
		cur = cur->next;
		mbedtls_free(san_list_entry);
	}
}

static size_t san_mbedtls_san_list_size(const san_mbedtls_san_list_entry_t* san_list)
{
	/* check parameter */
	if (san_list == NULL) return 0;

	/* Count entries in san list */
	size_t count = 0;
	const san_mbedtls_san_list_entry_t* cur = san_list;
	while (cur != NULL) {
		count++;
		cur = cur->next;
	}

	return count;
}

#if MBEDTLS_VERSION_NUMBER < 0x02130000
static void mbedtls_asn1_sequence_free(
	mbedtls_asn1_sequence* seq
)
{
	mbedtls_asn1_sequence* cur = seq->next;
	while (cur != NULL) {
	    mbedtls_asn1_sequence* tmp_seq = cur;
	    cur = cur->next;
	    free(tmp_seq);
	}
}
#endif

static void san_mbedtls_san_add_entry(
    san_mbedtls_san_list_entry_t** san_list,
	mbedtls_x509_sequence* cur
)
{
	/* Create new san entry */
	san_mbedtls_san_list_entry_t* san_list_entry = san_mbedtls_san_list_entry_new();
	if (san_list_entry == NULL) {
		return;
	}

	san_list_entry->san.type = cur->buf.tag;
    memcpy(&san_list_entry->san.san.unstructured_name, &cur->buf, sizeof(cur->buf));
    san_list_entry->next = *san_list;
    *san_list = san_list_entry;
}

static san_mbedtls_san_list_entry_t* san_mbedtls_san_sequence_create(
	const mbedtls_x509_crt* cert
)
{
	int ret = 0;
	mbedtls_x509_buf oid;
	san_mbedtls_san_list_entry_t* san_list = NULL;
	mbedtls_asn1_sequence san_seq;

	unsigned char* p = cert->v3_ext.p;
	unsigned char* end = cert->v3_ext.p  + cert->v3_ext.len;

	/* Get V3 extension sequence */
	mbedtls_asn1_sequence v3_ext_seq;
	v3_ext_seq.next = NULL;
	ret = mbedtls_asn1_get_sequence_of(&p, end, &v3_ext_seq, MBEDTLS_ASN1_CONSTRUCTED | MBEDTLS_ASN1_SEQUENCE);
    if (ret != 0) {
    	return NULL;
    }

	/* Get Object MBEDTLS_OID_SUBJECT_ALT_NAME from sequence */
    mbedtls_asn1_sequence* cur = &v3_ext_seq;
    while (cur != NULL) {

    	/* Get oid */
    	size_t len;
    	p = cur->buf.p;
        end = cur->buf.p + cur->buf.len;
        ret = mbedtls_asn1_get_tag(&p, end, &len, MBEDTLS_ASN1_OID);
        if (ret != 0) {
        	cur = cur->next;
        	continue;
        }

        oid.tag = MBEDTLS_ASN1_OID;
        oid.len = len;
        oid.p = p;

        /* Handle only SNA attributes */
        if( MBEDTLS_OID_CMP(MBEDTLS_OID_SUBJECT_ALT_NAME, &oid ) != 0) {
        	cur = cur->next;
        	continue;
        }
        p = p + len;

        /* Handle Octet string */
        ret = mbedtls_asn1_get_tag(&p, end, &len, MBEDTLS_ASN1_OCTET_STRING);
        if (ret != 0) {
        	cur = cur->next;
        	continue;
        }
        end = p + len;

        /* Handle Sequence  */
        ret = mbedtls_asn1_get_tag(&p, end, &len, MBEDTLS_ASN1_CONSTRUCTED | MBEDTLS_ASN1_SEQUENCE);
        if (ret != 0) {
            cur = cur->next;
         	continue;
        }
        end = p + len;

        /* Handle SNA attributes */
        while (p < end) {
        	/* Get attribute type */
        	int type = p[0] & 0xFF;
        	p++;

        	/* Get attribute len */
        	ret = mbedtls_asn1_get_len(&p, end, &len);
        	if (ret != 0) {
        		break;
        	}

        	switch (type)
            {
                case MBEDTLS_ASN1_CONTEXT_SPECIFIC | MBEDTLS_X509_SAN_DNS_NAME:
				{
					/* Add DNS name to sna list */
					san_seq.buf.tag = MBEDTLS_X509_SAN_DNS_NAME;
					san_seq.buf.len = len;
					san_seq.buf.p = p;
					san_seq.next = NULL;
					san_mbedtls_san_add_entry(&san_list, &san_seq);
					break;
				}
                case MBEDTLS_ASN1_CONTEXT_SPECIFIC | MBEDTLS_X509_SAN_UNIFORM_RESOURCE_IDENTIFIER:
 				{
 					/* Add URO name to sna ist */
 					san_seq.buf.tag = MBEDTLS_X509_SAN_UNIFORM_RESOURCE_IDENTIFIER;
 					san_seq.buf.len = len;
 					san_seq.buf.p = p;
 					san_seq.next = NULL;
 					san_mbedtls_san_add_entry(&san_list, &san_seq);
 					break;
 				}
                case MBEDTLS_ASN1_CONTEXT_SPECIFIC | MBEDTLS_X509_SAN_IP_ADDRESS:
 				{
 					/* Add IP address to sna list */
 					san_seq.buf.tag = MBEDTLS_X509_SAN_IP_ADDRESS;
 					san_seq.buf.len = len;
 					san_seq.buf.p = p;
 					san_seq.next = NULL;
 					san_mbedtls_san_add_entry(&san_list, &san_seq);
 					break;
 				}
            }
        	p = p + len;
        }

    	cur = cur->next;
    }

    mbedtls_asn1_sequence_free(&v3_ext_seq);
	return san_list;
}

san_mbedtls_san_list_entry_t* san_mbedtls_get_san_list_from_cert(
	const mbedtls_x509_crt* cert
)
{
	/* Check parameter */
	if (cert == NULL) {
		return NULL;
	}

#if MBEDTLS_VERSION_NUMBER < 0x02130000
	return san_mbedtls_san_sequence_create(cert);
#else

	/* Read subject alternate names from certificate */
	san_mbedtls_san_list_entry_t* san_list = NULL;
	const mbedtls_x509_sequence* cur = &cert->subject_alt_names;

    while (cur != NULL) {
    	san_mbedtls_san_list_entry_t* san_list_entry = NULL;

    	int type = 0;
    	switch (cur->buf.tag & (MBEDTLS_ASN1_TAG_CLASS_MASK | MBEDTLS_ASN1_TAG_VALUE_MASK))
    	{
    		case (MBEDTLS_ASN1_CONTEXT_SPECIFIC | MBEDTLS_X509_SAN_DNS_NAME):
			{
    			type = MBEDTLS_X509_SAN_DNS_NAME;
    	        break;
			}
    		case (MBEDTLS_ASN1_CONTEXT_SPECIFIC | MBEDTLS_X509_SAN_UNIFORM_RESOURCE_IDENTIFIER):
			{
    			type = MBEDTLS_X509_SAN_UNIFORM_RESOURCE_IDENTIFIER;
    			break;
			}
    		case (MBEDTLS_ASN1_CONTEXT_SPECIFIC | MBEDTLS_X509_SAN_IP_ADDRESS):
			{
    			type = MBEDTLS_X509_SAN_IP_ADDRESS;
    			break;
			}
     		default:
    		{
    			/* Ignore other subject alternate names */
    			cur = cur->next;
    			continue;
    		}
    	}

		/* Add to sna ist */
        mbedtls_asn1_sequence san_seq;
		san_seq.buf.tag = type;
		san_seq.buf.len = sizeof(cur->buf);
		san_seq.buf.p = type;
		san_seq.next = NULL;
		san_mbedtls_san_add_entry(&san_list, &san_seq);

    	cur = cur->next;
    }

	return san_list;
#endif
}

int san_mbedtls_set_san_list_to_csr(mbedtls_x509write_csr* req,
		                             const san_mbedtls_san_list_entry_t* san_list)
{
	int ret = 0;

	/* check parameter */
	if (req == NULL || san_list == NULL) return 0;

	/* Calculate size of extension buffer */
	size_t san_list_size = san_mbedtls_san_list_size(san_list);
	if (san_list_size == 0) return 1;
	size_t ext_buf_size = (san_list_size * (MBEDTLS_SAN_MAX_LEN +2)) + 2;

	/* Create extension buffer */
	unsigned char* ext_buf = (unsigned char*)mbedtls_calloc(1, ext_buf_size);
	if (ext_buf == NULL) return 0;
	memset(ext_buf, 0x00, ext_buf_size);

	/* Write san entries to extension buffer */
	const san_mbedtls_san_list_entry_t* cur = san_list;
	unsigned char* pc = ext_buf + ext_buf_size;
	size_t len = 0;
	while (cur != NULL) {
		switch (cur->san.type)
		{
			case MBEDTLS_X509_SAN_DNS_NAME:
			case MBEDTLS_X509_SAN_UNIFORM_RESOURCE_IDENTIFIER:
			case MBEDTLS_X509_SAN_IP_ADDRESS:
			{
				/* Write variable to extension buffer */
				SAN_CHK_ASN1_ADD(len, ext_buf, mbedtls_asn1_write_raw_buffer(
					&pc, ext_buf, cur->san.san.unstructured_name.p, cur->san.san.unstructured_name.len));

				SAN_CHK_ASN1_ADD(len, ext_buf, mbedtls_asn1_write_len(
					&pc, ext_buf, cur->san.san.unstructured_name.len));

				SAN_CHK_ASN1_ADD(len, ext_buf, mbedtls_asn1_write_tag(
					&pc, ext_buf, MBEDTLS_ASN1_CONTEXT_SPECIFIC | (unsigned char)cur->san.type));

				break;
			}
	   		default:
	    	{
	    		/* Ignore other subject alternate names */
	    	}
		}
		cur = cur->next;
	}
	if (len == 0) {
		mbedtls_free(ext_buf);
		return 1;
	}

	/* Write sequence info to extension buffer */
	SAN_CHK_ASN1_ADD(len, ext_buf, mbedtls_asn1_write_len(&pc, ext_buf, len));
	SAN_CHK_ASN1_ADD(len, ext_buf, mbedtls_asn1_write_tag(&pc, ext_buf, MBEDTLS_ASN1_CONSTRUCTED | MBEDTLS_ASN1_SEQUENCE));

	/* Write extension buffer to CSR */
	ret = mbedtls_x509write_csr_set_extension(
			req, MBEDTLS_OID_SUBJECT_ALT_NAME, MBEDTLS_OID_SIZE(MBEDTLS_OID_SUBJECT_ALT_NAME),
            ext_buf + ext_buf_size - len, len);
	if (ret != 0) {
		mbedtls_free(ext_buf);
		return 0;
	}

	mbedtls_free(ext_buf);
	return 1;
}

bool san_mbedtls_get_uniform_resource_identifier(
	san_mbedtls_san_list_entry_t* san_list,
	UA_String* uniform_resource_identifier
)
{
	/* Check parameter */
	if (san_list == NULL || uniform_resource_identifier == NULL) {
		return false;
	}

	UA_String_init(uniform_resource_identifier);

	const san_mbedtls_san_list_entry_t* cur = san_list;
	while (cur != NULL) {
		if (cur->san.type == MBEDTLS_X509_SAN_UNIFORM_RESOURCE_IDENTIFIER) {
			uniform_resource_identifier->length = cur->san.san.unstructured_name.len;
			uniform_resource_identifier->data =  cur->san.san.unstructured_name.p;
			return true;
		}
		cur = cur->next;
	}
	return false;
}

#endif
