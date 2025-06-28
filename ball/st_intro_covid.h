/*
 * Copyright (C) 2025 Microsoft / Neverball authors / Jānis Rūcis
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
 * Some filter limits restrict replays with exceeded level status.
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

#define DEMO_LOCKDOWN_RANGE_NIGHT_TIMELEFT(output, from, to, out_sec) \
    do if (from >= to) {                                              \
        time_t _lockdown_date = time(NULL);                           \
        int _clockhour = localtime(&_lockdown_date)->tm_hour;         \
        int _clockmin = localtime(&_lockdown_date)->tm_min;           \
        int _clocksec = localtime(&_lockdown_date)->tm_sec;           \
        out_sec = (from * 3600)                                       \
            - ((_clockhour * 3600) + (_clockmin * 60) + (_clocksec)); \
        if (out_sec < 0) out_sec += 86400;                            \
        if (out_sec >= 86400) out_sec -= 86400;                       \
        output = _clockhour >= to && _clockhour < from;               \
    } while (0)

#define DEMO_LOCKDOWN_RANGE_DAY_TIMELEFT(output, from, to, out_sec)   \
    do if (from <= to) {                                              \
        time_t _lockdown_date = time(NULL);                           \
        int _clockhour = localtime(&_lockdown_date)->tm_hour;         \
        int _clockmin = localtime(&_lockdown_date)->tm_min;           \
        int _clocksec = localtime(&_lockdown_date)->tm_sec;           \
        out_sec = (from * 3600)                                       \
            - ((_clockhour * 3600) + (_clockmin * 60) + (_clocksec)); \
        if (out_sec < 0) out_sec += 86400;                            \
        if (out_sec >= 86400) out_sec -= 86400;                       \
        output = _clockhour > from && _clockhour <= to;               \
    } while (0)

#define DEMO_LOCKDOWN_RANGE_NIGHT(output, from, to)           \
    do if (from >= to) {                                      \
        time_t _lockdown_date = time(NULL);                   \
        int _clockhour = localtime(&_lockdown_date)->tm_hour; \
        output = _clockhour >= to && _clockhour < from;       \
    } while (0)

#define DEMO_LOCKDOWN_RANGE_DAY(output, from, to)             \
    do if (from <= to) {                                      \
        time_t _lockdown_date = time(NULL);                   \
        int _clockhour = localtime(&_lockdown_date)->tm_hour; \
        output = _clockhour > from && _clockhour <= to;       \
    } while (0)
#endif

#endif
