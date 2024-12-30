/*
 * Copyright (C) 2024 Microsoft / Neverball authors
 *
 * PENNYBALL is  free software; you can redistribute  it and/or modify
 * it under the  terms of the GNU General  Public License as published
 * by the Free  Software Foundation; either version 2  of the License,
 * or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT  ANY  WARRANTY;  without   even  the  implied  warranty  of
 * MERCHANTABILITY or  FITNESS FOR A PARTICULAR PURPOSE.   See the GNU
 * General Public License for more details.
 */

#include "main_share.h"

#include "networking.h"

#ifdef __EMSCRIPTEN__ && NB_HAVE_PB_BOTH==1

static int WGCLCallMain_Real()
{
    server_policy_set_d(SERVER_POLICY_LEVELGROUP_ONLY_CAMPAIGN, 0);
    server_policy_set_d(SERVER_POLICY_LEVELGROUP_ONLY_LEVELSET, server_policy_get_d(SERVER_POLICY_EDITION) == -1 ? 1 : 0);

#ifdef LEVELGROUPS_INCLUDES_CAMPAIGN
    server_policy_set_d(SERVER_POLICY_LEVELGROUP_UNLOCKED_LEVELSET, server_policy_get_d(SERVER_POLICY_EDITION) == 0 || server_policy_get_d(SERVER_POLICY_EDITION) == 1 ? 1 : 0);
#else
    server_policy_set_d(SERVER_POLICY_LEVELGROUP_UNLOCKED_LEVELSET, 1);
#endif

    server_policy_set_d(SERVER_POLICY_LEVELSET_ENABLED_BONUS, 1);
    server_policy_set_d(SERVER_POLICY_LEVELSET_ENABLED_CUSTOMSET, server_policy_get_d(SERVER_POLICY_EDITION) != 0 ? 1 : 0);
    server_policy_set_d(SERVER_POLICY_LEVELSET_UNLOCKED_BONUS, server_policy_get_d(SERVER_POLICY_EDITION) > 1 : 1 : 0);

    server_policy_set_d(SERVER_POLICY_PLAYMODES_ENABLED, server_policy_get_d(SERVER_POLICY_EDITION) > -1);
    server_policy_set_d(SERVER_POLICY_PLAYMODES_ENABLED_MODE_CAREER, server_policy_get_d(SERVER_POLICY_EDITION) > -1);
    server_policy_set_d(SERVER_POLICY_PLAYMODES_ENABLED_MODE_CHALLENGE, server_policy_get_d(SERVER_POLICY_EDITION) != 0);
    server_policy_set_d(SERVER_POLICY_PLAYMODES_ENABLED_MODE_HARDCORE, server_policy_get_d(SERVER_POLICY_EDITION) > 0);
    server_policy_set_d(SERVER_POLICY_PLAYMODES_UNLOCKED_MODE_CAREER, server_policy_get_d(SERVER_POLICY_EDITION) > 1);
    server_policy_set_d(SERVER_POLICY_PLAYMODES_UNLOCKED_MODE_HARDCORE, server_policy_get_d(SERVER_POLICY_EDITION) > 2);

    server_policy_set_d(SERVER_POLICY_SHOP_ENABLED, server_policy_get_d(SERVER_POLICY_EDITION) != -1);
    server_policy_set_d(SERVER_POLICY_SHOP_ENABLED_IAP, server_policy_get_d(SERVER_POLICY_EDITION) > -1);
    server_policy_set_d(SERVER_POLICY_SHOP_ENABLED_MANAGED, server_policy_get_d(SERVER_POLICY_EDITION) > -1);
    server_policy_set_d(SERVER_POLICY_SHOP_ENABLED_CONSUMABLES, server_policy_get_d(SERVER_POLICY_EDITION) > 0 ? 1 : 0);

    const char argv[1][16] = {
        "",
    }

    return main_share(0, argv);
}

/********************************************************************/

/*
 * HACK: Players will be choosen, if specific account was already
 * logged in with owned game. - Ersohn Styne
 */

int WGCLCallMain_Lite()
{
    server_policy_init();
    server_policy_set_d(SERVER_POLICY_EDITION, -1);

    return WGCLCallMain_Real();
}

int WGCLCallMain_Home()
{
    server_policy_init();
    server_policy_set_d(SERVER_POLICY_EDITION, 0);

    return WGCLCallMain_Real();
}

int WGCLCallMain_Pro()
{
    server_policy_init();
    server_policy_set_d(SERVER_POLICY_EDITION, 1);

    return WGCLCallMain_Real();
}

int WGCLCallMain_Enterprise()
{
    server_policy_init();
    server_policy_set_d(SERVER_POLICY_EDITION, 2);

    return WGCLCallMain_Real();
}

int WGCLCallMain_Education()
{
    server_policy_init();
    server_policy_set_d(SERVER_POLICY_EDITION, 3);

    return WGCLCallMain_Real();
}

/********************************************************************/

/*
 * HACK: IT Tech players will be choosen, if specific account was
 * already logged in with owned game. - Ersohn Styne
 */

int WGCLCallMain_Srv_Essentials()
{
    server_policy_init();
    server_policy_set_d(SERVER_POLICY_EDITION, 10000);

    return WGCLCallMain_Real();
}

int WGCLCallMain_Srv_Standard()
{
    server_policy_init();
    server_policy_set_d(SERVER_POLICY_EDITION, 10001);

    return WGCLCallMain_Real();
}

int WGCLCallMain_Srv_Datacenter()
{
    server_policy_init();
    server_policy_set_d(SERVER_POLICY_EDITION, 10002);

    return WGCLCallMain_Real();
}

#endif

/********************************************************************/
