/**
* \file config.h
*
* \brief Configuration options (set of defines)
*
*  Copyright (C) 2006-2014, ARM Limited, All Rights Reserved
*
*  This file is part of mbed TLS (https://tls.mbed.org)
*
*  This program is free software; you can redistribute it and/or modify
*  it under the terms of the GNU General Public License as published by
*  the Free Software Foundation; either version 2 of the License, or
*  (at your option) any later version.
*
*  This program is distributed in the hope that it will be useful,
*  but WITHOUT ANY WARRANTY; without even the implied warranty of
*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*  GNU General Public License for more details.
*
*  You should have received a copy of the GNU General Public License along
*  with this program; if not, write to the Free Software Foundation, Inc.,
*  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*
* This set of compile-time options may be used to enable
* or disable features selectively, and reduce the global
* memory footprint.
*/
#ifndef MBEDTLS_CONFIG_H
#define MBEDTLS_CONFIG_H

#if defined(_MSC_VER) && !defined(_CRT_SECURE_NO_DEPRECATE)
#define _CRT_SECURE_NO_DEPRECATE 1
#endif

/**
* \name SECTION: System support
*
* This section sets system specific settings.
* \{
*/

/**
* \def MBEDTLS_HAVE_ASM
*
* The compiler has support for asm().
*
* Requires support for asm() in compiler.
*
* Used in:
*      library/timing.c
*      library/padlock.c
*      include/mbedtls/bn_mul.h
*
* Comment to disable the use of assembly code.
*/
#define MBEDTLS_HAVE_ASM

/**
* \def MBEDTLS_HAVE_SSE2
*
* CPU supports SSE2 instruction set.
*
* Uncomment if the CPU supports SSE2 (IA-32 specific).
*/
//#define MBEDTLS_HAVE_SSE2

/**
* \def MBEDTLS_HAVE_TIME
*
* System has time.h and time().
* The time does not need to be correct, only time differences are used,
* by contrast with MBEDTLS_HAVE_TIME_DATE
*
* Comment if your system does not support time functions
*/
#define MBEDTLS_HAVE_TIME

/**
* \def MBEDTLS_HAVE_TIME_DATE
*
* System has time.h and time(), gmtime() and the clock is correct.
* The time needs to be correct (not necesarily very accurate, but at least
* the date should be correct). This is used to verify the validity period of
* X.509 certificates.
*
* Comment if your system does not have a correct clock.
*/
#define MBEDTLS_HAVE_TIME_DATE

/**
* \def MBEDTLS_PLATFORM_MEMORY
*
* Enable the memory allocation layer.
*
* By default mbed TLS uses the system-provided calloc() and free().
* This allows different allocators (self-implemented or provided) to be
* provided to the platform abstraction layer.
*
* Enabling MBEDTLS_PLATFORM_MEMORY without the
* MBEDTLS_PLATFORM_{FREE,CALLOC}_MACROs will provide
* "mbedtls_platform_set_calloc_free()" allowing you to set an alternative calloc() and
* free() function pointer at runtime.
*
* Enabling MBEDTLS_PLATFORM_MEMORY and specifying
* MBEDTLS_PLATFORM_{CALLOC,FREE}_MACROs will allow you to specify the
* alternate function at compile time.
*
* Requires: MBEDTLS_PLATFORM_C
*
* Enable this layer to allow use of alternative memory allocators.
*/
//#define MBEDTLS_PLATFORM_MEMORY

/**
* \def MBEDTLS_PLATFORM_NO_STD_FUNCTIONS
*
* Do not assign standard functions in the platform layer (e.g. calloc() to
* MBEDTLS_PLATFORM_STD_CALLOC and printf() to MBEDTLS_PLATFORM_STD_PRINTF)
*
* This makes sure there are no linking errors on platforms that do not support
* these functions. You will HAVE to provide alternatives, either at runtime
* via the platform_set_xxx() functions or at compile time by setting
* the MBEDTLS_PLATFORM_STD_XXX defines, or enabling a
* MBEDTLS_PLATFORM_XXX_MACRO.
*
* Requires: MBEDTLS_PLATFORM_C
*
* Uncomment to prevent default assignment of standard functions in the
* platform layer.
*/
//#define MBEDTLS_PLATFORM_NO_STD_FUNCTIONS

/**
* \def MBEDTLS_PLATFORM_XXX_ALT
*
* Uncomment a macro to let mbed TLS support the function in the platform
* abstraction layer.
*
* Example: In case you uncomment MBEDTLS_PLATFORM_PRINTF_ALT, mbed TLS will
* provide a function "mbedtls_platform_set_printf()" that allows you to set an
* alternative printf function pointer.
*
* All these define require MBEDTLS_PLATFORM_C to be defined!
*
* \note MBEDTLS_PLATFORM_SNPRINTF_ALT is required on Windows;
* it will be enabled automatically by check_config.h
*
* \warning MBEDTLS_PLATFORM_XXX_ALT cannot be defined at the same time as
* MBEDTLS_PLATFORM_XXX_MACRO!
*
* Uncomment a macro to enable alternate implementation of specific base
* platform function
*/
//#define MBEDTLS_PLATFORM_EXIT_ALT
//#define MBEDTLS_PLATFORM_FPRINTF_ALT
//#define MBEDTLS_PLATFORM_PRINTF_ALT
//#define MBEDTLS_PLATFORM_SNPRINTF_ALT

/**
* \def MBEDTLS_DEPRECATED_WARNING
*
* Mark deprecated functions so that they generate a warning if used.
* Functions deprecated in one version will usually be removed in the next
* version. You can enable this to help you prepare the transition to a new
* major version by making sure your code is not using these functions.
*
* This only works with GCC and Clang. With other compilers, you may want to
* use MBEDTLS_DEPRECATED_REMOVED
*
* Uncomment to get warnings on using deprecated functions.
*/
//#define MBEDTLS_DEPRECATED_WARNING

/**
* \def MBEDTLS_DEPRECATED_REMOVED
*
* Remove deprecated functions so that they generate an error if used.
* Functions deprecated in one version will usually be removed in the next
* version. You can enable this to help you prepare the transition to a new
* major version by making sure your code is not using these functions.
*
* Uncomment to get errors on using deprecated functions.
*/
//#define MBEDTLS_DEPRECATED_REMOVED

/* \} name SECTION: System support */

/**
* \name SECTION: mbed TLS feature support
*
* This section sets support for features that are or are not needed
* within the modules that are enabled.
* \{
*/

/**
* \def MBEDTLS_TIMING_ALT
*
* Uncomment to provide your own alternate implementation for mbedtls_timing_hardclock(),
* mbedtls_timing_get_timer(), mbedtls_set_alarm(), mbedtls_set/get_delay()
*
* Only works if you have MBEDTLS_TIMING_C enabled.
*
* You will need to provide a header "timing_alt.h" and an implementation at
* compile time.
*/
//#define MBEDTLS_TIMING_ALT

/**
* \def MBEDTLS__MODULE_NAME__ALT
*
* Uncomment a macro to let mbed TLS use your alternate core implementation of
* a symmetric crypto or hash module (e.g. platform specific assembly
* optimized implementations). Keep in mind that the function prototypes
* should remain the same.
*
* This replaces the whole module. If you only want to replace one of the
* functions, use one of the MBEDTLS__FUNCTION_NAME__ALT flags.
*
* Example: In case you uncomment MBEDTLS_AES_ALT, mbed TLS will no longer
* provide the "struct mbedtls_aes_context" definition and omit the base function
* declarations and implementations. "aes_alt.h" will be included from
* "aes.h" to include the new function definitions.
*
* Uncomment a macro to enable alternate implementation of the corresponding
* module.
*/
//#define MBEDTLS_AES_ALT
//#define MBEDTLS_ARC4_ALT
//#define MBEDTLS_BLOWFISH_ALT
//#define MBEDTLS_CAMELLIA_ALT
//#define MBEDTLS_DES_ALT
//#define MBEDTLS_XTEA_ALT
//#define MBEDTLS_MD2_ALT
//#define MBEDTLS_MD4_ALT
//#define MBEDTLS_MD5_ALT
//#define MBEDTLS_RIPEMD160_ALT
//#define MBEDTLS_SHA1_ALT
//#define MBEDTLS_SHA256_ALT
//#define MBEDTLS_SHA512_ALT

/**
* \def MBEDTLS__FUNCTION_NAME__ALT
*
* Uncomment a macro to let mbed TLS use you alternate core implementation of
* symmetric crypto or hash function. Keep in mind that function prototypes
* should remain the same.
*
* This replaces only one function. The header file from mbed TLS is still
* used, in contrast to the MBEDTLS__MODULE_NAME__ALT flags.
*
* Example: In case you uncomment MBEDTLS_SHA256_PROCESS_ALT, mbed TLS will
* no longer provide the mbedtls_sha1_process() function, but it will still provide
* the other function (using your mbedtls_sha1_process() function) and the definition
* of mbedtls_sha1_context, so your implementation of mbedtls_sha1_process must be compatible
* with this definition.
*
* Note: if you use the AES_xxx_ALT macros, then is is recommended to also set
* MBEDTLS_AES_ROM_TABLES in order to help the linker garbage-collect the AES
* tables.
*
* Uncomment a macro to enable alternate implementation of the corresponding
* function.
*/
//#define MBEDTLS_MD2_PROCESS_ALT
//#define MBEDTLS_MD4_PROCESS_ALT
//#define MBEDTLS_MD5_PROCESS_ALT
//#define MBEDTLS_RIPEMD160_PROCESS_ALT
//#define MBEDTLS_SHA1_PROCESS_ALT
//#define MBEDTLS_SHA256_PROCESS_ALT
//#define MBEDTLS_SHA512_PROCESS_ALT
//#define MBEDTLS_DES_SETKEY_ALT
//#define MBEDTLS_DES_CRYPT_ECB_ALT
//#define MBEDTLS_DES3_CRYPT_ECB_ALT
//#define MBEDTLS_AES_SETKEY_ENC_ALT
//#define MBEDTLS_AES_SETKEY_DEC_ALT
//#define MBEDTLS_AES_ENCRYPT_ALT
//#define MBEDTLS_AES_DECRYPT_ALT

/**
* \def MBEDTLS_SELF_TEST
*
* Enable the checkup functions (*_self_test).
*/
//#define MBEDTLS_SELF_TEST

/* \} name SECTION: mbed TLS feature support */

/**
* \name SECTION: mbed TLS modules
*
* This section enables or disables entire modules in mbed TLS
* \{
*/

/**
 * \def MBEDTLS_ERROR_C
 *
 * Enable error code to error string conversion.
 *
 * Module:  library/error.c
 * Caller:
 *
 * This module enables mbedtls_strerror().
 */
#define MBEDTLS_ERROR_C

/**
* \def MBEDTLS_MD_C
*
* Enable the generic message digest layer.
*
* Module:  library/mbedtls_md.c
* Caller:
*
* Uncomment to enable generic message digest wrappers.
*/
#define MBEDTLS_MD_C

/**
* \def MBEDTLS_PLATFORM_C
*
* Enable the platform abstraction layer that allows you to re-assign
* functions like calloc(), free(), snprintf(), printf(), fprintf(), exit().
*
* Enabling MBEDTLS_PLATFORM_C enables to use of MBEDTLS_PLATFORM_XXX_ALT
* or MBEDTLS_PLATFORM_XXX_MACRO directives, allowing the functions mentioned
* above to be specified at runtime or compile time respectively.
*
* \note This abstraction layer must be enabled on Windows (including MSYS2)
* as other module rely on it for a fixed snprintf implementation.
*
* Module:  library/platform.c
* Caller:  Most other .c files
*
* This module enables abstraction of common (libc) functions.
*/
#define MBEDTLS_PLATFORM_C

/**
* \def MBEDTLS_SHA1_C
*
* Enable the SHA1 cryptographic hash algorithm.
*
* Module:  library/mbedtls_sha1.c
* Caller:  library/mbedtls_md.c
*          library/ssl_cli.c
*          library/ssl_srv.c
*          library/ssl_tls.c
*          library/x509write_crt.c
*
* This module is required for SSL/TLS and SHA1-signed certificates.
*/
#define MBEDTLS_SHA1_C

/* \} name SECTION: mbed TLS modules */

/**
* \name SECTION: Module configuration options
*
* This section allows for the setting of module specific sizes and
* configuration options. The default values are already present in the
* relevant header files and should suffice for the regular use cases.
*
* Our advice is to enable options and change their values here
* only if you have a good reason and know the consequences.
*
* Please check the respective header file for documentation on these
* parameters (to prevent duplicate documentation).
* \{
*/

/* Platform options */
//#define MBEDTLS_PLATFORM_STD_MEM_HDR   <stdlib.h> /**< Header to include if MBEDTLS_PLATFORM_NO_STD_FUNCTIONS is defined. Don't define if no header is needed. */
//#define MBEDTLS_PLATFORM_STD_CALLOC        calloc /**< Default allocator to use, can be undefined */
//#define MBEDTLS_PLATFORM_STD_FREE            free /**< Default free to use, can be undefined */
//#define MBEDTLS_PLATFORM_STD_EXIT            exit /**< Default exit to use, can be undefined */
//#define MBEDTLS_PLATFORM_STD_FPRINTF      fprintf /**< Default fprintf to use, can be undefined */
//#define MBEDTLS_PLATFORM_STD_PRINTF        printf /**< Default printf to use, can be undefined */
/* Note: your snprintf must correclty zero-terminate the buffer! */
//#define MBEDTLS_PLATFORM_STD_SNPRINTF    snprintf /**< Default snprintf to use, can be undefined */

/* To Use Function Macros MBEDTLS_PLATFORM_C must be enabled */
/* MBEDTLS_PLATFORM_XXX_MACRO and MBEDTLS_PLATFORM_XXX_ALT cannot both be defined */
//#define MBEDTLS_PLATFORM_CALLOC_MACRO        calloc /**< Default allocator macro to use, can be undefined */
//#define MBEDTLS_PLATFORM_FREE_MACRO            free /**< Default free macro to use, can be undefined */
//#define MBEDTLS_PLATFORM_EXIT_MACRO            exit /**< Default exit macro to use, can be undefined */
//#define MBEDTLS_PLATFORM_FPRINTF_MACRO      fprintf /**< Default fprintf macro to use, can be undefined */
//#define MBEDTLS_PLATFORM_PRINTF_MACRO        printf /**< Default printf macro to use, can be undefined */
/* Note: your snprintf must correclty zero-terminate the buffer! */
//#define MBEDTLS_PLATFORM_SNPRINTF_MACRO    snprintf /**< Default snprintf macro to use, can be undefined */

/* \} name SECTION: Module configuration options */

#include "check_config.h"

#endif /* MBEDTLS_CONFIG_H */
