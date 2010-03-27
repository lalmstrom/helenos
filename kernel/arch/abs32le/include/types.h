/*
 * Copyright (c) 2010 Martin Decky
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * - Redistributions of source code must retain the above copyright
 *   notice, this list of conditions and the following disclaimer.
 * - Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer in the
 *   documentation and/or other materials provided with the distribution.
 * - The name of the author may not be used to endorse or promote products
 *   derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/** @addtogroup abs32le
 * @{
 */
/** @file
 */

#ifndef KERN_abs32le_TYPES_H_
#define KERN_abs32le_TYPES_H_

typedef uint32_t size_t;

typedef uint32_t uintptr_t;
typedef uint32_t pfn_t;

typedef uint32_t ipl_t;

typedef uint32_t unative_t;
typedef int32_t native_t;
typedef uint32_t atomic_count_t;

typedef struct {
} fncptr_t;

#define PRIp  "x"  /**< Format for uintptr_t. */
#define PRIs  "u"  /**< Format for size_t. */

#define PRId8   "d"    /**< Format for int8_t. */
#define PRId16  "d"    /**< Format for int16_t. */
#define PRId32  "d"    /**< Format for int32_t. */
#define PRId64  "lld"  /**< Format for int64_t. */
#define PRIdn   "d"    /**< Format for native_t. */

#define PRIu8   "u"    /**< Format for uint8_t. */
#define PRIu16  "u"    /**< Format for uint16_t. */
#define PRIu32  "u"    /**< Format for uint32_t. */
#define PRIu64  "llu"  /**< Format for uint64_t. */
#define PRIun   "u"    /**< Format for unative_t. */

#define PRIx8   "x"    /**< Format for hexadecimal (u)int8_t. */
#define PRIx16  "x"    /**< Format for hexadecimal (u)int16_t. */
#define PRIx32  "x"    /**< Format for hexadecimal (u)uint32_t. */
#define PRIx64  "llx"  /**< Format for hexadecimal (u)int64_t. */
#define PRIxn   "x"    /**< Format for hexadecimal (u)native_t. */

#endif

/** @}
 */
