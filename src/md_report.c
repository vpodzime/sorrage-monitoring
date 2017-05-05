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
#include <systemd/sd-journal.h>

/**
 * This program is supposed to be used as the 'PROGRAM' in
 * '/etc/mdadm.conf'. It is expected to be run on every event MD RAID
 * monitoring reports with the arguments described in man:mdadm(8) and it
 * reports every such event to the journal using structured logging.
 */

int main (int argc, char *argv[]) {
    char *event = NULL;
    char *md_dev = NULL;
    char *member = NULL;
    int ret = -1;

    if (argc < 3)
        error (1, 0, "Not enough arguments given!");

    event = argv[1];
    md_dev = argv[2];

    if (argc > 3)
        member = argv[3];

    ret = sd_journal_send ("MESSAGE_ID=3183267b90074a4595e91daef0e01462",
                           "MESSAGE=mdadm reported %s on the MD device %s", event, md_dev,
                           "SOURCE=MD RAID", "SOURCE_MAN=mdadm(8)",
                           "DEVICE=%s", md_dev, "STATE=%s", event,
                           "PRIORITY=%i", LOG_WARNING, "PRIORITY_DESC=warning",
                           NULL);

    if (ret != 0)
        error (2, -ret, "Failed to report the event using journal");

    return 0;
}
