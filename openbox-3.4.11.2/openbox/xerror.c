/* -*- indent-tabs-mode: nil; tab-width: 4; c-basic-offset: 4; -*-

   xerror.c for the Openbox window manager
   Copyright (c) 2006        Mikael Magnusson
   Copyright (c) 2003-2007   Dana Jansens

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   See the COPYING file for a copy of the GNU General Public License.
*/

#include "openbox.h"
#include "gettext.h"
#include "debug.h"
#include "xerror.h"
#include <glib.h>
#include <X11/Xlib.h>

static gboolean xerror_ignore = FALSE;
gboolean xerror_occured = FALSE;

gint xerror_handler(Display *d, XErrorEvent *e)
{
#ifdef DEBUG
    gchar errtxt[128];

    XGetErrorText(d, e->error_code, errtxt, 127);
    if (!xerror_ignore) {
        if (e->error_code == BadWindow)
            /*g_message(_("X Error: %s\n"), errtxt)*/;
        else
            g_error(_("X Error: %s"), errtxt);
    } else
        ob_debug("XError code %d '%s'\n", e->error_code, errtxt);
#else
    (void)d; (void)e;
#endif

    xerror_occured = TRUE;
    return 0;
}

void xerror_set_ignore(gboolean ignore)
{
    XSync(ob_display, FALSE);
    xerror_ignore = ignore;
}
