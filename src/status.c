/*
 * Copyright 2022 Nico Sonack <nsonack@herrhotzenplotz.de>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following
 * disclaimer in the documentation and/or other materials provided
 * with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <ghcli/status.h>
#include <ghcli/forges.h>

void
ghcli_status(int count)
{
    ghcli_notification *notifications      = NULL;
    size_t              notifications_size = 0;

    notifications_size = ghcli_get_notifications(&notifications, count);

    if (count < 0) {
        ghcli_print_notifications(notifications, notifications_size);
    } else {
        ghcli_print_notifications(
            notifications,
            count < (int)notifications_size
            ? count : notifications_size);
    }

    ghcli_free_notifications(notifications, notifications_size);
}

size_t
ghcli_get_notifications(ghcli_notification **out, int count)
{
    return ghcli_forge()->get_notifications(out, count);
}

void
ghcli_free_notifications(
    ghcli_notification *notifications,
    size_t              notifications_size)
{
    for (size_t i = 0; i < notifications_size; ++i) {
        free(notifications[i].title);
        free(notifications[i].reason);
        free(notifications[i].date);
        free(notifications[i].type);
        free(notifications[i].repository);
    }

    free(notifications);
}

void
ghcli_print_notifications(
    ghcli_notification *notifications,
    size_t              notifications_size)
{
    for (size_t i = 0; i < notifications_size; ++i) {
        printf(
            "%s - %s - %s - %s\n",
            notifications[i].repository, notifications[i].type,
            notifications[i].date, notifications[i].reason);

        pretty_print(notifications[i].title, 4, 80, stdout);
        putchar('\n');
    }
}
