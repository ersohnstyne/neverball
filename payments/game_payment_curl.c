/*
 * Copyright (C) 2024 Microsoft / Neverball authors
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

/*
 * HACK: Remembering the code file differences:
 * Developers  who  programming  C++  can see more bedrock declaration
 * than C.  Developers  who  programming  C  can  see  few  procedural
 * declaration than  C++.  Keep  in  mind  when making  sure that your
 * extern code must associated. The valid file types are *.c and *.cpp,
 * so it's always best when making cross C++ compiler to keep both.
 * - Ersohn Styne
 */

#if __cplusplus
extern "C"
{
#endif
#include "game_payment.h"
#include "common.h"
#include "fetch.h"
#include "fs.h"
#include "log.h"
#if __cplusplus
}
#endif

#include <stdbool.h>

#if ENABLE_IAP
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

#define GAMEIAP_CURRDOMAIN "neverball.github.io"

// Read-only mode for debug builds
#define IAP_READ_ONLY_MODE

/*---------------------------------------------------------------------------*/

#if __cplusplus
namespace WorldPayment
{
    class OrderRetrieval
    {
    public:
        OrderRetrieval() {};
        bool Initialize();
#if ENABLE_IAP
        int ActivateOrderPack(const char *ProductKey);
#endif
        void Shutdown();

        OrderPack GetOrderPack() { return m_order_pack; }

    private:
#if ENABLE_IAP
        CURL *m_payment_handle;
#endif
        OrderPack m_order_pack;
    };
}
#else
#if ENABLE_IAP
static CURL *m_payment_handle;
#endif
static OrderPack m_order_pack;

OrderPack WorldPayment__OrderRetrieval__GetOrderPack() { return m_order_pack; }
#endif

/*---------------------------------------------------------------------------*/

#if __cplusplus
bool WorldPayment::OrderRetrieval::Initialize()
#else
int WorldPayment__OrderRetrieval__Initialize()
#endif
{
#if ENABLE_IAP
#if ENABLE_FETCH
    fetch_quit();
#endif

    unsigned long long init_flags = CURL_GLOBAL_NOTHING;

#if _WIN32
    /* In Windows 11 and later OS, we'll use Win32. */

    init_flags |= CURL_GLOBAL_WIN32;
#endif

    curl_global_init(init_flags);

    m_payment_handle = curl_easy_init();
    return m_payment_handle ? true : false;
#else
    return false;
#endif
}

struct payment_recvdata
{
    int prepared;
    char str[512];
};

static struct payment_recvdata curr_payment_recvdata;

static size_t WriteFromWeb_Bedrock(
    void *Buffer,
    UINT64 Size,
    UINT64 ByteMem,
    void *Parameters
)
{
    struct payment_recvdata *in_data = (struct payment_recvdata *) Parameters;

    if (in_data)
    {
        if (!in_data->prepared)
        {
            memset(in_data->str, 0, 256);
            in_data->prepared = 1;
        }

        if (in_data->prepared)
        {
            SAFECAT(in_data->str, (char *) Buffer);
            return Size * ByteMem;
        }
    }

    return 0;
}

#if ENABLE_IAP
#if __cplusplus
int WorldPayment::OrderRetrieval::ActivateOrderPack(const char *ProductKey)
#else
int WorldPayment__OrderRetrieval__ActivateOrderPack(const char *ProductKey)
#endif
{
    int world_result = 1;
    char targetAdrrText[MAXSTR];
    memset(&m_order_pack, 0, sizeof(OrderPack));

#if _WIN32 && !defined(__EMSCRIPTEN__) && !_CRT_SECURE_NO_WARNINGS
    sprintf_s(targetAdrrText, dstSize,
#else
    sprintf(targetAdrrText,
#endif
#ifdef IAP_READ_ONLY_MODE
            "http://" GAMEIAP_CURRDOMAIN "/iap/showorder-key/%s", ProductKey);
#else
            "http://" GAMEIAP_CURRDOMAIN "/iap/retrieve-order/%s", ProductKey);
    curl_easy_setopt(m_payment_handle, CURLOPT_POST, 1);
#endif

    curl_easy_setopt(m_payment_handle, CURLOPT_URL, targetAdrrText);
    curl_easy_setopt(m_payment_handle, CURLOPT_WRITEFUNCTION, WriteFromWeb_Bedrock);
    curl_easy_setopt(m_payment_handle, CURLOPT_WRITEDATA, &curr_payment_recvdata);
    curl_easy_setopt(m_payment_handle, CURLOPT_NOPROGRESS, 1L);

    CURLcode result = curl_easy_perform(m_payment_handle);

    if (result == CURLE_OK)
    {
        char str_status[13]; SAFECPY(str_status, curr_payment_recvdata.str + 1);

        if (strncmp(str_status, "\"error\":\"ok\"", 13) == 0)
        {
            log_printf("Order code retrieved!: %s\n", curr_payment_recvdata.str);

            int orderpack_id, orderpack_edition;

            char* outStr = strdup(curr_payment_recvdata.str + 22);

            sscanf(outStr, "\"IAPOrderID\":%d,\"EditionID\":%d,\"Coins\":%d,\"Gems\":%d,\"Balls\":%d,\"Earninator\":%d,\"Floatifier\":%d,\"Speedifier\":%d",
                   &orderpack_id,
                   &orderpack_edition,
                   &m_order_pack.Wallets[0],
                   &m_order_pack.Wallets[1],
                   &m_order_pack.Balls,
                   &m_order_pack.Powerups[0],
                   &m_order_pack.Powerups[1],
                   &m_order_pack.Powerups[2]);

            world_result = 0;

            free(outStr);
        }
    }
    else
        world_result = result;

    return result == CURLE_OK ? world_result : -world_result;
}
#endif

#if __cplusplus
void WorldPayment::OrderRetrieval::Shutdown()
#else
void WorldPayment__OrderRetrieval__Shutdown()
#endif
{
#if ENABLE_IAP
    curl_easy_cleanup(m_payment_handle);
    curl_global_cleanup();
#endif
}

/*---------------------------------------------------------------------------*/

#if __cplusplus
static WorldPayment::OrderRetrieval order_retrieval;
#endif

#if __cplusplus
extern "C"
{
#endif

int game_payment_activate(const char *product_key)
{
#if ENABLE_IAP
    int result = 0;
#if __cplusplus
    if (order_retrieval.Initialize())
        result = order_retrieval.ActivateOrderPack(product_key);
#else
    if (WorldPayment__OrderRetrieval__Initialize())
        result = WorldPayment__OrderRetrieval__ActivateOrderPack(product_key);
#endif
    else result = -1;

#if __cplusplus
    order_retrieval.Shutdown();
#else
    WorldPayment__OrderRetrieval__Shutdown();
#endif
#if ENABLE_FETCH
    fetch_reinit();
#endif

    return result;
#else
#if ENABLE_FETCH
    fetch_reinit();
#endif

    return -9999;
#endif
}

void game_payment_browse(int index)
{
    char linkstr[MAXSTR];

#if _WIN32 && !defined(__EMSCRIPTEN__) && !_CRT_SECURE_NO_WARNINGS
    sprintf_s(linkstr, MAXSTR
#else
    sprintf(linkstr,
#endif
#if _WIN32
            "start msedge http://%s/iap/overview/%d",
#elif defined(__APPLE__)
            "open http://%s/iap/overview/%d",
#elif defined(__linux__)
            "x-www-browser http://%s/iap/overview/%d",
#endif
            GAMEIAP_CURRDOMAIN, index + 1);

    system(linkstr);
}

struct game_orderpack curr_orderpack(void)
{
#if __cplusplus
    return order_retrieval.GetOrderPack();
#else
    return WorldPayment__OrderRetrieval__GetOrderPack();
#endif
}

#if __cplusplus
}
#endif