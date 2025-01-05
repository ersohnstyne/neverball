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

#include "main_share.h"

#include "networking.h"

#ifdef __EMSCRIPTEN__ && NB_HAVE_PB_BOTH==1

static void WGCLCallMain_Real(void)
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
    server_policy_set_d(SERVER_POLICY_LEVELSET_UNLOCKED_BONUS, server_policy_get_d(SERVER_POLICY_EDITION) > 1 ? 1 : 0);

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
}

/********************************************************************/

/*
 * HACK: Players will be choosen, if specific account was already
 * logged in with owned game. - Ersohn Styne
 */

void WGCLCallMain(int edition_id)
{
    server_policy_init();
    server_policy_set_d(SERVER_POLICY_EDITION, edition_id);
}

#endif

/********************************************************************/
