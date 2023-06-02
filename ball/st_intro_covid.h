/*
 * Copyright (C) 2023 Microsoft / Neverball authors
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

#ifndef ST_INTRO_COVID_H
#define ST_INTRO_COVID_H

/* RKI detects some covid high risk! */
//#define COVID_HIGH_RISK

/*
 * Some exceeded level status limits restrict replays.
 */
#define DEMO_QUARANTINED_MODE

/*
 * This replays locks down completly permanent,
 * when using COVID 19 support.
 */
//#define DEMO_LOCKDOWN_COMPLETE

#ifdef DEMO_QUARANTINED_MODE

#define DEMO_LOCKDOWN_RANGE_NIGHT_DEFAULT(nolockdown) \
    (nolockdown, 16, 8)

#define DEMO_LOCKDOWN_RANGE_NIGHT_START_HOUR_DEFAULT 16
#define DEMO_LOCKDOWN_RANGE_NIGHT_END_HOUR_DEFAULT 8

#include <assert.h>

#define DEMO_LOCKDOWN_RANGE_NIGHT_TIMELEFT(nolockdown, from, to, out_sec) \
    do { assert(from >= to);                                              \
        time_t lockdown_date = time(NULL);                                \
        int clockhour = localtime(&lockdown_date)->tm_hour;               \
        int clockmin = localtime(&lockdown_date)->tm_min;                 \
        int clocksec = localtime(&lockdown_date)->tm_sec;                 \
        out_sec = (from * 3600)                                           \
            - ((clockhour * 3600) + (clockmin * 60) + (clocksec));        \
        if (out_sec < 0) out_sec += 86400;                                \
        if (out_sec >= 86400) out_sec -= 86400;                           \
        nolockdown = clockhour >= to && clockhour < from;                 \
    } while (0)

#define DEMO_LOCKDOWN_RANGE_DAY_TIMELEFT(nolockdown, from, to, out_sec) \
    do { assert(from <= to);                                            \
        time_t lockdown_date = time(NULL);                              \
        int clockhour = localtime(&lockdown_date)->tm_hour;             \
        int clockmin = localtime(&lockdown_date)->tm_min;               \
        int clocksec = localtime(&lockdown_date)->tm_sec;               \
        out_sec = (from * 3600)                                         \
            - ((clockhour * 3600) + (clockmin * 60) + (clocksec));      \
        if (out_sec < 0) out_sec += 86400;                              \
        if (out_sec >= 86400) out_sec -= 86400;                         \
        nolockdown = clockhour > from && clockhour <= to;               \
    } while (0)

#define DEMO_LOCKDOWN_RANGE_NIGHT(nolockdown, from, to)     \
    do { assert(from >= to);                                \
        time_t lockdown_date = time(NULL);                  \
        int clockhour = localtime(&lockdown_date)->tm_hour; \
        nolockdown = clockhour >= to && clockhour < from;   \
    } while (0)

#define DEMO_LOCKDOWN_RANGE_DAY(nolockdown, from, to)       \
    do { assert(from <= to);                                \
        time_t lockdown_date = time(NULL);                  \
        int clockhour = localtime(&lockdown_date)->tm_hour; \
        nolockdown = clockhour > from && clockhour <= to;   \
    } while (0)
#endif

#endif
