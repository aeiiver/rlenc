/*---------------------- rlenc.h - Run-length encoding -----------------------\

USAGE
        Include these lines in *one* source file:

            #define RLENC_IMPLEMENTATION
            #include "rlenc.h"

        In your other source files, you can include this file as usual.

OPTIONS
        You may change the way this file is included with these macros:

            // Include the implementation
            #define RLENC_IMPLEMENTATION

            // Make the implementation static
            #define RLENC_STATIC

            // Define the assert macro to use in this library
            #define RLENC_ASSERT(expr) assert(expr)
            // Or remove assertions
            // #define RLENC_ASSERT(expr)

            // Include a main function that will run the tests
            #define RLENC_TEST

API
        rlenc_enc   Encode a sequence of bytes.

        rlenc_dec   Decode a sequence of bytes.

        rlenc_free  Free a successful result. If an unsuccessful
                    result is provided, this will lead to undefined
                    behavior if RLENC_ASSERT is set as empty.

ALLOCATOR
        The 'rlenc_enc' and 'rlenc_dec' functions need a way to
        allocate some memory.  You can provide custom allocator
        callbacks through the 'rlenc_allocer' type. If your callbacks
        need some context data in order to work, set 'ctx' to point
        to this data.

        For convenience, you can use the 'rlenc_allocer' macro to
        create an allocator. If you don't know what allocator to use
        yet, use 'malloc' and 'free' from <stdlib.h> and create an
        allocator like this:

            rlenc_allocer all = rlenc_allocer(malloc, free, 0);

LICENSE
        MIT License or Public Domain, whichever fits your requirements.
        See more information at the end of this file.
\----------------------------------------------------------------------------*/

#ifndef RLENC_H
#define RLENC_H

#ifdef RLENC_STATIC
#undef RLENC_STATIC
#define RLENC_STATIC static
#else
#define RLENC_STATIC
#endif // RLENC_STATIC

#include <stddef.h>
#include <stdint.h>

#define rlenc_allocer(malloc, free, ctx)           \
    ((rlenc_allocer) {                             \
        (void *(*)(ptrdiff_t, void *))malloc,      \
        (void (*)(void *, ptrdiff_t, void *))free, \
        (void *)ctx,                               \
    })

typedef struct {
    void *(*malloc)(ptrdiff_t size, void *ctx);
    void (*free)(void *ptr, ptrdiff_t size, void *ctx);
    void *ctx;
} rlenc_allocer;

typedef enum {
    RLENC_OK = 0, // Successful
    RLENC_NOT_RLENCODED = -128, // Provided data is not Run-length-encoded
} rlenc_status;

typedef struct {
    int status;
    uint8_t *data;
    ptrdiff_t len;
    rlenc_allocer _all;
} rlenc_result;

typedef struct {
    int status;
    ptrdiff_t count;
    uint8_t byte;
    ptrdiff_t len;
} _rlenc_read_result;

RLENC_STATIC rlenc_result rlenc_enc(uint8_t *data, ptrdiff_t len, rlenc_allocer all);
RLENC_STATIC rlenc_result rlenc_dec(uint8_t *data, ptrdiff_t len, rlenc_allocer all);
RLENC_STATIC void rlenc_free(rlenc_result res);
RLENC_STATIC ptrdiff_t _rlenc_write(uint8_t *dst, ptrdiff_t count, uint8_t byte);
RLENC_STATIC _rlenc_read_result _rlenc_read(uint8_t *src);

#endif // RLENC_H

#ifdef RLENC_IMPLEMENTATION

#ifndef RLENC_ASSERT
#include <assert.h>
#define RLENC_ASSERT(expr) assert(expr)
#endif // RLENC_NO_ASSERT

RLENC_STATIC rlenc_result rlenc_enc(uint8_t *data, ptrdiff_t len, rlenc_allocer all)
{
    rlenc_result res;
    res.data = all.malloc(len, all.ctx);
    res._all = all;

    uint8_t *cur = res.data;
    uint8_t byte = data[0];
    ptrdiff_t count = 1;

    for (ptrdiff_t i = 1;; i += 1) {
        if (i == len) {
            cur += _rlenc_write(cur, count, byte);
            break;
        } else if (data[i] != byte) {
            cur += _rlenc_write(cur, count, byte);
            byte = data[i];
            count = 1;
        } else {
            count += 1;
        }
    }

    res.status = RLENC_OK;
    res.len = cur - res.data;
    return res;
}

RLENC_STATIC rlenc_result rlenc_dec(uint8_t *data, ptrdiff_t len, rlenc_allocer all)
{
    rlenc_result res;
    res._all = all;

    res.len = 0;
    uint8_t *readcur = data;

    for (; readcur < data + len;) {
        _rlenc_read_result read = _rlenc_read(readcur);

        if (read.status < 0) {
            res.status = read.status;
            return res;
        }
        readcur += read.len;
        res.len += read.count;
    }

    res.data = all.malloc(res.len, all.ctx);
    readcur = data;
    uint8_t *writecur = res.data;

    for (; readcur < data + len;) {
        _rlenc_read_result read = _rlenc_read(readcur);

        if (read.status < 0) {
            res.status = read.status;
            return res;
        }
        readcur += read.len;

        for (ptrdiff_t j = 0; j < read.count; j += 1) {
            *writecur++ = read.byte;
        }
    }

    res.status = RLENC_OK;
    res.len = writecur - res.data;
    return res;
}

RLENC_STATIC void rlenc_free(rlenc_result res)
{
    RLENC_ASSERT(res.status == RLENC_OK);
    res._all.free(res.data, res.len, res._all.ctx);
}

RLENC_STATIC ptrdiff_t _rlenc_write(uint8_t *dst, ptrdiff_t count, uint8_t byte)
{
    char digs[20];
    uint8_t nb_digs = 0;

    while (count > 0) {
        digs[nb_digs++] = count % 10;
        count /= 10;
    }
    for (ptrdiff_t i = 0; i < nb_digs; i += 1) {
        dst[i] = '0' + digs[nb_digs - i - 1];
    }
    dst[nb_digs] = byte;

    return nb_digs + 1;
}

RLENC_STATIC _rlenc_read_result _rlenc_read(uint8_t *src)
{
    _rlenc_read_result res;

    uint8_t *cur = src;
    res.count = 0;
    for (; '0' <= *cur && *cur <= '9'; cur += 1) {
        res.count = *cur - '0' + res.count * 10;
    }
    if (cur == src) {
        res.status = RLENC_NOT_RLENCODED;
        return res;
    }
    res.status = RLENC_OK;
    res.byte = *cur;
    res.len = cur - src + 1;
    return res;
}

#ifdef RLENC_TEST
#include <assert.h>
#include <stdlib.h>
#include <string.h>

static void rlenc_result_equals(rlenc_result lhs, rlenc_result rhs)
{
    assert(lhs.status == rhs.status);
    assert(lhs.len == rhs.len);
    assert(memcmp(lhs.data, rhs.data, rhs.len) == 0);
}

int main(void)
{
    /* Valid bytes */
    {
        uint8_t dec[67] = "WWWWWWWWWWWWBWWWWWWWWWWWWBBBWWWWWWWWWWWWWWWWWWWWWWWWBWWWWWWWWWWWWWW";
        uint8_t enc[18] = "12W1B12W3B24W1B14W";
        {
            rlenc_result recv = rlenc_enc(dec, sizeof(dec), rlenc_allocer(malloc, free, 0));
            rlenc_result expc = { 0, enc, sizeof(enc) };
            rlenc_result_equals(expc, recv);
            rlenc_free(recv);
        }
        {
            rlenc_result recv = rlenc_dec(enc, sizeof(enc), rlenc_allocer(malloc, free, 0));
            rlenc_result expc = { 0, dec, sizeof(dec) };
            rlenc_result_equals(expc, recv);
            rlenc_free(recv);
        }
    }

    /* Invalid bytes */
    {
        uint8_t enc[19] = "12W1B12W3B24W1B14WW";
        {
            rlenc_result recv = rlenc_dec(enc, sizeof(enc), rlenc_allocer(malloc, free, 0));
            assert(recv.status == RLENC_NOT_RLENCODED);
        }
    }

    return 0;
}
#endif // RLENC_TEST
#endif // RLENC_IMPLEMENTATION

/*----------------------------------------------------------------------------\

---------------------------
ALTERNATIVE A - MIT License
---------------------------

Copyright © 2023 aeiiver

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the “Software”),
to deal in the Software without restriction, including without limitation the
rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
sell copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.

-----------------------------------------
ALTERNATIVE B - Public Domain (Unlicense)
-----------------------------------------

This is free and unencumbered software released into the public domain.

Anyone is free to copy, modify, publish, use, compile, sell, or distribute
this software, either in source code form or as a compiled binary, for any
purpose, commercial or non-commercial, and by any means.

In jurisdictions that recognize copyright laws, the author or authors of
this software dedicate any and all copyright interest in the software to
the public domain. We make this dedication for the benefit of the public
at large and to the detriment of our heirs and successors. We intend this
dedication to be an overt act of relinquishment in perpetuity of all present
and future rights to this software under copyright law.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

For more information, please refer to <http://unlicense.org/>

\----------------------------------------------------------------------------*/
