/* -*- indent-tabs-mode: nil; tab-width: 4; c-basic-offset: 4; -*-

   config.h for the Openbox window manager
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

#ifndef __config_h
#define __config_h

#include "misc.h"
#include "stacking.h"
#include "place.h"
#include "client.h"
#include "geom.h"
#include "moveresize.h"
#include "render/render.h"

#include <glib.h>

struct _ObParseInst;

typedef struct _ObAppSettings ObAppSettings;

struct _ObAppSettings
{
    GPatternSpec *class;
    GPatternSpec *name;
    GPatternSpec *role;
    ObClientType  type;

    GravityPoint position;
    gboolean pos_given;
    gboolean pos_force;

    guint desktop;
    gint shade;
    gint decor;
    gint focus;
    gint monitor;
    gint iconic;
    gint skip_pager;
    gint skip_taskbar;
    gint max_horz;
    gint max_vert;
    gint fullscreen;

    gint layer;
};

/*! Should new windows be focused */
extern gboolean config_focus_new;
/*! Focus windows when the mouse enters them */
extern gboolean config_focus_follow;
/*! Timeout for focusing windows on focus follows mouse, in milliseconds */
extern guint    config_focus_delay;
/*! If windows should automatically be raised when they are focused in
 focus follows mouse */
extern gboolean config_focus_raise;
/*! Focus the last focused window, not under the mouse, in follow mouse mode */
extern gboolean config_focus_last;
/*! Try keep focus on the window under the mouse when the mouse is not moving
 */
extern gboolean config_focus_under_mouse;

/*! The algorithm to use for placing new windows */
extern ObPlacePolicy config_place_policy;
/*! Place windows in the center of the free area */
extern gboolean config_place_center;
/*! Place windows on the active monitor (unless they are part of an application
  already on another monitor) */
extern ObPlaceMonitor config_place_monitor;

/*! Place dialogs and stuff on this monitor.  Index starts at 1.  If this is
  0, then use the config_primary_monitor instead. */
extern guint config_primary_monitor_index;
/*! Where to place dialogs and stuff if it is not specified by index. */
extern ObPlaceMonitor config_primary_monitor;

/*! User-specified margins around the edge of the screen(s) */
extern StrutPartial config_margins;

/*! When true windows' contents are refreshed while they are resized; otherwise
  they are not updated until the resize is complete */
extern gboolean config_resize_redraw;
/*! show move/resize popups? 0 = no, 1 = always, 2 = only
  resizing !1 increments */
extern gint config_resize_popup_show;
/*! where to show the resize popup */
extern ObResizePopupPos config_resize_popup_pos;
/*! where to place the popup if it's in a fixed position */
extern GravityPoint config_resize_popup_fixed;

/*! The stacking layer the dock will reside in */
extern ObStackingLayer config_dock_layer;
/*! Is the dock floating */
extern gboolean config_dock_floating;
/*! Don't use a strut for the dock */
extern gboolean config_dock_nostrut;
/*! Where to place the dock if not floating */
extern ObDirection config_dock_pos;
/*! If config_dock_floating, this is the top-left corner's
  position */
extern gint config_dock_x;
/*! If config_dock_floating, this is the top-left corner's
  position */
extern gint config_dock_y;
/*! Whether the dock places the dockapps in it horizontally or vertically */
extern ObOrientation config_dock_orient;
/*! Whether to auto-hide the dock when the pointer is not over it */
extern gboolean config_dock_hide;
/*! The number of milliseconds to wait before hiding the dock */
extern guint config_dock_hide_delay;
/*! The number of milliseconds to wait before showing the dock */
extern guint config_dock_show_delay;
/*! The mouse button to be used to move dock apps */
extern guint config_dock_app_move_button;
/*! The modifiers to be used with the button to move dock apps */
extern guint config_dock_app_move_modifiers;

/*! The name of the theme */
extern gchar *config_theme;

/*! Show the one-pixel border after toggleDecor */
extern gboolean config_theme_keepborder;
/*! Titlebar button layout */
extern gchar *config_title_layout;
/*! Animate windows iconifying and restoring */
extern gboolean config_animate_iconify;

/*! The font for the active window's title */
extern RrFont *config_font_activewindow;
/*! The font for inactive windows' titles */
extern RrFont *config_font_inactivewindow;
/*! The font for menu titles */
extern RrFont *config_font_menutitle;
/*! The font for menu items */
extern RrFont *config_font_menuitem;
/*! The font for on-screen-displays/popups */
extern RrFont *config_font_osd;

/*! The number of desktops */
extern guint config_desktops_num;
/*! Desktop to start on, put 5 to start in the center of a 3x3 grid */
extern guint config_screen_firstdesk;
/*! Names for the desktops */
extern GSList *config_desktops_names;
/*! Amount of time to show the desktop switch dialog */
extern guint config_desktop_popup_time;

/*! The keycode of the key combo which resets the keybaord chains */
extern guint config_keyboard_reset_keycode;
/*! The modifiers of the key combo which resets the keybaord chains */
extern guint config_keyboard_reset_state;

/*! Number of pixels a drag must go before being considered a drag */
extern gint config_mouse_threshold;
/*! Number of milliseconds within which 2 clicks must occur to be a
  double-click */
extern gint config_mouse_dclicktime;
/*! Number of milliseconds that the mouse has to be on the screen edge before
  a screen edge event is triggered */
extern gint config_mouse_screenedgetime;

/*! Number of pixels to resist while crossing another window's edge */
extern gint config_resist_win;
/*! Number of pixels to resist while crossing a screen's edge */
extern gint config_resist_edge;

/*! Delay for hiding menu when opening in milliseconds */
extern guint    config_menu_hide_delay;
/*! Center menus vertically about the parent entry */
extern gboolean config_menu_middle;
/*! Delay before opening a submenu in milliseconds */
extern guint    config_submenu_show_delay;
/*! Delay before closing a submenu in milliseconds */
extern guint    config_submenu_hide_delay;
/*! Show icons in client_list_menu */
extern gboolean config_menu_client_list_icons;
/*! Show manage desktops in client_list_menu */
extern gboolean config_menu_manage_desktops;
/*! User-specified menu files */
extern GSList *config_menu_files;
/*! Per app settings */
extern GSList *config_per_app_settings;

void config_startup(struct _ObParseInst *i);
void config_shutdown(void);

/*! Create an ObAppSettings structure with the default values */
ObAppSettings* config_create_app_settings(void);
/*! Copies any settings in src to dest, if they are their default value in
  src. */
void config_app_settings_copy_non_defaults(const ObAppSettings *src,
                                           ObAppSettings *dest);

#endif
