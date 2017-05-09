/*
 * Copyright (C) 2017  Red Hat, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 * Author: Vratislav Podzimek <vpodzime@redhat.com>
 */

#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include <systemd/sd-journal.h>

/**
 * This program is a simple monitor of storage-related events reported to
 * journal using structured logging. It transforms all the reports into desktop
 * notifications.
 *
 * It is supposed to be run by a non-root user as part of the desktop session.
 */

void send_notification (sd_journal *j) {
    int ret = -1;
    size_t length = 0;
    const void *source = NULL;
    const void *device = NULL;
    const void *state = NULL;
    const void *details = NULL;
    char *cmd = NULL;

    ret = sd_journal_get_data (j, "SOURCE", &source, &length);
    if (ret < 0) {
        fprintf (stderr, "Failed to read SOURCE field: %s\n", strerror(-ret));
        return;
    } else
        /* get_data("KEY",...) returns a string prefixed with KEY= */
        source += 7;

    ret = sd_journal_get_data (j, "DEVICE", &device, &length);
    if (ret < 0) {
        fprintf (stderr, "Failed to read DEVICE field: %s\n", strerror(-ret));
        return;
    } else
        device += 7;

    ret = sd_journal_get_data (j, "STATE", &state, &length);
    if (ret < 0) {
        fprintf (stderr, "Failed to read STATE field: %s\n", strerror(-ret));
        return;
    } else
        state += 6;

    ret = sd_journal_get_data (j, "DETAILS", &details, &length);
    if (ret < 0) {
        fprintf (stderr, "Failed to read DETAILS field: %s\n", strerror(-ret));
        return;
    } else
        details += 8;

    ret = asprintf (&cmd, "notify-send 'Storage event reported by %s' '%s is %s: %s'",
                    source, device, state, details);
    if (ret > 0) {
        system (cmd);
        free (cmd);
    } else
        fprintf (stderr, "Failed allocate the notify-send command");
}

int main (int argc, char argv[]) {
    int ret = -1;
    sd_journal *j = NULL;
    time_t time_stamp = -1;

    /* we only want to report new stuff, let's get the timestamp of when we were
       started here */
    time_stamp = time (NULL);
    if (time_stamp < 0) {
        fprintf (stderr, "Failed to get system time: %s\n", strerror(errno));
        return 1;
    }

    ret = sd_journal_open (&j, 0);
    if (ret < 0) {
        fprintf (stderr, "Failed to open journal: %s\n", strerror(-ret));
        return 1;
    }

    /* and seek in the journal to that the time we were started at */
    ret = sd_journal_seek_realtime_usec (j, time_stamp * 1000000);
    if (ret < 0) {
        fprintf (stderr, "Failed to move to the tail of the journal: %s\n", strerror(-ret));
        return 1;
    }

    /* we only care about storage-related event reports */
    ret = sd_journal_add_match (j, "MESSAGE_ID=3183267b90074a4595e91daef0e01462", 0);
    if (ret < 0) {
        fprintf (stderr, "Failed to add match for MESSAGE_ID: %s\n", strerror(-ret));
        return 1;
    }

    while (ret >= 0) {
        ret = sd_journal_next (j);
        if (ret < 0) {
            fprintf (stderr, "Failed to iterate to next entry: %s\n", strerror(-ret));
        } else if (ret == 0) {
            /* reached the end, let's wait for changes, and try again */
            ret = sd_journal_wait (j, (uint64_t) -1);
            if (ret < 0) {
                fprintf (stderr, "Failed to wait for changes: %s\n", strerror(-ret));
            }
        } else {
            send_notification (j);
        }
    }
    sd_journal_close (j);
    return 0;
}
