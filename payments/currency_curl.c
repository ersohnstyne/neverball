/*
 * Copyright (C) 2026 Microsoft / Neverball authors / Ersohn Styne
 *
 * NEVERBALL is  free software; you can redistribute  it and/or modify
 * it under the  terms of the GNU General  Public License as published
 * by the Free  Software Foundation; either version 2  of the License,
 * or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT  ANY  WARRANTY;  without   even  the  implied  warranty  of
 * MERCHANTABILITY or  FITNESS FOR A PARTICULAR PURPOSE.   See the GNU
 * General Public License for more details.
 */

#include "currency.h"

#include "common.h"
#include "lang.h"

#include "vec3.h"

#if _DEBUG && _MSC_VER
#ifndef _CRTDBG_MAP_ALLOC
#pragma message(__FILE__": Missing _CRT_MAP_ALLOC, recreate: _CRTDBG_MAP_ALLOC + crtdbg.h")
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif
#endif

#if ENABLE_FETCH!=0
#include <curl/curl.h>

#if _WIN32 && _MSC_VER
#pragma comment(lib, "Normaliz.lib")
#pragma comment(lib, "WS2_32.lib")
#pragma comment(lib, "Wldap32.lib")
#pragma comment(lib, "crypt32.lib")
#pragma comment(lib, "advapi32.lib")
#endif

#if _DEBUG
#pragma comment(lib, "libcurl_a_debug.lib")
#else
#pragma comment(lib, "libcurl_a.lib")
#endif
#endif

/*
 * Premium: cxball.stynegame.de
 * Legacy IAP: currency.neverball.org
 */
#define GAMEIAP_CURRDOMAIN "cxball.stynegame.de"

#define CURRENT_EUROFXREF_FILEPATH "eurofxref.xml"

/*---------------------------------------------------------------------------*/

#if !defined(__NDS__) && !defined(__3DS__) && \
    !defined(__GAMECUBE__) && !defined(__WII__) && !defined(__WIIU__) && \
    !defined(__SWITCH__)

/*
 * Currencies are mostly sightly different, when you using different
 * countries in a single game applications.
 *
 * When adding new currencies, ensure that conforms are following:
 * - multiply of zeroed from subunit
 * - 2 code country
 * - 3 code currency
 * - Alternative currency (optional)
 */
struct currency_region
{
    const int   subunit;       /* multiply of zeroed from subunit (in cents) */
    const char *country;       /* 2 code country                             */
    const char *code_real;     /* 3 code currency                            */
    const char *icon;          /* Alternative currency (optional)            */
} currency_regions[] = {
    { 100,   "ar",    "AED", /*CURRENCY_FINANCE_UAE*/ },
    { 100,   "au",    "AUD", CURRENCY_FINANCE_AUD },
    { 100,   "br",    "BRL", CURRENCY_FINANCE_BR  },
    { 100,   "ch",    "CHF", CURRENCY_FINANCE_CH  },
    { 100,   "en_gb", "GBP", CURRENCY_FINANCE_GB  },
    { 100,   "en_us", "USD", CURRENCY_FINANCE_US  },
    { 100,   "hu",    "HUF", CURRENCY_FINANCE_HU  },
    { 100,   "zh_CN", "HKD", CURRENCY_FINANCE_HKD },
    { 100,   "zh_TW", "TWD", CURRENCY_FINANCE_TWD },
    { 100,   "id",    "IDR", CURRENCY_FINANCE_ID  },
    { 100,   "in",    "INR", /*CURRENCY_FINANCE_IN*/  },
    { 100,   "ja",    "JPY", CURRENCY_FINANCE_JA },
    { 1000,  "ko",    "KRW", /*CURRENCY_FINANCE_KR*/  },
    { 100,   "no",    "NOK", CURRENCY_FINANCE_NO  },
    { 100,   "nz",    "NZD", CURRENCY_FINANCE_NZD },
    { 100,   "pl",    "PLN", CURRENCY_FINANCE_PL  },
    { 100,   "en_sg", "SGD", CURRENCY_FINANCE_SG  },
    { 100,   "th",    "THB", /*CURRENCY_FINANCE_TH*/  },
};

struct currency_recvdata
{
    int prepared;
    fs_file handle;
};

static int currency_ready = 0;

static size_t WriteFromMem(void   *Buffer,
                           size_t  Size,
                           size_t  ByteMem,
                           void   *Parameters)
{
    struct currency_recvdata *in_data = (struct currency_recvdata *) Parameters;

    if (in_data)
    {
        if (!in_data->prepared)
        {
            const char *target_path = CURRENT_EUROFXREF_FILEPATH;
            if ((in_data->handle = fs_open_write(target_path)))
                in_data->prepared = 1;
        }

        if (in_data->prepared)
            return fs_write(Buffer, Size * ByteMem, in_data->handle);
    }

    return 0;
}

static struct currency_recvdata recv_in_data;

#endif

void currency_init(void)
{
#if !defined(__NDS__) && !defined(__3DS__) && \
    !defined(__GAMECUBE__) && !defined(__WII__) && !defined(__WIIU__) && \
    !defined(__SWITCH__)
#if ENABLE_FETCH!=0
    if (currency_ready) return;

    CURL *currency_handle;
    const char *url =
        "https://raw.githubusercontent.com/pixyzehn/getexpenses.app/main/eurofxref/eurofxref.xml";

    unsigned long long init_flags = CURL_GLOBAL_NOTHING;

    /* Check if URL contains HTTPS. If so, use SSL init flags. */

    if (str_starts_with(url, "https:"))
        init_flags |= CURL_GLOBAL_SSL;

#if _WIN32
    /* In Windows 11 and later OS, we'll use Win32. */

    init_flags |= CURL_GLOBAL_WIN32;
#endif

#if ENABLE_FETCH!=1
    /* Used with ENABLE_FETCH=1 */

    curl_global_init(init_flags);
#endif

    if ((currency_handle = curl_easy_init()))
    {
        recv_in_data.prepared = 0;

        curl_easy_setopt(currency_handle, CURLOPT_URL,           url);
        curl_easy_setopt(currency_handle, CURLOPT_BUFFERSIZE,    102400L);
        curl_easy_setopt(currency_handle, CURLOPT_NOPROGRESS,    1L);
        curl_easy_setopt(currency_handle, CURLOPT_WRITEFUNCTION, WriteFromMem);
        curl_easy_setopt(currency_handle, CURLOPT_WRITEDATA,     &recv_in_data);

#if _WIN32 && defined(CURLSSLOPT_NATIVE_CA)
        if (str_starts_with(url, "https:"))
            curl_easy_setopt(currency_handle, CURLOPT_SSL_OPTIONS, CURLSSLOPT_NATIVE_CA);
#endif

        CURLcode ec = curl_easy_perform(currency_handle);

        if (ec == CURLE_OK)
            currency_ready = 1;

        fs_close(recv_in_data.handle);

        curl_easy_cleanup(currency_handle);
    }

#if ENABLE_FETCH!=1
    /* Used with ENABLE_FETCH=1 */

    curl_global_cleanup();
#endif
#endif
#endif
}

void currency_quit(void)
{
    /* HACK: Only temporary download stuff. */
}

int currency_get_code_from_locale(const char  *locale,
                                  char       **currency_code,
                                  char       **currency_icon,
                                  int         *subunit)
{
#if !defined(__NDS__) && !defined(__3DS__) && \
    !defined(__GAMECUBE__) && !defined(__WII__) && !defined(__WIIU__) && \
    !defined(__SWITCH__)
#if ENABLE_FETCH!=0
    if (!currency_ready) return 0;
#endif

    for (int i = 0; i < ARRAYSIZE(currency_regions); i++)
    {
        if (strcmp(locale, currency_regions[i].country) == 0)
        {
            if (currency_icon && *currency_icon)
            {
                free(*currency_icon);
                *currency_icon = NULL;
            }

            if (currency_code && *currency_code)
            {
                free(*currency_code);
                *currency_code = NULL;
            }

            *currency_icon = strdup(currency_regions[i].icon ?
                                    currency_regions[i].icon : "");
            *currency_code = strdup(currency_regions[i].code_real);
            if (subunit) *subunit = currency_regions[i].subunit;
            return 1;
        }
    }

    *subunit = 100;
    return 0;
#else
    return 0;
#endif
}

const char *currency_get_price_from_locale(const char *locale, float price)
{
#if !defined(__NDS__) && !defined(__3DS__) && \
    !defined(__GAMECUBE__) && !defined(__WII__) && !defined(__WIIU__) && \
    !defined(__SWITCH__)
#if ENABLE_FETCH!=0
    if (!currency_ready) return "-,--";
#endif

    char temp_pricing[MAXSTR];
    char *currency_code, *currency_icon;
    int temp_whole   = 0, temp_cents = 0,
        temp_whole_k = 0, subunit;

    int have_currency_locale = currency_get_code_from_locale(locale,
                                                             &currency_code,
                                                             &currency_icon,
                                                             &subunit);

    if (have_currency_locale)
    {
        fs_file currency_read_handle;
        float   origprice      = price;
        float   modprice       = origprice;
        int     currency_found = 0;

        if ((currency_read_handle = fs_open_read(CURRENT_EUROFXREF_FILEPATH)))
        {
            char  line[MAXSTR];
            char  in_currency_code[3] = { 0, 0, 0 };
            float in_rate;

            while (fs_gets(line, sizeof (line), currency_read_handle) &&
                   !currency_found)
            {
                if (str_starts_with(line, "<Cube currency=") && !currency_found)
                {
#if _WIN32 && !defined(__EMSCRIPTEN__) && !_CRT_SECURE_NO_WARNINGS
                    int scanf_res = sscanf_s(line,
#else
                    int scanf_res = sscanf(line,
#endif
                                           "<Cube currency=\"%c%c%c\" rate=\"%f\"",
                                           &in_currency_code[0],
                                           &in_currency_code[1],
                                           &in_currency_code[2],
                                           &in_rate);

                    if (strncmp(currency_code, in_currency_code, 3) == 0)
                    {
                        modprice = (float) (origprice * in_rate);
                        currency_found = 1;
                    }
                }
            }

            fs_close(currency_read_handle);
        }

        temp_cents   =  (ROUND(modprice * subunit)) % subunit;
        temp_whole   =  (ROUND(modprice * subunit)) / subunit;
        temp_whole_k = ((ROUND(modprice * subunit)) / subunit) / 1000;

        /* Should be more than Rp. 1000, then display value is Rp. 1k. */

        if (temp_whole >= 1000)
            sprintf(temp_pricing, "%dk %s",
                    temp_whole_k,
                    currency_found ?
                    currency_icon[0] ?
                    currency_icon :
                    currency_code :
                    CURRENCY_FINANCE_EU);
        else
            sprintf(temp_pricing, "%d,%02d %s",
                    temp_whole, temp_cents,
                    currency_found ?
                    currency_icon[0] ?
                    currency_icon :
                    currency_code :
                    CURRENCY_FINANCE_EU);
    }
    else
    {
        temp_cents = (ROUND(price * subunit)) % subunit;
        temp_whole = (ROUND(price * subunit)) / subunit;

        sprintf(temp_pricing, "%d,%02d %s",
                temp_whole, temp_cents, CURRENCY_FINANCE_EU);
    }

    return temp_pricing;
#else
    return "-,--";
#endif
}
