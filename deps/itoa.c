/*
 * Copyright 2017 Techie Delight
 * Permission is hereby granted, free of charge, to any person obtaining a copy 
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights 
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell 
 * copies of the Software, and to permit persons to whom the Software is 
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included 
 * in all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS 
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE 
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, 
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF 
 * OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

/* Originally released by techiedelight.com 
 * (http://www.techiedelight.com/implement-itoa-function-in-c/) under the
 * MIT license. */

#include "itoa.h"

static void swap(char *x, char *y) {
    char t = *x;
    *x = *y;
    *y = t;
}

/* function to reverse buffer */
static char* reverse(char *buffer, UA_UInt16 i, UA_UInt16 j) {
    while (i < j)
        swap(&buffer[i++], &buffer[j--]);

    return buffer;
}

/* adapted from http://www.techiedelight.com/implement-itoa-function-in-c/ to use UA_... types */
UA_UInt16 itoaUnsigned(UA_UInt64 value, char* buffer, UA_Byte base) {
    /* consider absolute value of number */
    UA_UInt64 n = value;

    UA_UInt16 i = 0;
    while (n) {
        UA_UInt64 r = n % base;

        if (r >= 10)
            buffer[i++] = (char)(65 + (r - 10));
        else
            buffer[i++] = (char)(48 + r);

        n = n / base;
    }
    /* if number is 0 */
    if (i == 0)
        buffer[i++] = '0';

    buffer[i] = '\0'; /* null terminate string */
    i--;
    /* reverse the string */
    reverse(buffer, 0, i);
    i++;
    return i;
}

/* adapted from http://www.techiedelight.com/implement-itoa-function-in-c/ */
UA_UInt16 itoaSigned(UA_Int64 value, char* buffer) {
    /* Special case for UA_INT64_MIN which can not simply be negated */
    /* it will cause a signed integer overflow */
    UA_UInt64 n;
    if(value == UA_INT64_MIN) {
        n = (UA_UInt64)UA_INT64_MAX + 1;
    } else {
        n = (UA_UInt64)value;
        if(value < 0){
            n = (UA_UInt64)-value;
        }
    }

    UA_UInt16 i = 0;
    while(n) {
        UA_UInt64 r = n % 10;
        buffer[i++] = (char)('0' + r);
        n = n / 10;
    }

    if(i == 0)
        buffer[i++] = '0'; /* if number is 0 */
    if(value < 0)
        buffer[i++] = '-';
    buffer[i] = '\0'; /* null terminate string */
    i--;
    reverse(buffer, 0, i); /* reverse the string and return it */
    i++;
    return i;
}

