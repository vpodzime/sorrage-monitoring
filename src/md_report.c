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

#include <error.h>
#include <string.h>
#include <systemd/sd-journal.h>

/**
 * This program is supposed to be used as the 'PROGRAM' in
 * '/etc/mdadm.conf'. It is expected to be run on every event MD RAID
 * monitoring reports with the arguments described in man:mdadm(8) and it
 * reports every such event to the journal using structured logging.
 */

typedef struct _EventInfo {
    char *event;
    char *state;
    char *details;
    int  priority;
} EventInfo;

/* XXX: this has to have the same order as LOG_* levels defined in man:syslog(3) and syslog.h */
const char* log_lvl_desc[] = {"emergency", "alert", "critical", "error", "warning", "notice", "info", "debug"};

#define N_INFOS 9
EventInfo event_infos[N_INFOS] = {
    /* XXX: Should use syslog priority specified for events in man:mdadm(8)? All
            look more critical there */
    {"DeviceDisappeared", "deactivated", "MD array was deactivated", LOG_WARNING},
    {"RebuildStarted", "rebuilding", "MD array is rebuilding", LOG_INFO},
    {"RebuildFinished", "rebuilt", "MD array is now rebuilt", LOG_INFO},
    {"Fail", "failed", "Device was marked as failed", LOG_WARNING},
    {"FailSpare", "failed", "Device was marked as failed", LOG_WARNING},
    {"SpareActive", "activated", "Device was activated as part of an MD RAID", LOG_INFO},
    {"NewArray", "activated", "MD array was activated", LOG_INFO},
    {"DegradedArray", "degraded", "MD array is degraded", LOG_CRIT},
    {"SparesMissing", "missing spares", "MD array is missing spares", LOG_WARNING},
    // RebuildNN   -- rebuild progress reporting
    // MoveSpare   -- spare moved from one array to another one
    // TestMessage -- array found at startup and '--test' was given
};

#define md_fields(event, md_dev, member) "MD_EVENT=%s", event, \
                                         "MD_ARRAY=%s", md_dev, \
                                         "MD_MEMBER=%s", member

int main (int argc, char *argv[]) {
    char *event = NULL;
    char *md_dev = NULL;
    char *member = NULL;
    int i = -1;
    EventInfo *info = NULL;
    int priority = -1;
    int ret = -1;

    if (argc < 3)
        error (1, 0, "Not enough arguments given!");

    event = argv[1];
    md_dev = argv[2];

    if (argc > 3)
        member = argv[3];

    for (i=0; !info && i < N_INFOS; i++)
        if (strcmp (event, event_infos[i].event) == 0)
            info = event_infos + i;

    if (info) {
        if (!member) {
            ret = sd_journal_send ("MESSAGE_ID=3183267b90074a4595e91daef0e01462",
                                   "MESSAGE=mdadm reported %s on the MD device %s", event, md_dev,
                                   "SOURCE=MD RAID", "SOURCE_MAN=mdadm(8)",
                                   "DEVICE=%s", md_dev, "STATE=%s", info->state,
                                   "PRIORITY=%i", info->priority,
                                   "PRIORITY_DESC=%s", log_lvl_desc[info->priority],
                                   "DETAILS=%s", info->details,
                                   md_fields (event, md_dev, ""),
                                   NULL);
        } else {
            ret = sd_journal_send ("MESSAGE_ID=3183267b90074a4595e91daef0e01462",
                                   "MESSAGE=mdadm reported %s on the device %s", event, member,
                                   "SOURCE=MD RAID", "SOURCE_MAN=mdadm(8)",
                                   "DEVICE=%s", member, "STATE=%s", info->state,
                                   "PRIORITY=%i", info->priority, "PRIORITY_DESC=%s", log_lvl_desc[info->priority],
                                   "DETAILS=%s", info->details,
                                   md_fields (event, md_dev, member),
                                   NULL);
        }
    } else
        ret = sd_journal_send ("MESSAGE=unrecognized or ignored event %s reported by mdadm on device %s",
                               event, md_dev,
                               "PRIORITY=%i", LOG_INFO,
                               md_fields (event, md_dev, member ? member : ""),
                               NULL);

    if (ret != 0)
        error (2, -ret, "Failed to report the event using journal");

    return 0;
}
