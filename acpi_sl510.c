/*-
 *   Copyright (c) 2015 by Tobias Kortkamp
 *   tobias.kortkamp@gmail.com
 *
 *   Based on acpi_call:
 *   Copyright (C) 2011 by Maxim Ignatenko
 *   gelraen.ua@gmail.com
 *
 *   All rights reserved.
 *
 *   Redistribution and use in source and binary forms, with or without
 *    modification, are permitted provided that the following conditions
 *    are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in
 *       the documentation and/or other materials provided with the
 *       distribution.
 *
 *   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *   "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 *   A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 *   OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 *   SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 *   LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 *   DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 *   THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *   (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 *   OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#include <sys/param.h>
#include <sys/module.h>
#include <sys/kernel.h>
#include <sys/conf.h>
#include <sys/sysctl.h>
#include <sys/errno.h>
#include <sys/eventhandler.h>
#include <contrib/dev/acpica/include/acpi.h>

static struct sysctl_ctx_list clist;
static struct sysctl_oid *poid;

static int current_brightness_level = 8;

static int
acpi_sl510_set_brightness(int level)
{
    ACPI_OBJECT args[1];
    ACPI_OBJECT_LIST objlist = { .Count = 1, .Pointer = args };
    args[0].Type = ACPI_TYPE_INTEGER;
    args[0].Integer.Value = level;

    ACPI_BUFFER result;
    result.Length = ACPI_ALLOCATE_BUFFER;
    result.Pointer = NULL;

    ACPI_STATUS retval = AcpiEvaluateObject(NULL, "\\VBRC", &objlist, &result);
    if (ACPI_SUCCESS(retval)) {
        if (result.Pointer != NULL) {
            AcpiOsFree(result.Pointer);
        }
        return 1;
    } else {
        return 0;
    }
}

static int
acpi_sl510_sysctl_set_brightness(SYSCTL_HANDLER_ARGS)
{
    int error = 0;
    int level = current_brightness_level;

    error = sysctl_handle_int(oidp, &level, 0, req);
    if (error || !req->newptr) {
        return (error);
    }

    if (level >= 0 && level <= 15 && acpi_sl510_set_brightness(level)) {
        current_brightness_level = level;
    } else {
        error = EINVAL;
    }

    return (error);
}

// TODO: Auto-restore brightness level after resume

static int
acpi_sl510_modevent(module_t mod __unused, int event, void *arg __unused)
{
    int error = 0;

    switch (event) {
    case MOD_LOAD:
        sysctl_ctx_init(&clist);
        poid = SYSCTL_ADD_NODE(&clist,
                               SYSCTL_STATIC_CHILDREN(_dev),
                               OID_AUTO,
                               "acpi_sl510",
                               CTLFLAG_RW,
                               0,
                               "LCD brightness control for Thinkpad SL510");
        SYSCTL_ADD_PROC(&clist,
                        SYSCTL_CHILDREN(poid),
                        OID_AUTO,
                        "lcd_brightness",
                        CTLTYPE_INT | CTLFLAG_RW,
                        &current_brightness_level,
                        0,
                        acpi_sl510_sysctl_set_brightness,
                        "I",
                        "LCD brightness (valid range: 0 to 15)");

        // Set default level on load
        acpi_sl510_set_brightness(current_brightness_level);
        break;
    case MOD_UNLOAD:
        sysctl_ctx_free(&clist);
        break;
    default:
        error = EOPNOTSUPP;
        break;
    }

    return (error);
}

DEV_MODULE(acpi_sl510, acpi_sl510_modevent, NULL);
