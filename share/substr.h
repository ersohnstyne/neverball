#ifndef SUBSTR_H
#define SUBSTR_H 1

#include <string.h>
#include "common.h"

struct strbuf
{
    char *buf;
};

static struct strbuf substr(const char *str, size_t start, size_t count)
{
    struct strbuf sb = { NULL };

    if (str)
    {
        const size_t len = strlen(str);

        start = MIN(start, len);
        count = MIN(len - start, count);

        sb.buf = malloc(len + 1);

        if (sb.buf)
        {
            count = MIN(sizeof (sb.buf), count);

            if (count > 0 && count < len + 1)
            {
                memcpy(sb.buf, str + start, count);
                sb.buf[count] = 0;
            }
        }
    }

    return sb;
}

/*
 * Allocate a fixed-size buffer on the stack and fill it with the given substring.
 */
#define SUBSTR(str, start, count) (substr((str), (start), (count)).buf)

#endif