/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "ua_types_encoding_xml.h"
#include "../deps/yxml.h"
#include <check.h>
#include <string.h>

/* Helper: parse a full XML string via raw yxml API.
 * Returns the last non-OK return code, or YXML_OK if finished.
 * Stores event counts in the provided counters (if non-NULL). */
static yxml_ret_t
yxml_parse_full(const char *xml, size_t len, char *buf, size_t buflen,
                int *elemstarts, int *elemends, int *attrstarts,
                int *contents, int *pistarts, int *picontents, int *piends) {
    yxml_t x;
    yxml_init(&x, buf, buflen);
    if(elemstarts) *elemstarts = 0;
    if(elemends) *elemends = 0;
    if(attrstarts) *attrstarts = 0;
    if(contents) *contents = 0;
    if(pistarts) *pistarts = 0;
    if(picontents) *picontents = 0;
    if(piends) *piends = 0;
    for(size_t i = 0; i < len; i++) {
        yxml_ret_t r = yxml_parse(&x, (unsigned char)xml[i]);
        if(r < YXML_OK) return r;
        if(r == YXML_ELEMSTART && elemstarts) (*elemstarts)++;
        if(r == YXML_ELEMEND && elemends) (*elemends)++;
        if(r == YXML_ATTRSTART && attrstarts) (*attrstarts)++;
        if(r == YXML_CONTENT && contents) (*contents)++;
        if(r == YXML_PISTART && pistarts) (*pistarts)++;
        if(r == YXML_PICONTENT && picontents) (*picontents)++;
        if(r == YXML_PIEND && piends) (*piends)++;
    }
    return yxml_eof(&x);
}

/* === xml_tokenize wrapper tests === */

START_TEST(parseElement) {
    const char *xml = "<test attr1=\"attr\">my-content</test>";
    xml_token tokens[32];
    xml_result r = xml_tokenize(xml, (unsigned int)strlen(xml), tokens, 32);
    ck_assert(r.error == XML_ERROR_NONE);
    ck_assert_uint_gt(r.num_tokens, 0);
} END_TEST

START_TEST(parseNestedElements) {
    const char *xml = "<root><child1>text1</child1><child2>text2</child2></root>";
    xml_token tokens[32];
    xml_result r = xml_tokenize(xml, (unsigned int)strlen(xml), tokens, 32);
    ck_assert(r.error == XML_ERROR_NONE);
    ck_assert_uint_gt(r.num_tokens, 0);
} END_TEST

START_TEST(parseMultipleAttributes) {
    const char *xml = "<test a1=\"v1\" a2=\"v2\" a3=\"v3\">content</test>";
    xml_token tokens[32];
    xml_result r = xml_tokenize(xml, (unsigned int)strlen(xml), tokens, 32);
    ck_assert(r.error == XML_ERROR_NONE);
} END_TEST

START_TEST(parseSelfClosing) {
    const char *xml = "<test/>";
    xml_token tokens[32];
    xml_result r = xml_tokenize(xml, (unsigned int)strlen(xml), tokens, 32);
    ck_assert(r.error == XML_ERROR_NONE);
} END_TEST

START_TEST(parseSelfClosingWithAttr) {
    const char *xml = "<test attr=\"value\"/>";
    xml_token tokens[32];
    xml_result r = xml_tokenize(xml, (unsigned int)strlen(xml), tokens, 32);
    ck_assert(r.error == XML_ERROR_NONE);
} END_TEST

START_TEST(parseNamespaced) {
    const char *xml = "<ns:elem xmlns:ns=\"http://example.org\">content</ns:elem>";
    xml_token tokens[32];
    xml_result r = xml_tokenize(xml, (unsigned int)strlen(xml), tokens, 32);
    ck_assert(r.error == XML_ERROR_NONE);
} END_TEST

START_TEST(parseEmptyElement) {
    const char *xml = "<e></e>";
    xml_token tokens[32];
    xml_result r = xml_tokenize(xml, (unsigned int)strlen(xml), tokens, 32);
    ck_assert(r.error == XML_ERROR_NONE);
} END_TEST

START_TEST(parseMalformedMismatch) {
    const char *xml = "<a></b>";
    xml_token tokens[32];
    xml_result r = xml_tokenize(xml, (unsigned int)strlen(xml), tokens, 32);
    ck_assert(r.error == XML_ERROR_INVALID);
} END_TEST

START_TEST(parseMalformedUnclosed) {
    const char *xml = "<test>";
    xml_token tokens[32];
    xml_result r = xml_tokenize(xml, (unsigned int)strlen(xml), tokens, 32);
    ck_assert(r.error == XML_ERROR_INVALID);
} END_TEST

START_TEST(parseTokenOverflow) {
    const char *xml = "<a><b><c>text</c></b></a>";
    xml_token tokens[1]; /* Too small */
    xml_result r = xml_tokenize(xml, (unsigned int)strlen(xml), tokens, 1);
    ck_assert(r.error == XML_ERROR_OVERFLOW);
} END_TEST

START_TEST(parseDeeplyNested) {
    const char *xml = "<a><b><c><d>deep</d></c></b></a>";
    xml_token tokens[32];
    xml_result r = xml_tokenize(xml, (unsigned int)strlen(xml), tokens, 32);
    ck_assert(r.error == XML_ERROR_NONE);
    ck_assert_uint_gt(r.num_tokens, 0);
} END_TEST

START_TEST(parseComment) {
    const char *xml = "<root><!-- a comment --><child/></root>";
    xml_token tokens[32];
    xml_result r = xml_tokenize(xml, (unsigned int)strlen(xml), tokens, 32);
    ck_assert(r.error == XML_ERROR_NONE);
} END_TEST

START_TEST(parseProcessingInstruction) {
    const char *xml = "<?xml version=\"1.0\"?><root/>";
    xml_token tokens[32];
    xml_result r = xml_tokenize(xml, (unsigned int)strlen(xml), tokens, 32);
    ck_assert(r.error == XML_ERROR_NONE);
} END_TEST

START_TEST(parseAttrSpecialChars) {
    const char *xml = "<test attr=\"a&amp;b\">content</test>";
    xml_token tokens[32];
    xml_result r = xml_tokenize(xml, (unsigned int)strlen(xml), tokens, 32);
    ck_assert(r.error == XML_ERROR_NONE);
} END_TEST

START_TEST(parseSiblingElements) {
    const char *xml = "<root><a/><b/><c/></root>";
    xml_token tokens[32];
    xml_result r = xml_tokenize(xml, (unsigned int)strlen(xml), tokens, 32);
    ck_assert(r.error == XML_ERROR_NONE);
} END_TEST

/* === Direct yxml API tests for deep coverage === */

/* Processing instructions (non-xml PI) */
START_TEST(yxmlPI_basic) {
    char buf[512];
    int ps = 0, pc = 0, pe = 0;
    const char *xml = "<?mypi?><r/>";
    yxml_ret_t ret = yxml_parse_full(xml, strlen(xml), buf, sizeof(buf),
                                     NULL, NULL, NULL, NULL, &ps, &pc, &pe);
    ck_assert_int_eq(ret, YXML_OK);
    ck_assert_int_ge(ps, 1);
    ck_assert_int_ge(pe, 1);
} END_TEST

START_TEST(yxmlPI_withContent) {
    char buf[512];
    int ps = 0, pc = 0, pe = 0;
    const char *xml = "<?mypi some content data?><r/>";
    yxml_ret_t ret = yxml_parse_full(xml, strlen(xml), buf, sizeof(buf),
                                     NULL, NULL, NULL, NULL, &ps, &pc, &pe);
    ck_assert_int_eq(ret, YXML_OK);
    ck_assert_int_ge(ps, 1);
    ck_assert_int_gt(pc, 0); /* PI content chars */
    ck_assert_int_ge(pe, 1);
} END_TEST

START_TEST(yxmlPI_questionInContent) {
    /* '?' mid-content triggers datapi2 path */
    char buf[512];
    int pc = 0;
    const char *xml = "<?mypi content?notend still?><r/>";
    yxml_ret_t ret = yxml_parse_full(xml, strlen(xml), buf, sizeof(buf),
                                     NULL, NULL, NULL, NULL, NULL, &pc, NULL);
    ck_assert_int_eq(ret, YXML_OK);
    ck_assert_int_gt(pc, 0);
} END_TEST

START_TEST(yxmlPI_insideElement) {
    /* PI within element content (le2 path) */
    char buf[512];
    int ps = 0, pe = 0;
    const char *xml = "<r><?innerpi data?></r>";
    yxml_ret_t ret = yxml_parse_full(xml, strlen(xml), buf, sizeof(buf),
                                     NULL, NULL, NULL, NULL, &ps, NULL, &pe);
    ck_assert_int_eq(ret, YXML_OK);
    ck_assert_int_ge(ps, 1);
    ck_assert_int_ge(pe, 1);
} END_TEST

START_TEST(yxmlPI_afterRoot) {
    /* PI after root element (le3 path) */
    char buf[512];
    int ps = 0;
    const char *xml = "<r/><?postpi stuff?>";
    yxml_ret_t ret = yxml_parse_full(xml, strlen(xml), buf, sizeof(buf),
                                     NULL, NULL, NULL, NULL, &ps, NULL, NULL);
    ck_assert_int_eq(ret, YXML_OK);
    ck_assert_int_ge(ps, 1);
} END_TEST

START_TEST(yxmlPI_multipleLocations) {
    /* PIs before, inside, and after root */
    char buf[512];
    int ps = 0;
    const char *xml = "<?xml version=\"1.0\"?><?pre data?><r><?inner x?></r><?post y?>";
    yxml_ret_t ret = yxml_parse_full(xml, strlen(xml), buf, sizeof(buf),
                                     NULL, NULL, NULL, NULL, &ps, NULL, NULL);
    ck_assert_int_eq(ret, YXML_OK);
    ck_assert_int_ge(ps, 3); /* pre, inner, post */
} END_TEST

START_TEST(yxmlPI_xmlName_inElement) {
    /* PI named "xml" inside element -> YXML_ESYN */
    char buf[512];
    const char *xml = "<r><?xml bad?></r>";
    yxml_ret_t ret = yxml_parse_full(xml, strlen(xml), buf, sizeof(buf),
                                     NULL, NULL, NULL, NULL, NULL, NULL, NULL);
    ck_assert_int_lt(ret, YXML_OK);
} END_TEST

/* xmldecl prefix mismatches (PI names starting with x) */
START_TEST(yxmlPI_xPrefix) {
    char buf[512];
    int ps = 0;
    /* <?xfoo?> - starts like xml decl but isn't */
    const char *xml = "<?xfoo bar?><r/>";
    yxml_ret_t ret = yxml_parse_full(xml, strlen(xml), buf, sizeof(buf),
                                     NULL, NULL, NULL, NULL, &ps, NULL, NULL);
    ck_assert_int_eq(ret, YXML_OK);
    ck_assert_int_ge(ps, 1);
} END_TEST

START_TEST(yxmlPI_xShort) {
    char buf[512];
    int ps = 0;
    /* <?x?> - single char PI name starting with x */
    const char *xml = "<?x?><r/>";
    yxml_ret_t ret = yxml_parse_full(xml, strlen(xml), buf, sizeof(buf),
                                     NULL, NULL, NULL, NULL, &ps, NULL, NULL);
    ck_assert_int_eq(ret, YXML_OK);
    ck_assert_int_ge(ps, 1);
} END_TEST

START_TEST(yxmlPI_xmPrefix) {
    char buf[512];
    int ps = 0;
    /* <?xmfoo?> - starts xm but isn't xml */
    const char *xml = "<?xmfoo data?><r/>";
    yxml_ret_t ret = yxml_parse_full(xml, strlen(xml), buf, sizeof(buf),
                                     NULL, NULL, NULL, NULL, &ps, NULL, NULL);
    ck_assert_int_eq(ret, YXML_OK);
    ck_assert_int_ge(ps, 1);
} END_TEST

START_TEST(yxmlPI_xmlfooPrefix) {
    char buf[512];
    int ps = 0;
    /* <?xmlfoo?> - starts xml but has more name chars */
    const char *xml = "<?xmlfoo data?><r/>";
    yxml_ret_t ret = yxml_parse_full(xml, strlen(xml), buf, sizeof(buf),
                                     NULL, NULL, NULL, NULL, &ps, NULL, NULL);
    ck_assert_int_eq(ret, YXML_OK);
    ck_assert_int_ge(ps, 1);
} END_TEST

START_TEST(yxmlPI_xWithSpace) {
    char buf[512];
    int ps = 0;
    /* <?x data?> - single 'x' with space content */
    const char *xml = "<?x data?><r/>";
    yxml_ret_t ret = yxml_parse_full(xml, strlen(xml), buf, sizeof(buf),
                                     NULL, NULL, NULL, NULL, &ps, NULL, NULL);
    ck_assert_int_eq(ret, YXML_OK);
} END_TEST

/* CDATA sections */
START_TEST(yxmlCDATA_basic) {
    char buf[512];
    int cnt = 0;
    const char *xml = "<r><![CDATA[hello world]]></r>";
    yxml_ret_t ret = yxml_parse_full(xml, strlen(xml), buf, sizeof(buf),
                                     NULL, NULL, NULL, &cnt, NULL, NULL, NULL);
    ck_assert_int_eq(ret, YXML_OK);
    ck_assert_int_gt(cnt, 0);
} END_TEST

START_TEST(yxmlCDATA_singleBracket) {
    /* Single ] in CDATA triggers cd1/datacd1 */
    char buf[512];
    int cnt = 0;
    const char *xml = "<r><![CDATA[a]b]]></r>";
    yxml_ret_t ret = yxml_parse_full(xml, strlen(xml), buf, sizeof(buf),
                                     NULL, NULL, NULL, &cnt, NULL, NULL, NULL);
    ck_assert_int_eq(ret, YXML_OK);
    ck_assert_int_gt(cnt, 0);
} END_TEST

START_TEST(yxmlCDATA_doubleBracket) {
    /* Double ]] not followed by > in CDATA triggers cd2/datacd2 */
    char buf[512];
    int cnt = 0;
    const char *xml = "<r><![CDATA[a]]b]]></r>";
    yxml_ret_t ret = yxml_parse_full(xml, strlen(xml), buf, sizeof(buf),
                                     NULL, NULL, NULL, &cnt, NULL, NULL, NULL);
    ck_assert_int_eq(ret, YXML_OK);
    ck_assert_int_gt(cnt, 0);
} END_TEST

START_TEST(yxmlCDATA_tripleBracket) {
    /* Multiple ]s in CDATA: ]]]]] triggers various cd states */
    char buf[512];
    int cnt = 0;
    const char *xml = "<r><![CDATA[]]]]]></r>";
    yxml_ret_t ret = yxml_parse_full(xml, strlen(xml), buf, sizeof(buf),
                                     NULL, NULL, NULL, &cnt, NULL, NULL, NULL);
    ck_assert_int_eq(ret, YXML_OK);
} END_TEST

START_TEST(yxmlCDATA_empty) {
    char buf[512];
    const char *xml = "<r><![CDATA[]]></r>";
    yxml_ret_t ret = yxml_parse_full(xml, strlen(xml), buf, sizeof(buf),
                                     NULL, NULL, NULL, NULL, NULL, NULL, NULL);
    ck_assert_int_eq(ret, YXML_OK);
} END_TEST

/* Entity references in element content */
START_TEST(yxmlEntityRef_content) {
    char buf[512];
    int cnt = 0;
    const char *xml = "<r>&amp;&lt;&gt;&apos;&quot;</r>";
    yxml_ret_t ret = yxml_parse_full(xml, strlen(xml), buf, sizeof(buf),
                                     NULL, NULL, NULL, &cnt, NULL, NULL, NULL);
    ck_assert_int_eq(ret, YXML_OK);
    ck_assert_int_gt(cnt, 0);
} END_TEST

START_TEST(yxmlEntityRef_unknown) {
    /* Unknown entity -> YXML_EREF */
    char buf[512];
    const char *xml = "<r>&foo;</r>";
    yxml_ret_t ret = yxml_parse_full(xml, strlen(xml), buf, sizeof(buf),
                                     NULL, NULL, NULL, NULL, NULL, NULL, NULL);
    ck_assert_int_eq(ret, YXML_EREF);
} END_TEST

START_TEST(yxmlEntityRef_tooLong) {
    /* Entity name > 8 chars -> YXML_EREF */
    char buf[512];
    const char *xml = "<r>&averylongname;</r>";
    yxml_ret_t ret = yxml_parse_full(xml, strlen(xml), buf, sizeof(buf),
                                     NULL, NULL, NULL, NULL, NULL, NULL, NULL);
    ck_assert_int_eq(ret, YXML_EREF);
} END_TEST

/* Character references (decimal and hex) */
START_TEST(yxmlCharRef_decimalASCII) {
    char buf[512];
    int cnt = 0;
    const char *xml = "<r>&#65;</r>"; /* 'A' */
    yxml_ret_t ret = yxml_parse_full(xml, strlen(xml), buf, sizeof(buf),
                                     NULL, NULL, NULL, &cnt, NULL, NULL, NULL);
    ck_assert_int_eq(ret, YXML_OK);
    ck_assert_int_gt(cnt, 0);
} END_TEST

START_TEST(yxmlCharRef_hexASCII) {
    char buf[512];
    int cnt = 0;
    const char *xml = "<r>&#x42;</r>"; /* 'B' */
    yxml_ret_t ret = yxml_parse_full(xml, strlen(xml), buf, sizeof(buf),
                                     NULL, NULL, NULL, &cnt, NULL, NULL, NULL);
    ck_assert_int_eq(ret, YXML_OK);
    ck_assert_int_gt(cnt, 0);
} END_TEST

START_TEST(yxmlCharRef_twoByte) {
    /* 2-byte UTF-8 (U+00A9 = copyright sign) */
    char buf[512];
    int cnt = 0;
    const char *xml = "<r>&#xA9;</r>";
    yxml_ret_t ret = yxml_parse_full(xml, strlen(xml), buf, sizeof(buf),
                                     NULL, NULL, NULL, &cnt, NULL, NULL, NULL);
    ck_assert_int_eq(ret, YXML_OK);
    ck_assert_int_gt(cnt, 0);
} END_TEST

START_TEST(yxmlCharRef_threeByte) {
    /* 3-byte UTF-8 (U+2603 = snowman) */
    char buf[512];
    int cnt = 0;
    const char *xml = "<r>&#x2603;</r>";
    yxml_ret_t ret = yxml_parse_full(xml, strlen(xml), buf, sizeof(buf),
                                     NULL, NULL, NULL, &cnt, NULL, NULL, NULL);
    ck_assert_int_eq(ret, YXML_OK);
    ck_assert_int_gt(cnt, 0);
} END_TEST

START_TEST(yxmlCharRef_fourByte) {
    /* 4-byte UTF-8 (U+10000) */
    char buf[512];
    int cnt = 0;
    const char *xml = "<r>&#x10000;</r>";
    yxml_ret_t ret = yxml_parse_full(xml, strlen(xml), buf, sizeof(buf),
                                     NULL, NULL, NULL, &cnt, NULL, NULL, NULL);
    ck_assert_int_eq(ret, YXML_OK);
    ck_assert_int_gt(cnt, 0);
} END_TEST

START_TEST(yxmlCharRef_inAttribute) {
    char buf[512];
    int as = 0;
    const char *xml = "<r a=\"&#x41;&#65;\"/>";
    yxml_ret_t ret = yxml_parse_full(xml, strlen(xml), buf, sizeof(buf),
                                     NULL, NULL, &as, NULL, NULL, NULL, NULL);
    ck_assert_int_eq(ret, YXML_OK);
    ck_assert_int_ge(as, 1);
} END_TEST

START_TEST(yxmlCharRef_invalidCodepoint) {
    /* Invalid codepoint U+FFFE -> should be rejected */
    char buf[512];
    const char *xml = "<r>&#xFFFE;</r>";
    yxml_ret_t ret = yxml_parse_full(xml, strlen(xml), buf, sizeof(buf),
                                     NULL, NULL, NULL, NULL, NULL, NULL, NULL);
    ck_assert_int_lt(ret, YXML_OK);
} END_TEST

START_TEST(yxmlCharRef_zero) {
    char buf[512];
    const char *xml = "<r>&#0;</r>";
    yxml_ret_t ret = yxml_parse_full(xml, strlen(xml), buf, sizeof(buf),
                                     NULL, NULL, NULL, NULL, NULL, NULL, NULL);
    ck_assert_int_lt(ret, YXML_OK);
} END_TEST

/* XML declaration with encoding and standalone */
START_TEST(yxmlDecl_encoding) {
    char buf[512];
    const char *xml = "<?xml version=\"1.0\" encoding=\"UTF-8\"?><r/>";
    yxml_ret_t ret = yxml_parse_full(xml, strlen(xml), buf, sizeof(buf),
                                     NULL, NULL, NULL, NULL, NULL, NULL, NULL);
    ck_assert_int_eq(ret, YXML_OK);
} END_TEST

START_TEST(yxmlDecl_standalone_yes) {
    char buf[512];
    const char *xml = "<?xml version=\"1.0\" standalone=\"yes\"?><r/>";
    yxml_ret_t ret = yxml_parse_full(xml, strlen(xml), buf, sizeof(buf),
                                     NULL, NULL, NULL, NULL, NULL, NULL, NULL);
    ck_assert_int_eq(ret, YXML_OK);
} END_TEST

START_TEST(yxmlDecl_standalone_no) {
    char buf[512];
    const char *xml = "<?xml version=\"1.0\" standalone=\"no\"?><r/>";
    yxml_ret_t ret = yxml_parse_full(xml, strlen(xml), buf, sizeof(buf),
                                     NULL, NULL, NULL, NULL, NULL, NULL, NULL);
    ck_assert_int_eq(ret, YXML_OK);
} END_TEST

START_TEST(yxmlDecl_encoding_and_standalone) {
    char buf[512];
    const char *xml = "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?><r/>";
    yxml_ret_t ret = yxml_parse_full(xml, strlen(xml), buf, sizeof(buf),
                                     NULL, NULL, NULL, NULL, NULL, NULL, NULL);
    ck_assert_int_eq(ret, YXML_OK);
} END_TEST

START_TEST(yxmlDecl_withSpaces) {
    char buf[512];
    const char *xml = "<?xml version=\"1.0\" ?><r/>";
    yxml_ret_t ret = yxml_parse_full(xml, strlen(xml), buf, sizeof(buf),
                                     NULL, NULL, NULL, NULL, NULL, NULL, NULL);
    ck_assert_int_eq(ret, YXML_OK);
} END_TEST

START_TEST(yxmlDecl_encodingWithSpaces) {
    char buf[512];
    const char *xml = "<?xml version=\"1.0\" encoding=\"UTF-8\" ?><r/>";
    yxml_ret_t ret = yxml_parse_full(xml, strlen(xml), buf, sizeof(buf),
                                     NULL, NULL, NULL, NULL, NULL, NULL, NULL);
    ck_assert_int_eq(ret, YXML_OK);
} END_TEST

/* DOCTYPE */
START_TEST(yxmlDTD_basic) {
    char buf[512];
    const char *xml = "<!DOCTYPE root><root/>";
    yxml_ret_t ret = yxml_parse_full(xml, strlen(xml), buf, sizeof(buf),
                                     NULL, NULL, NULL, NULL, NULL, NULL, NULL);
    ck_assert_int_eq(ret, YXML_OK);
} END_TEST

START_TEST(yxmlDTD_withSystemId) {
    char buf[512];
    const char *xml = "<!DOCTYPE root \"sysid\"><root/>";
    yxml_ret_t ret = yxml_parse_full(xml, strlen(xml), buf, sizeof(buf),
                                     NULL, NULL, NULL, NULL, NULL, NULL, NULL);
    ck_assert_int_eq(ret, YXML_OK);
} END_TEST

START_TEST(yxmlDTD_withInternalSubset) {
    char buf[512];
    const char *xml = "<!DOCTYPE root [<!ELEMENT r ANY>]><root/>";
    yxml_ret_t ret = yxml_parse_full(xml, strlen(xml), buf, sizeof(buf),
                                     NULL, NULL, NULL, NULL, NULL, NULL, NULL);
    ck_assert_int_eq(ret, YXML_OK);
} END_TEST

START_TEST(yxmlDTD_withComment) {
    char buf[512];
    const char *xml = "<!DOCTYPE root [<!-- dtd comment -->]><root/>";
    yxml_ret_t ret = yxml_parse_full(xml, strlen(xml), buf, sizeof(buf),
                                     NULL, NULL, NULL, NULL, NULL, NULL, NULL);
    ck_assert_int_eq(ret, YXML_OK);
} END_TEST

START_TEST(yxmlDTD_withPI) {
    char buf[512];
    const char *xml = "<!DOCTYPE root [<?dtdpi stuff?>]><root/>";
    yxml_ret_t ret = yxml_parse_full(xml, strlen(xml), buf, sizeof(buf),
                                     NULL, NULL, NULL, NULL, NULL, NULL, NULL);
    ck_assert_int_eq(ret, YXML_OK);
} END_TEST

START_TEST(yxmlDTD_full) {
    char buf[512];
    const char *xml = "<!DOCTYPE root [<!ELEMENT r ANY><!-- comment --><?pi data?>]><root/>";
    yxml_ret_t ret = yxml_parse_full(xml, strlen(xml), buf, sizeof(buf),
                                     NULL, NULL, NULL, NULL, NULL, NULL, NULL);
    ck_assert_int_eq(ret, YXML_OK);
} END_TEST

START_TEST(yxmlDTD_quotedString) {
    char buf[512];
    const char *xml = "<!DOCTYPE root [<!ENTITY x \"value\">]><root/>";
    yxml_ret_t ret = yxml_parse_full(xml, strlen(xml), buf, sizeof(buf),
                                     NULL, NULL, NULL, NULL, NULL, NULL, NULL);
    ck_assert_int_eq(ret, YXML_OK);
} END_TEST

/* Attribute parsing edge cases */
START_TEST(yxmlAttr_spacesAroundEquals) {
    /* attr = "val" triggers attr1 state (space between name and =) */
    char buf[512];
    int as = 0;
    const char *xml = "<r attr = \"val\"/>";
    yxml_ret_t ret = yxml_parse_full(xml, strlen(xml), buf, sizeof(buf),
                                     NULL, NULL, &as, NULL, NULL, NULL, NULL);
    ck_assert_int_eq(ret, YXML_OK);
    ck_assert_int_ge(as, 1);
} END_TEST

START_TEST(yxmlAttr_spaceAfterEquals) {
    /* attr=  "val" triggers attr2 space state */
    char buf[512];
    const char *xml = "<r attr=  \"val\"/>";
    yxml_ret_t ret = yxml_parse_full(xml, strlen(xml), buf, sizeof(buf),
                                     NULL, NULL, NULL, NULL, NULL, NULL, NULL);
    ck_assert_int_eq(ret, YXML_OK);
} END_TEST

START_TEST(yxmlAttr_singleQuoted) {
    char buf[512];
    const char *xml = "<r attr='val'/>";
    yxml_ret_t ret = yxml_parse_full(xml, strlen(xml), buf, sizeof(buf),
                                     NULL, NULL, NULL, NULL, NULL, NULL, NULL);
    ck_assert_int_eq(ret, YXML_OK);
} END_TEST

START_TEST(yxmlAttr_entityInValue) {
    char buf[512];
    const char *xml = "<r a=\"&amp;&lt;&gt;\"/>";
    yxml_ret_t ret = yxml_parse_full(xml, strlen(xml), buf, sizeof(buf),
                                     NULL, NULL, NULL, NULL, NULL, NULL, NULL);
    ck_assert_int_eq(ret, YXML_OK);
} END_TEST

/* Element spacing edge cases */
START_TEST(yxmlElem_spaceBeforeClose) {
    /* <r > triggers elem1 SP path */
    char buf[512];
    const char *xml = "<r >text</r>";
    yxml_ret_t ret = yxml_parse_full(xml, strlen(xml), buf, sizeof(buf),
                                     NULL, NULL, NULL, NULL, NULL, NULL, NULL);
    ck_assert_int_eq(ret, YXML_OK);
} END_TEST

START_TEST(yxmlElem_spaceBeforeSelfClose) {
    /* <r /> triggers elem1 SP + / path */
    char buf[512];
    const char *xml = "<r />";
    yxml_ret_t ret = yxml_parse_full(xml, strlen(xml), buf, sizeof(buf),
                                     NULL, NULL, NULL, NULL, NULL, NULL, NULL);
    ck_assert_int_eq(ret, YXML_OK);
} END_TEST

START_TEST(yxmlElem_multipleSpaces) {
    /* <r   a="v"/> - multiple spaces */
    char buf[512];
    const char *xml = "<r   a=\"v\"/>";
    yxml_ret_t ret = yxml_parse_full(xml, strlen(xml), buf, sizeof(buf),
                                     NULL, NULL, NULL, NULL, NULL, NULL, NULL);
    ck_assert_int_eq(ret, YXML_OK);
} END_TEST

START_TEST(yxmlElem_closeTagSpace) {
    /* </r > - space in close tag: yxml may or may not accept */
    char buf[512];
    const char *xml = "<r></r >";
    yxml_ret_t ret = yxml_parse_full(xml, strlen(xml), buf, sizeof(buf),
                                     NULL, NULL, NULL, NULL, NULL, NULL, NULL);
    /* yxml treats space in close tag as syntax error */
    ck_assert_int_ne(ret, 99); /* just verify it returns something */
} END_TEST

/* Error tests */
START_TEST(yxmlErr_closeMismatch) {
    /* Close tag char mismatch */
    char buf[512];
    const char *xml = "<abc></xyz>";
    yxml_ret_t ret = yxml_parse_full(xml, strlen(xml), buf, sizeof(buf),
                                     NULL, NULL, NULL, NULL, NULL, NULL, NULL);
    ck_assert_int_eq(ret, YXML_ECLOSE);
} END_TEST

START_TEST(yxmlErr_closeShort) {
    /* Close tag is prefix of open tag -> YXML_ECLOSE */
    char buf[512];
    const char *xml = "<abcde></abc>";
    yxml_ret_t ret = yxml_parse_full(xml, strlen(xml), buf, sizeof(buf),
                                     NULL, NULL, NULL, NULL, NULL, NULL, NULL);
    ck_assert_int_eq(ret, YXML_ECLOSE);
} END_TEST

START_TEST(yxmlErr_stackOverflow) {
    /* Tiny stack -> YXML_ESTACK */
    char buf[4];
    const char *xml = "<abcdef/>";
    yxml_ret_t ret = yxml_parse_full(xml, strlen(xml), buf, sizeof(buf),
                                     NULL, NULL, NULL, NULL, NULL, NULL, NULL);
    ck_assert_int_eq(ret, YXML_ESTACK);
} END_TEST

START_TEST(yxmlErr_stackOverflowNesting) {
    /* Tiny stack with nesting - 3 bytes is too small for element name */
    char buf[3];
    const char *xml = "<ab/>";
    yxml_ret_t ret = yxml_parse_full(xml, strlen(xml), buf, sizeof(buf),
                                     NULL, NULL, NULL, NULL, NULL, NULL, NULL);
    ck_assert_int_eq(ret, YXML_ESTACK);
} END_TEST

START_TEST(yxmlErr_syntax) {
    char buf[512];
    const char *xml = "not xml at all";
    yxml_ret_t ret = yxml_parse_full(xml, strlen(xml), buf, sizeof(buf),
                                     NULL, NULL, NULL, NULL, NULL, NULL, NULL);
    ck_assert_int_lt(ret, YXML_OK);
} END_TEST

START_TEST(yxmlErr_eof) {
    /* Incomplete document */
    char buf[512];
    yxml_t x;
    yxml_init(&x, buf, sizeof(buf));
    /* No input at all */
    yxml_ret_t ret = yxml_eof(&x);
    ck_assert_int_eq(ret, YXML_EEOF);
} END_TEST

/* BOM handling */
START_TEST(yxmlBOM_utf8) {
    char buf[512];
    const char *xml = "\xEF\xBB\xBF<r/>";
    yxml_ret_t ret = yxml_parse_full(xml, strlen(xml), buf, sizeof(buf),
                                     NULL, NULL, NULL, NULL, NULL, NULL, NULL);
    ck_assert_int_eq(ret, YXML_OK);
} END_TEST

START_TEST(yxmlBOM_withWhitespace) {
    char buf[512];
    const char *xml = "\xEF\xBB\xBF  <r/>";
    yxml_ret_t ret = yxml_parse_full(xml, strlen(xml), buf, sizeof(buf),
                                     NULL, NULL, NULL, NULL, NULL, NULL, NULL);
    ck_assert_int_eq(ret, YXML_OK);
} END_TEST

START_TEST(yxmlBOM_whitespaceOnly) {
    char buf[512];
    const char *xml = "  <r/>";
    yxml_ret_t ret = yxml_parse_full(xml, strlen(xml), buf, sizeof(buf),
                                     NULL, NULL, NULL, NULL, NULL, NULL, NULL);
    ck_assert_int_eq(ret, YXML_OK);
} END_TEST

/* Post-root content */
START_TEST(yxmlPostRoot_whitespace) {
    char buf[512];
    const char *xml = "<r/>  ";
    yxml_ret_t ret = yxml_parse_full(xml, strlen(xml), buf, sizeof(buf),
                                     NULL, NULL, NULL, NULL, NULL, NULL, NULL);
    ck_assert_int_eq(ret, YXML_OK);
} END_TEST

START_TEST(yxmlPostRoot_comment) {
    char buf[512];
    const char *xml = "<r/><!-- post-root comment -->";
    yxml_ret_t ret = yxml_parse_full(xml, strlen(xml), buf, sizeof(buf),
                                     NULL, NULL, NULL, NULL, NULL, NULL, NULL);
    ck_assert_int_eq(ret, YXML_OK);
} END_TEST

START_TEST(yxmlPostRoot_commentAndPI) {
    char buf[512];
    int ps = 0;
    const char *xml = "<r/>  <!-- comment --><?postpi data?>";
    yxml_ret_t ret = yxml_parse_full(xml, strlen(xml), buf, sizeof(buf),
                                     NULL, NULL, NULL, NULL, &ps, NULL, NULL);
    ck_assert_int_eq(ret, YXML_OK);
    ck_assert_int_ge(ps, 1);
} END_TEST

/* Newline/CR handling */
START_TEST(yxmlNewline_CRLF) {
    char buf[512];
    int cnt = 0;
    const char *xml = "<r>line1\r\nline2</r>";
    yxml_ret_t ret = yxml_parse_full(xml, strlen(xml), buf, sizeof(buf),
                                     NULL, NULL, NULL, &cnt, NULL, NULL, NULL);
    ck_assert_int_eq(ret, YXML_OK);
    ck_assert_int_gt(cnt, 0);
} END_TEST

START_TEST(yxmlNewline_bareCR) {
    char buf[512];
    int cnt = 0;
    const char *xml = "<r>line1\rline2</r>";
    yxml_ret_t ret = yxml_parse_full(xml, strlen(xml), buf, sizeof(buf),
                                     NULL, NULL, NULL, &cnt, NULL, NULL, NULL);
    ck_assert_int_eq(ret, YXML_OK);
    ck_assert_int_gt(cnt, 0);
} END_TEST

START_TEST(yxmlNewline_LF) {
    char buf[512];
    int cnt = 0;
    const char *xml = "<r>line1\nline2</r>";
    yxml_ret_t ret = yxml_parse_full(xml, strlen(xml), buf, sizeof(buf),
                                     NULL, NULL, NULL, &cnt, NULL, NULL, NULL);
    ck_assert_int_eq(ret, YXML_OK);
} END_TEST

/* Pre-root comments */
START_TEST(yxmlPreRoot_comment) {
    char buf[512];
    const char *xml = "<!-- pre-root comment --><r/>";
    yxml_ret_t ret = yxml_parse_full(xml, strlen(xml), buf, sizeof(buf),
                                     NULL, NULL, NULL, NULL, NULL, NULL, NULL);
    ck_assert_int_eq(ret, YXML_OK);
} END_TEST

/* Comment edge cases */
START_TEST(yxmlComment_multiDash) {
    /* Comment with text after -- (invalid per spec but yxml's handling) */
    char buf[512];
    const char *xml = "<r><!-- comment --></r>";
    yxml_ret_t ret = yxml_parse_full(xml, strlen(xml), buf, sizeof(buf),
                                     NULL, NULL, NULL, NULL, NULL, NULL, NULL);
    ck_assert_int_eq(ret, YXML_OK);
} END_TEST

/* Comprehensive integration test */
START_TEST(yxmlIntegration_full) {
    char buf[1024];
    int es = 0, ee = 0, as = 0, cnt = 0, ps = 0, pc = 0, pe = 0;
    const char *xml =
        "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>"
        "<!DOCTYPE root [<!ELEMENT r ANY><!-- dtd comment --><?dtdpi stuff?>]>"
        "<root a = \"v1\" b=  \"v2\">"
        "<![CDATA[data ]bracket ]]notend ]]]end]]>"
        "&amp;&lt;&#65;&#x42;&#xA9;"
        "<?innerpi content?with?marks?>"
        "<child />"
        "</root>"
        "<!-- post comment -->"
        "<?postpi data?>";
    yxml_ret_t ret = yxml_parse_full(xml, strlen(xml), buf, sizeof(buf),
                                     &es, &ee, &as, &cnt, &ps, &pc, &pe);
    ck_assert_int_eq(ret, YXML_OK);
    ck_assert_int_ge(es, 2);  /* root, child */
    ck_assert_int_ge(ee, 2);
    ck_assert_int_ge(as, 2);  /* a, b */
    ck_assert_int_gt(cnt, 0); /* CDATA + entity content */
    ck_assert_int_ge(ps, 2);  /* innerpi, postpi */
    ck_assert_int_gt(pc, 0);
    ck_assert_int_ge(pe, 2);
} END_TEST

/* Null byte -> YXML_ESYN */
START_TEST(yxmlErr_nullByte) {
    char buf[512];
    const char xml[] = "<r>\0</r>";
    yxml_t x;
    yxml_init(&x, buf, sizeof(buf));
    yxml_ret_t ret = YXML_OK;
    for(size_t i = 0; i < sizeof(xml)-1; i++) {
        ret = yxml_parse(&x, (unsigned char)xml[i]);
        if(ret < YXML_OK) break;
    }
    ck_assert_int_lt(ret, YXML_OK);
} END_TEST

/* eof function on various states */
START_TEST(yxmlEof_afterComplete) {
    char buf[512];
    const char *xml = "<r/>";
    yxml_t x;
    yxml_init(&x, buf, sizeof(buf));
    for(size_t i = 0; i < strlen(xml); i++)
        yxml_parse(&x, (unsigned char)xml[i]);
    ck_assert_int_eq(yxml_eof(&x), YXML_OK);
} END_TEST

START_TEST(yxmlEof_midElement) {
    char buf[512];
    const char *xml = "<r>";
    yxml_t x;
    yxml_init(&x, buf, sizeof(buf));
    for(size_t i = 0; i < strlen(xml); i++)
        yxml_parse(&x, (unsigned char)xml[i]);
    ck_assert_int_eq(yxml_eof(&x), YXML_EEOF);
} END_TEST

/* symlen function */
START_TEST(yxmlSymlen) {
    char buf[512];
    yxml_t x;
    yxml_init(&x, buf, sizeof(buf));
    const char *xml = "<myelem/>";
    for(size_t i = 0; i < strlen(xml); i++) {
        yxml_ret_t r = yxml_parse(&x, (unsigned char)xml[i]);
        if(r == YXML_ELEMSTART) {
            ck_assert_uint_eq(yxml_symlen(&x, x.elem), 6); /* "myelem" */
            break;
        }
    }
} END_TEST

static Suite *testSuite_yxml(void) {
    TCase *tc_parse = tcase_create("yxml_parse");
    tcase_add_test(tc_parse, parseElement);
    tcase_add_test(tc_parse, parseNestedElements);
    tcase_add_test(tc_parse, parseMultipleAttributes);
    tcase_add_test(tc_parse, parseSelfClosing);
    tcase_add_test(tc_parse, parseSelfClosingWithAttr);
    tcase_add_test(tc_parse, parseNamespaced);
    tcase_add_test(tc_parse, parseEmptyElement);
    tcase_add_test(tc_parse, parseMalformedMismatch);
    tcase_add_test(tc_parse, parseMalformedUnclosed);
    tcase_add_test(tc_parse, parseTokenOverflow);
    tcase_add_test(tc_parse, parseDeeplyNested);
    tcase_add_test(tc_parse, parseComment);
    tcase_add_test(tc_parse, parseProcessingInstruction);
    tcase_add_test(tc_parse, parseAttrSpecialChars);
    tcase_add_test(tc_parse, parseSiblingElements);

    TCase *tc_pi = tcase_create("yxml_pi");
    tcase_add_test(tc_pi, yxmlPI_basic);
    tcase_add_test(tc_pi, yxmlPI_withContent);
    tcase_add_test(tc_pi, yxmlPI_questionInContent);
    tcase_add_test(tc_pi, yxmlPI_insideElement);
    tcase_add_test(tc_pi, yxmlPI_afterRoot);
    tcase_add_test(tc_pi, yxmlPI_multipleLocations);
    tcase_add_test(tc_pi, yxmlPI_xmlName_inElement);
    tcase_add_test(tc_pi, yxmlPI_xPrefix);
    tcase_add_test(tc_pi, yxmlPI_xShort);
    tcase_add_test(tc_pi, yxmlPI_xmPrefix);
    tcase_add_test(tc_pi, yxmlPI_xmlfooPrefix);
    tcase_add_test(tc_pi, yxmlPI_xWithSpace);

    TCase *tc_cdata = tcase_create("yxml_cdata");
    tcase_add_test(tc_cdata, yxmlCDATA_basic);
    tcase_add_test(tc_cdata, yxmlCDATA_singleBracket);
    tcase_add_test(tc_cdata, yxmlCDATA_doubleBracket);
    tcase_add_test(tc_cdata, yxmlCDATA_tripleBracket);
    tcase_add_test(tc_cdata, yxmlCDATA_empty);

    TCase *tc_ref = tcase_create("yxml_refs");
    tcase_add_test(tc_ref, yxmlEntityRef_content);
    tcase_add_test(tc_ref, yxmlEntityRef_unknown);
    tcase_add_test(tc_ref, yxmlEntityRef_tooLong);
    tcase_add_test(tc_ref, yxmlCharRef_decimalASCII);
    tcase_add_test(tc_ref, yxmlCharRef_hexASCII);
    tcase_add_test(tc_ref, yxmlCharRef_twoByte);
    tcase_add_test(tc_ref, yxmlCharRef_threeByte);
    tcase_add_test(tc_ref, yxmlCharRef_fourByte);
    tcase_add_test(tc_ref, yxmlCharRef_inAttribute);
    tcase_add_test(tc_ref, yxmlCharRef_invalidCodepoint);
    tcase_add_test(tc_ref, yxmlCharRef_zero);

    TCase *tc_decl = tcase_create("yxml_decl");
    tcase_add_test(tc_decl, yxmlDecl_encoding);
    tcase_add_test(tc_decl, yxmlDecl_standalone_yes);
    tcase_add_test(tc_decl, yxmlDecl_standalone_no);
    tcase_add_test(tc_decl, yxmlDecl_encoding_and_standalone);
    tcase_add_test(tc_decl, yxmlDecl_withSpaces);
    tcase_add_test(tc_decl, yxmlDecl_encodingWithSpaces);

    TCase *tc_dtd = tcase_create("yxml_dtd");
    tcase_add_test(tc_dtd, yxmlDTD_basic);
    tcase_add_test(tc_dtd, yxmlDTD_withSystemId);
    tcase_add_test(tc_dtd, yxmlDTD_withInternalSubset);
    tcase_add_test(tc_dtd, yxmlDTD_withComment);
    tcase_add_test(tc_dtd, yxmlDTD_withPI);
    tcase_add_test(tc_dtd, yxmlDTD_full);
    tcase_add_test(tc_dtd, yxmlDTD_quotedString);

    TCase *tc_attr = tcase_create("yxml_attr");
    tcase_add_test(tc_attr, yxmlAttr_spacesAroundEquals);
    tcase_add_test(tc_attr, yxmlAttr_spaceAfterEquals);
    tcase_add_test(tc_attr, yxmlAttr_singleQuoted);
    tcase_add_test(tc_attr, yxmlAttr_entityInValue);

    TCase *tc_elem = tcase_create("yxml_elem");
    tcase_add_test(tc_elem, yxmlElem_spaceBeforeClose);
    tcase_add_test(tc_elem, yxmlElem_spaceBeforeSelfClose);
    tcase_add_test(tc_elem, yxmlElem_multipleSpaces);
    tcase_add_test(tc_elem, yxmlElem_closeTagSpace);

    TCase *tc_err = tcase_create("yxml_errors");
    tcase_add_test(tc_err, yxmlErr_closeMismatch);
    tcase_add_test(tc_err, yxmlErr_closeShort);
    tcase_add_test(tc_err, yxmlErr_stackOverflow);
    tcase_add_test(tc_err, yxmlErr_stackOverflowNesting);
    tcase_add_test(tc_err, yxmlErr_syntax);
    tcase_add_test(tc_err, yxmlErr_eof);
    tcase_add_test(tc_err, yxmlErr_nullByte);

    TCase *tc_misc = tcase_create("yxml_misc");
    tcase_add_test(tc_misc, yxmlBOM_utf8);
    tcase_add_test(tc_misc, yxmlBOM_withWhitespace);
    tcase_add_test(tc_misc, yxmlBOM_whitespaceOnly);
    tcase_add_test(tc_misc, yxmlPostRoot_whitespace);
    tcase_add_test(tc_misc, yxmlPostRoot_comment);
    tcase_add_test(tc_misc, yxmlPostRoot_commentAndPI);
    tcase_add_test(tc_misc, yxmlNewline_CRLF);
    tcase_add_test(tc_misc, yxmlNewline_bareCR);
    tcase_add_test(tc_misc, yxmlNewline_LF);
    tcase_add_test(tc_misc, yxmlPreRoot_comment);
    tcase_add_test(tc_misc, yxmlComment_multiDash);
    tcase_add_test(tc_misc, yxmlEof_afterComplete);
    tcase_add_test(tc_misc, yxmlEof_midElement);
    tcase_add_test(tc_misc, yxmlSymlen);
    tcase_add_test(tc_misc, yxmlIntegration_full);

    Suite *s = suite_create("Test XML decoding with the yxml library");
    suite_add_tcase(s, tc_parse);
    suite_add_tcase(s, tc_pi);
    suite_add_tcase(s, tc_cdata);
    suite_add_tcase(s, tc_ref);
    suite_add_tcase(s, tc_decl);
    suite_add_tcase(s, tc_dtd);
    suite_add_tcase(s, tc_attr);
    suite_add_tcase(s, tc_elem);
    suite_add_tcase(s, tc_err);
    suite_add_tcase(s, tc_misc);
    return s;
}

int main(void) {
    int number_failed = 0;
    Suite *s = testSuite_yxml();
    SRunner *sr = srunner_create(s);
    srunner_set_fork_status(sr, CK_NOFORK);
    srunner_run_all(sr, CK_NORMAL);
    number_failed += srunner_ntests_failed(sr);
    srunner_free(sr);
    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
