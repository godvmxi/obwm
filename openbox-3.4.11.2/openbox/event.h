/* -*- indent-tabs-mode: nil; tab-width: 4; c-basic-offset: 4; -*-

   event.h for the Openbox window manager
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

#ifndef __events_h
#define __events_h

#include <X11/Xlib.h>
#include <glib.h>

struct _ObClient;

/*! The amount of time before a window appears that is checked for user input
    to determine if the user is working in another window */
#define OB_EVENT_USER_TIME_DELAY (500) /* 0.5 seconds */

/*! Time at which the last event with a timestamp occured. */
extern Time event_curtime;
/*! The last user-interaction time, as given by the clients */
extern Time event_last_user_time;

/*! The value of the mask for the NumLock modifier */
extern guint NumLockMask;
/*! The value of the mask for the ScrollLock modifier */
extern guint ScrollLockMask;

void event_startup(gboolean reconfig);
void event_shutdown(gboolean reconfig);

/*! Make as if the mouse just entered the client, use only when using focus
    follows mouse */
void event_enter_client(struct _ObClient *client);

/*! Make mouse focus not move at all from the stuff that happens between these
    two function calls. */
gulong event_start_ignore_all_enters(void);
void event_end_ignore_all_enters(gulong start);

/*! End *all* active and passive grabs on the keyboard
    This is called in situations where if there is a grab going on, then
    we need to cancel it. If we move focus during the grab, applications
    will get NotifyWhileGrabbed events and ignore them!

    Actions should not rely on being able to move focus during an
    interactive grab. */
void event_cancel_all_key_grabs(void);

/* Halts any focus delay in progress, use this when the user is selecting a
   window for focus */
void event_halt_focus_delay(void);

/*! Compare t1 and t2, taking into account wraparound. True if t1
    comes at the same time or later than t2. */
gboolean event_time_after(guint32 t1, guint32 t2);

Time event_get_server_time(void);

#endif
