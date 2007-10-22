/*-
 * Copyright (c) 2004 os-cillation e.K.
 * maemo specific changes: Copyright (c) 2005 Nokia Corporation
 *
 * Written by Benedikt Meurer <benny@xfce.org>.
 * maemo specific changes by Johan Hedberg <johan.hedberg@nokia.com>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc., 59 Temple
 * Place, Suite 330, Boston, MA  02111-1307  USA.
 *
 * The geometry handling code was taken from gnome-terminal. The geometry hacks
 * where initially written by Owen Taylor.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <time.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <stdlib.h>
#include <libosso.h>
#if BROWSER == 0
#include <osso-browser-interface.h>
#elif BROWSER == 1
#include <tablet-browser-interface.h>
#endif

#include <libintl.h>
#include <locale.h>

#define GETTEXT_PACKAGE "osso-browser-ui"
#define _(String) gettext(String)
#define N_(String) String

#if HILDON == 0
#include <hildon-widgets/hildon-window.h>
#include <hildon-widgets/hildon-program.h>
#include <hildon-widgets/hildon-defines.h>
#include <hildon-widgets/hildon-banner.h>
#elif HILDON == 1
#include <hildon/hildon-window.h>
#include <hildon/hildon-program.h>
#include <hildon/hildon-defines.h>
#include <hildon/hildon-banner.h>
#endif
#include <gconf/gconf-client.h>
#include <gdk/gdkkeysyms.h>

#include "terminal-gconf.h"
#include "terminal-settings.h"
#include "terminal-window.h"
#include "shortcuts.h"


#define ALEN(a) (sizeof(a)/sizeof((a)[0]))

static void            terminal_window_dispose                 (GObject         *object);
static void            terminal_window_finalize                (GObject         *object);
static void            terminal_window_update_actions          (TerminalWindow     *window);
static void            terminal_window_context_menu            (TerminalWidget  *widget,
                                                             GdkEvent        *event,
                                                             TerminalWindow     *window);
static void            terminal_window_notify_title            (TerminalWidget  *terminal,
                                                             GParamSpec      *pspec,
                                                               TerminalWindow     *window);
static void            terminal_window_open_url                (GtkAction       *action,
		                                             TerminalWindow     *window);
static void            terminal_window_action_new_window          (GtkAction       *action,
                                                             TerminalWindow     *window);
static void            terminal_window_action_close_tab        (GtkAction       *action,
                                                             TerminalWindow     *window);
static void            terminal_window_action_copy             (GtkAction       *action,
                                                             TerminalWindow     *window);
static void            terminal_window_action_paste            (GtkAction       *action,
                                                             TerminalWindow     *window);
static void            terminal_window_action_edit_shortcuts   (GtkAction       *action,
                                                             TerminalWindow     *window);
static void            terminal_window_action_reverse          (GtkToggleAction *action,
                                                             TerminalWindow     *window);
static void            terminal_window_action_fullscreen       (GtkToggleAction *action,
                                                             TerminalWindow     *window);
static void            terminal_window_action_scrollbar        (GtkToggleAction *action,
                                                             TerminalWindow     *window);
static void            terminal_window_action_toolbar        (GtkToggleAction *action,
                                                             TerminalWindow     *window);
static void            terminal_window_action_font_size        (GtkRadioAction  *action,
                                                             GtkRadioAction  *current,
                                                             TerminalWindow     *window);
static void            terminal_window_action_reset            (GtkAction       *action,
                                                             TerminalWindow     *window);
static void            terminal_window_action_reset_and_clear  (GtkAction       *action,
                                                             TerminalWindow     *window);
static void            terminal_window_action_ctrl             (GtkAction       *action,
                                                             TerminalWindow     *window);
static void            terminal_window_action_settings         (GtkAction       *action,
                                                             TerminalWindow     *window);
static void            terminal_window_action_quit             (GtkAction       *action,
                                                             TerminalWindow     *window);

static void            terminal_window_close_window            (GtkAction *action, 
                                                             TerminalWindow     *window);
#if 0
static void            terminal_window_on_close_window            (GtkWidget *widget, 
                                                                TerminalWindow     *window);
#endif
static void            terminal_window_action_show_full_screen (GtkToggleAction *action,
                                                             TerminalWindow     *window);
static void            terminal_window_action_show_normal_screen(GtkToggleAction *action,
                                                              TerminalWindow     *window);
static void
terminal_window_set_toolbar (TerminalWindow *window, gboolean show);
static void
terminal_window_set_toolbar_fs (TerminalWindow *window, gboolean show);
static void            terminal_window_select_all         (GtkAction       *action,
                                                             TerminalWindow     *window);
static void     terminal_window_gconf_toolbar               (GConfClient    *client,
                                                               guint           conn_id,
                                                               GConfEntry     *entry,
                                                               TerminalWindow *window);
static void     terminal_window_gconf_toolbar_fs            (GConfClient    *client,
                                                               guint           conn_id,
                                                               GConfEntry     *entry,
                                                               TerminalWindow *window);
static void     terminal_window_gconf_keys               (GConfClient    *client,
                                                               guint           conn_id,
                                                               GConfEntry     *entry,
                                                               TerminalWindow *window);
static void     terminal_window_update_keys          (TerminalWindow *window,
                                                               GSList *keys,
							       GSList *key_labels);
static void	terminal_window_do_key_button		     (GObject *button,
							      TerminalWindow *window);
static void terminal_window_ctrl_clicked (GtkButton    *item,
					TerminalWindow *window);

struct _TerminalWindow
{
  HildonWindow         __parent__;

  GtkActionGroup      *action_group;
  GtkUIManager        *ui_manager;

  GtkWidget *menubar; /* menubar */
  GtkWidget *windows_menu; /* Where window menuitems are*/
  GtkWidget *tbar;
  GtkToolItem *cbutton;
  GtkAction *menuaction; /* Window menuitem */

  TerminalWidget *terminal;

  GConfClient	      *gconf_client;
  GSList	      *keys;
  guint                toolbar_conid;
  guint                toolbar_fs_conid;
  guint                keys_conid;
  guint                key_labels_conid;

  gboolean	       is_fs;
};

static GObjectClass *parent_class;

static GtkActionEntry action_entries[] =
{

  { "file-menu", NULL, N_ ("File"), },
  { "open-url", NULL, N_ ("Open URL"), NULL, NULL, G_CALLBACK (terminal_window_open_url), },
  { "close-tab", NULL, N_ ("Close Tab"), NULL, NULL, G_CALLBACK (terminal_window_action_close_tab), },
  { "shortcuts", NULL, N_ ("Shortcuts..."), NULL, NULL, G_CALLBACK (terminal_window_action_edit_shortcuts), },
  { "go-menu", NULL, N_ ("Go"), },
  { "font-menu", NULL, N_ ("Font size"), },
  { "terminal-menu", NULL, N_ ("Terminal"), },
  { "reset", NULL, N_ ("Reset"), NULL, NULL, G_CALLBACK (terminal_window_action_reset), },
  { "reset-and-clear", NULL, N_ ("Reset and Clear"), NULL, NULL, G_CALLBACK (terminal_window_action_reset_and_clear), },
  { "ctrl", NULL, N_ ("Send Ctrl-<some key>"), NULL, NULL, G_CALLBACK (terminal_window_action_ctrl), },
  { "quit", NULL, N_ ("Quit"), NULL, NULL, G_CALLBACK (terminal_window_action_quit), },


  { "view-menu", NULL, ("webb_me_view"), },
  { "edit-menu", NULL, N_("weba_me_edit"),  },
  { "copy", NULL, N_("weba_me_copy"), NULL, NULL, G_CALLBACK (terminal_window_action_copy), },
  { "paste", NULL, N_("weba_me_paste"), NULL, NULL, G_CALLBACK (terminal_window_action_paste), },
  { "tools-menu", NULL, N_("weba_me_tools"), },
  { "close-menu", NULL, N_("weba_me_close"), },
  { "windows-menu", NULL, ("weba_me_windows"), },
  { "new-window", NULL, N_("weba_me_new_window"), NULL, NULL, G_CALLBACK (terminal_window_action_new_window), }, 
  { "tools", NULL, N_("weba_me_tools"), },
  { "close-window", NULL, N_("weba_me_close_window"), NULL, NULL, G_CALLBACK (terminal_window_close_window), },
  { "close-all-windows", NULL, N_("weba_me_close_all_windows"), NULL, NULL, G_CALLBACK (terminal_window_action_quit), },

  { "show-toolbar-menu", NULL, N_("webb_me_show_toolbar"), },

  { "settings", NULL, N_("weba_me_settings"), NULL, NULL, G_CALLBACK (terminal_window_action_settings), },
  { "select-all", NULL, ("weba_me_select_all"), NULL, NULL, G_CALLBACK (terminal_window_select_all), },

};

static GtkRadioActionEntry font_size_action_entries[] =
{
  { "-8pt", NULL, N_ ("-8 pt"), NULL, NULL, -8 },
  { "-6pt", NULL, N_ ("-6 pt"), NULL, NULL, -6 },
  { "-4pt", NULL, N_ ("-4 pt"), NULL, NULL, -4 },
  { "-2pt", NULL, N_ ("-2 pt"), NULL, NULL, -2 },
  { "+0pt", NULL, N_ ("+0 pt"), NULL, NULL, 0 },
  { "+2pt", NULL, N_ ("+2 pt"), NULL, NULL, +2 },
  { "+4pt", NULL, N_ ("+4 pt"), NULL, NULL, +4 },
  { "+6pt", NULL, N_ ("+6 pt"), NULL, NULL, +6 },
  { "+8pt", NULL, N_ ("+8 pt"), NULL, NULL, +8 },
};

static GtkToggleActionEntry toggle_action_entries[] =
{
  { "reverse", NULL, N_ ("Reverse colors"), NULL, NULL, G_CALLBACK (terminal_window_action_reverse), TRUE },
  { "scrollbar", NULL, N_ ("Scrollbar"), NULL, NULL, G_CALLBACK (terminal_window_action_scrollbar), TRUE },
  { "toolbar", NULL, N_ ("Toolbar"), NULL, NULL, G_CALLBACK (terminal_window_action_toolbar), TRUE },

  { "fullscreen", NULL, N_ ("webb_me_full_screen"), NULL, NULL, G_CALLBACK (terminal_window_action_fullscreen), FALSE },
  { "show-full-screen", NULL, N_ ("webb_me_full_screen"), NULL, NULL, G_CALLBACK (terminal_window_action_show_full_screen), TRUE},
  { "show-normal-screen", NULL, N_ ("webb_me_normal_mode"), NULL, NULL, G_CALLBACK(terminal_window_action_show_normal_screen), TRUE},
};

static const gchar ui_description[] =
 "<ui>"
 "  <popup name='popup-menu'>"
 "    <menuitem action='new-window'/>"
 "    <separator/>"
 "    <menuitem action='paste'/>"
 "  </popup>"
 "</ui>";

/*
 "    <separator/>"
 "    <menuitem action='settings'/>"
*/

G_DEFINE_TYPE (TerminalWindow, terminal_window, HILDON_TYPE_WINDOW);

enum signals {
    S_NEW_WINDOW = 0,
    S_COUNT
};

static guint sigs[S_COUNT];


typedef struct {
    GtkWidget *dialog;
    gchar *ret;
} ctrl_dialog_data;


static void
terminal_window_class_init (TerminalWindowClass *klass)
{
  GObjectClass *gobject_class;
  
  parent_class = g_type_class_peek_parent (klass);

  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->dispose = terminal_window_dispose;
  gobject_class->finalize = terminal_window_finalize;

  sigs[S_NEW_WINDOW] = g_signal_new("new_window",
				    TERMINAL_TYPE_WINDOW,
				    G_SIGNAL_RUN_LAST,
				    G_STRUCT_OFFSET(TerminalWindowClass,
						    new_window),
				    NULL,
				    NULL,
				    g_cclosure_marshal_VOID__STRING,
				    G_TYPE_NONE,
				    1, G_TYPE_STRING);
}

static GtkWidget *
attach_menu(GtkWidget *parent, GtkActionGroup *actiongroup,
            GtkAccelGroup *accelgroup, const gchar *name) {
    GtkAction *action;
    GtkWidget *menuitem, *menu;

    action = gtk_action_group_get_action(actiongroup, name);
    gtk_action_set_accel_group(action, accelgroup);
    menuitem = gtk_action_create_menu_item(action);
    gtk_menu_shell_append(GTK_MENU_SHELL(parent), menuitem);
    menu =  gtk_menu_new();
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(menuitem), menu);

    return menu;
}

static void
attach_item(GtkWidget *parent, GtkActionGroup *actiongroup,
            GtkAccelGroup *accelgroup, const gchar *name) {
    GtkAction *action;
    GtkWidget *menuitem;

    action = gtk_action_group_get_action(actiongroup, name);
    gtk_action_set_accel_group(action, accelgroup);
    menuitem = gtk_action_create_menu_item(action);
    gtk_menu_shell_append(GTK_MENU_SHELL(parent), menuitem);
}

#if 0
static void
on_window_menu_select (GtkWidget *widget, TerminalWindow *window)
{
    g_assert (TERMINAL_IS_WINDOW (window));
    g_debug (__FUNCTION__);
    gtk_window_present ( GTK_WINDOW (window));
}

static void
remove_item (gpointer data, GObject *obj)
{
    g_debug (__FUNCTION__);

    if (data != NULL && GTK_IS_WIDGET (data))
        gtk_widget_hide (GTK_WIDGET (data));
}

static void
attach_new_window (TerminalWindow *window, TerminalWindow *toaddapp, 
		   const gchar *label)
{
    GtkWidget *menuitem;
    GtkRadioAction *action = gtk_radio_action_new (label, 
						   label, 
						   NULL, 
						   label, 1);

    gtk_action_group_add_action (window->action_group, GTK_ACTION (action));

    gtk_radio_action_set_group (action, app_window_group);
    app_window_group = gtk_radio_action_get_group (action);

    menuitem = gtk_action_create_menu_item(GTK_ACTION (action));

    gtk_menu_shell_prepend(GTK_MENU_SHELL(toaddapp->windows_menu), menuitem);

    g_signal_connect (menuitem, "activate", G_CALLBACK (on_window_menu_select), window);
    window->menuaction = GTK_ACTION (action);

    /* This does not work as tought */
    g_object_weak_ref (G_OBJECT (window->menuaction), remove_item, menuitem);
    /* Here is no unref because menuaction is wanted to keep to controll menus*/
}
#endif

static void
populate_menubar (TerminalWindow *window, GtkAccelGroup *accelgroup)
{
  GtkActionGroup      *actiongroup = window->action_group;
  GtkWidget           *parent = NULL, *subparent;
  GtkWidget *menubar = NULL;

  menubar = gtk_menu_new();

  window->windows_menu = attach_menu(menubar, actiongroup, accelgroup, "windows-menu");

  parent = attach_menu(menubar, actiongroup, accelgroup, "edit-menu");
  attach_item(parent, actiongroup, accelgroup, "copy");
  attach_item(parent, actiongroup, accelgroup, "paste");
  /* TODO: ??? */
  /*  attach_item(parent, actiongroup, accelgroup, "select-all");
   */

  parent = attach_menu(menubar, actiongroup, accelgroup, "view-menu");
  attach_item(parent, actiongroup, accelgroup, "fullscreen");

  subparent = attach_menu(parent, actiongroup, accelgroup, "show-toolbar-menu");
  attach_item(subparent, actiongroup, accelgroup, "show-full-screen");
  attach_item(subparent, actiongroup, accelgroup, "show-normal-screen");

  parent = attach_menu(menubar, actiongroup, accelgroup, "tools-menu");
  attach_item(parent, actiongroup, accelgroup, "settings");

  parent = attach_menu(menubar, actiongroup, accelgroup, "close-menu");
  //  attach_item(parent, actiongroup, accelgroup, "close-window");
  attach_item(parent, actiongroup, accelgroup, "close-all-windows");
  
  hildon_window_set_menu(HILDON_WINDOW(window), GTK_MENU(menubar));

#if 0
  /* Add old windows to current */
  for (GSList *list = g_slist_nth (window_list, 0); list != NULL; 
	 list = g_slist_next (list) ) {
    GtkWidget *menuitem = gtk_action_create_menu_item(
				       GTK_ACTION (TERMINAL_WINDOW(list->data)->menuaction));
    gtk_menu_shell_prepend(GTK_MENU_SHELL(window->windows_menu), 
			   menuitem);

    gtk_radio_action_set_group (GTK_RADIO_ACTION (TERMINAL_WINDOW(list->data)->menuaction), app_window_group);
    app_window_group = gtk_radio_action_get_group (GTK_RADIO_ACTION (TERMINAL_WINDOW(list->data)->menuaction));

    g_object_weak_ref (G_OBJECT (TERMINAL_WINDOW (list->data)->menuaction), remove_item, menuitem);
    g_object_unref (TERMINAL_WINDOW(list->data)->menuaction);

  }
  gchar window_name[256];
  if(windows != 0)
    g_snprintf (window_name, 255, "XTerm (%d)", window_id++); /* window_id can overflow */
  else
    g_snprintf (window_name, 7, "XTerm");

  /* This for current window */
  attach_new_window (window, window, window_name);

  /* This for previous apps to add current window */
  for (GSList *list = g_slist_nth (window_list, 0); list != NULL; 
	 list = g_slist_next (list) ) {
    GtkWidget *menuitem = gtk_action_create_menu_item(GTK_ACTION (window->menuaction));
    gtk_menu_shell_prepend(GTK_MENU_SHELL(TERMINAL_WINDOW (list->data)->windows_menu), 
			   menuitem);

    /* This does not work as thought to */
    g_object_weak_ref (G_OBJECT (window->menuaction), remove_item, menuitem);
    g_object_unref (window->menuaction);

    gtk_radio_action_set_group (GTK_RADIO_ACTION (window->menuaction), app_window_group);
    app_window_group = gtk_radio_action_get_group (GTK_RADIO_ACTION (window->menuaction));
  }
#endif
  //  GtkWidget *menuitem = gtk_separator_menu_item_new();
  //  gtk_widget_show (menuitem);
  //  gtk_menu_shell_append(GTK_MENU_SHELL(window->windows_menu), 
  //			menuitem);
  attach_item(window->windows_menu, actiongroup, accelgroup, "new-window");
  window->menubar = menubar;
  gtk_widget_show_all(window->menubar);
}

static int
terminal_window_get_font_size(TerminalWindow *window) {
    GtkAction *action;

    action = gtk_action_group_get_action(window->action_group, "-8pt");
    if (!action) {
        return 0xf00b4;
    }

    return gtk_radio_action_get_current_value(GTK_RADIO_ACTION(action));
}

static
gboolean terminal_window_set_font_size(TerminalWindow *window, int new_size) {
    GtkAction *action;
    char new_name[5];

    if (new_size < -8 || new_size > 8) {
        return FALSE;
    }

    snprintf(new_name, sizeof(new_name), "%+dpt", new_size);

    action = gtk_action_group_get_action(window->action_group, new_name);
    if (!action) {
        return FALSE;
    }

    gtk_toggle_action_set_active(GTK_TOGGLE_ACTION(action), TRUE);
    gtk_toggle_action_toggled(GTK_TOGGLE_ACTION(action));

    return TRUE;
}


static gboolean
terminal_window_key_press_event (TerminalWindow *window,
                              GdkEventKey *event,
                              gpointer user_data) {

    int font_size;
    GtkAction *action;

    switch (event->keyval) 
    {
        case HILDON_HARDKEY_FULLSCREEN: /* Full screen */
            action = gtk_action_group_get_action(window->action_group,
                                                 "fullscreen");
	    gtk_action_activate(action);
            return TRUE;

        case HILDON_HARDKEY_INCREASE: /* Zoom in */
            font_size = terminal_window_get_font_size(window);
            if (font_size == 0xf00b4) {
		hildon_banner_show_information(GTK_WIDGET(window), NULL,
			"Getting font size failed!");
                return TRUE;
            }

            if (font_size >= 8) {
		hildon_banner_show_information(GTK_WIDGET(window), NULL,
			"Already at maximum font size.");
                return TRUE;
            }
            terminal_window_set_font_size(window, font_size + 2);
            return TRUE;

        case HILDON_HARDKEY_DECREASE: /* Zoom out */
            font_size = terminal_window_get_font_size(window);
            if (font_size == 0xf00b4) {
		hildon_banner_show_information(GTK_WIDGET(window), NULL,
			"Getting font size failed!");
                return TRUE;
            }
            
            if (font_size <= -8) {
		hildon_banner_show_information(GTK_WIDGET(window), NULL,
			"Already at minimum font size.");
                return TRUE;
            }
            terminal_window_set_font_size(window, font_size - 2);
            return TRUE;
	case HILDON_HARDKEY_HOME: /* Ignoring... */
	    return TRUE;
    }

    return FALSE;
}

static void
terminal_window_init (TerminalWindow *window)
{
  GtkAction           *action;
  GtkAccelGroup       *accel_group;
  GtkWidget           *popup;
  GError              *error = NULL;
  gchar               *role;
  gint                 font_size;
  gboolean             scrollbar, toolbar, toolbar_fs, reverse;
  GConfValue          *gconf_value;
  GSList	      *keys;
  GSList	      *key_labels;

  window->terminal = NULL;
  gtk_window_set_title(GTK_WINDOW(window), "osso_xterm");

  g_signal_connect( G_OBJECT(window), "key-press-event",
                    G_CALLBACK(terminal_window_key_press_event), NULL);

  window->gconf_client = gconf_client_get_default();
  
  window->toolbar_conid = gconf_client_notify_add(window->gconf_client,
          OSSO_XTERM_GCONF_TOOLBAR,
          (GConfClientNotifyFunc)terminal_window_gconf_toolbar,
          window,
          NULL, &error);
  window->toolbar_fs_conid = gconf_client_notify_add(window->gconf_client,
          OSSO_XTERM_GCONF_TOOLBAR_FS,
          (GConfClientNotifyFunc)terminal_window_gconf_toolbar_fs,
          window,
          NULL, &error);
  window->keys_conid = gconf_client_notify_add(window->gconf_client,
          OSSO_XTERM_GCONF_KEYS,
          (GConfClientNotifyFunc)terminal_window_gconf_keys,
          window,
          NULL, &error);
  window->key_labels_conid = gconf_client_notify_add(window->gconf_client,
          OSSO_XTERM_GCONF_KEY_LABELS,
          (GConfClientNotifyFunc)terminal_window_gconf_keys,
          window,
          NULL, &error);
  font_size = gconf_client_get_int(window->gconf_client,
                                   OSSO_XTERM_GCONF_FONT_SIZE,
                                   &error);
  if (error != NULL) {
      g_printerr("Unable to get font size from gconf: %s\n",
                 error->message);
      g_error_free(error);
      error = NULL;
  }

  gconf_value = gconf_client_get(window->gconf_client,
                                 OSSO_XTERM_GCONF_SCROLLBAR,
                                 &error);

  if (error != NULL) {
      g_printerr("Unable to get scrollbar setting from gconf: %s\n",
                 error->message);
      g_error_free(error);
      error = NULL;
  }
  scrollbar = OSSO_XTERM_DEFAULT_SCROLLBAR;
  if (gconf_value) {
          if (gconf_value->type == GCONF_VALUE_BOOL)
                  scrollbar = gconf_value_get_bool(gconf_value);
          gconf_value_free(gconf_value);
  }

  gconf_value = gconf_client_get(window->gconf_client,
                                 OSSO_XTERM_GCONF_TOOLBAR,
                                 &error);

  if (error != NULL) {
      g_printerr("Unable to get toolbar setting from gconf: %s\n",
                 error->message);
      g_error_free(error);
      error = NULL;
  }

  toolbar = OSSO_XTERM_DEFAULT_TOOLBAR;
  if (gconf_value) {
          if (gconf_value->type == GCONF_VALUE_BOOL)
                  toolbar = gconf_value_get_bool(gconf_value);
          gconf_value_free(gconf_value);
  }

  gconf_value = gconf_client_get(window->gconf_client,
                                 OSSO_XTERM_GCONF_TOOLBAR_FS,
                                 &error);

  if (error != NULL) {
      g_printerr("Unable to get toolbar setting from gconf: %s\n",
                 error->message);
      g_error_free(error);
      error = NULL;
  }

  toolbar_fs = OSSO_XTERM_DEFAULT_TOOLBAR_FS;
  if (gconf_value) {
          if (gconf_value->type == GCONF_VALUE_BOOL)
                  toolbar_fs = gconf_value_get_bool(gconf_value);
          gconf_value_free(gconf_value);
  }

  gconf_value = gconf_client_get(window->gconf_client,
                                 OSSO_XTERM_GCONF_REVERSE,
                                 &error);
  if (error != NULL) {
      g_printerr("Unable to get reverse setting from gconf: %s\n",
                 error->message);
      g_error_free(error);
      error = NULL;
  }
  reverse = OSSO_XTERM_DEFAULT_REVERSE;
  if (gconf_value) {
          if (gconf_value->type == GCONF_VALUE_BOOL)
                  reverse = gconf_value_get_bool(gconf_value);
          gconf_value_free(gconf_value);
  }

  window->action_group = gtk_action_group_new ("terminal-app");
  gtk_action_group_set_translation_domain (window->action_group,
                                           GETTEXT_PACKAGE);
  gtk_action_group_add_actions (window->action_group,
                                action_entries,
                                G_N_ELEMENTS (action_entries),
                                GTK_WIDGET (window));
  gtk_action_group_add_toggle_actions (window->action_group,
                                       toggle_action_entries,
                                       G_N_ELEMENTS (toggle_action_entries),
                                       GTK_WIDGET (window));

  gtk_action_group_add_radio_actions (window->action_group,
                                      font_size_action_entries,
                                      G_N_ELEMENTS(font_size_action_entries),
                                      font_size,
                                      G_CALLBACK(terminal_window_action_font_size),
                                      GTK_WIDGET(window));;

  action = gtk_action_group_get_action(window->action_group, "scrollbar");
  gtk_toggle_action_set_active(GTK_TOGGLE_ACTION(action), scrollbar);

  action = gtk_action_group_get_action(window->action_group, "reverse");
  gtk_toggle_action_set_active(GTK_TOGGLE_ACTION(action), reverse);

  window->ui_manager = gtk_ui_manager_new ();
  gtk_ui_manager_insert_action_group (window->ui_manager, window->action_group, 0);
  if (gtk_ui_manager_add_ui_from_string (window->ui_manager,
                                         ui_description, strlen(ui_description),
                                         &error) == 0)
  {
      g_warning ("Unable to create menu: %s", error->message);
      g_error_free (error);
  }

  accel_group = gtk_ui_manager_get_accel_group (window->ui_manager);
  gtk_window_add_accel_group (GTK_WINDOW (window), accel_group);

  populate_menubar(window, accel_group);

  popup = gtk_ui_manager_get_widget (window->ui_manager, "/popup-menu");
  gtk_widget_tap_and_hold_setup(GTK_WIDGET(window), popup, NULL,
                                GTK_TAP_AND_HOLD_NONE);

#if 1
  g_signal_connect_swapped (G_OBJECT (window), "destroy",
                    G_CALLBACK (gtk_widget_destroy), window);
#endif

  /* setup fullscreen mode */
  if (!gdk_net_wm_supports (gdk_atom_intern ("_NET_WM_STATE_FULLSCREEN", FALSE)))
    {
      action = gtk_action_group_get_action (window->action_group, "fullscreen");
      g_object_set (G_OBJECT (action), "sensitive", FALSE, NULL);
    }

  /* set a unique role on each window (for session management) */
  role = g_strdup_printf ("Terminal-%p-%d-%d", window, getpid (), (gint) time (NULL));
  gtk_window_set_role (GTK_WINDOW (window), role);
  g_free (role);

  window->tbar = gtk_toolbar_new ();
  g_object_set(window->tbar, 
		  "orientation", GTK_ORIENTATION_HORIZONTAL,
		  NULL);
  hildon_window_add_toolbar(HILDON_WINDOW(window), GTK_TOOLBAR(window->tbar));

  action = gtk_action_group_get_action(window->action_group, "show-full-screen");
  gtk_toggle_action_set_active(GTK_TOGGLE_ACTION(action), toolbar_fs);
  action = gtk_action_group_get_action(window->action_group, "show-normal-screen");
  gtk_toggle_action_set_active(GTK_TOGGLE_ACTION(action), toolbar);

  window->cbutton = gtk_tool_button_new (NULL, "Ctrl");
//  gtk_tool_item_set_expand(widget->cbutton, TRUE);
//  gtk_tool_button_set_label(GTK_TOOL_BUTTON(widget->cbutton), "Ctrl");
  gtk_widget_show(GTK_WIDGET(window->cbutton));

  /*
  g_signal_connect (G_OBJECT(widget->terminal), "notify::ctrlify",
		    G_CALLBACK(terminal_widget_vte_ctrlify_notify),
		    widget);
		    */
/*
  g_signal_connect (G_OBJECT(widget->cbutton), "toggled",
		    G_CALLBACK(terminal_widget_ctrlify_notify),
		    widget);
*/
  g_signal_connect (G_OBJECT(window->cbutton), "clicked",
		    G_CALLBACK(terminal_window_ctrl_clicked),
		    window);

  gtk_toolbar_insert(GTK_TOOLBAR(window->tbar),
		  window->cbutton,
		  -1);

  keys = gconf_client_get_list(window->gconf_client,
                                 OSSO_XTERM_GCONF_KEYS,
				 GCONF_VALUE_STRING,
                                 &error);
  key_labels = gconf_client_get_list(window->gconf_client,
                                 OSSO_XTERM_GCONF_KEY_LABELS,
				 GCONF_VALUE_STRING,
                                 &error);

  terminal_window_update_keys(TERMINAL_WINDOW(window), keys, key_labels);

  g_slist_foreach(keys, (GFunc)g_free, NULL);
  g_slist_foreach(key_labels, (GFunc)g_free, NULL);
  g_slist_free(keys);
  g_slist_free(key_labels);

  if (toolbar) {
	  gtk_widget_show(window->tbar);
  }

  window->is_fs = FALSE;
}


static void
terminal_window_dispose (GObject *object)
{
  parent_class->dispose (object);
}



static void
terminal_window_finalize (GObject *object)
{
  TerminalWindow *window = TERMINAL_WINDOW (object);

  g_debug (__FUNCTION__);

  if (window->terminal != NULL)
    g_object_unref (window->terminal);
  if (window->action_group != NULL)
    g_object_unref (G_OBJECT (window->action_group));
  if (window->ui_manager != NULL)
    g_object_unref (G_OBJECT (window->ui_manager));

  gconf_client_notify_remove(window->gconf_client,
                             window->toolbar_conid);
  gconf_client_notify_remove(window->gconf_client,
                             window->toolbar_fs_conid);
  gconf_client_notify_remove(window->gconf_client,
                             window->keys_conid);
  gconf_client_notify_remove(window->gconf_client,
                             window->key_labels_conid);

  g_object_unref(G_OBJECT(window->gconf_client));

  parent_class->finalize (object);
}


static TerminalWidget*
terminal_window_get_widget (TerminalWindow *window)
{
  return TERMINAL_WIDGET (window->terminal);    
}

static void
terminal_window_update_actions (TerminalWindow *window)
{
  TerminalWidget *terminal;
  GtkAction      *action;

  terminal = terminal_window_get_widget (window);
  if (G_LIKELY (terminal != NULL))
    {
      action = gtk_action_group_get_action (window->action_group, "copy");
      g_object_set (G_OBJECT (action),
                    "sensitive", terminal_widget_has_selection (terminal),
                    NULL);
    }
}


static void
terminal_window_notify_title(TerminalWidget *terminal,
                          GParamSpec   *pspec,
                          TerminalWindow  *window)
{
  gchar *terminal_title = terminal_widget_get_title(terminal);
  gtk_window_set_title(GTK_WINDOW(window), terminal_title);
  g_free(terminal_title);
}

static void
terminal_window_context_menu (TerminalWidget  *widget,
                           GdkEvent        *event,
                           TerminalWindow     *window)
{
  TerminalWidget *terminal;
  GtkWidget      *popup;
  gint            button = 0;
  gint            time;

  terminal = terminal_window_get_widget (window);

  if (G_LIKELY (widget == terminal))
    {
      popup = gtk_ui_manager_get_widget (window->ui_manager, "/popup-menu");
      gtk_widget_show_all(GTK_WIDGET(popup));

      if (G_UNLIKELY (popup == NULL))
        return;

      if (event != NULL)
        {
          if (event->type == GDK_BUTTON_PRESS)
            {
	      char *msg = terminal_widget_get_tag(terminal,
			      event->button.x,
			      event->button.y,
			      NULL);
		  GtkWidget *item = gtk_ui_manager_get_widget(window->ui_manager,
				  "/popup-menu/open-url");
	      if (msg == NULL)
	        {
		  gtk_widget_hide(item);
		  g_object_set_data(G_OBJECT(window), "url", NULL);
		}
	      else
	        {
		  gtk_widget_show(item);
		  g_object_set_data_full(G_OBJECT(window), "url", msg, g_free);
		}
              button = event->button.button;
	    }
	    time = event->button.time;
        }
      else
        {
          time = gtk_get_current_event_time ();
        }

      terminal->im_pending = FALSE;

      gtk_menu_popup (GTK_MENU (popup), NULL, NULL,
                      NULL, NULL, button, time);
    }
}



static void
terminal_window_open_url (GtkAction	*action,
		       TerminalWindow	*window)
{
  osso_context_t *osso = (osso_context_t *)g_object_get_data(
		  G_OBJECT(window), "osso");
  gchar *url = (gchar *)g_object_get_data(G_OBJECT(window), "url");

  if (url && osso) {
    osso_rpc_run_with_defaults(osso,
		    "osso_browser",
		    OSSO_BROWSER_OPEN_NEW_WINDOW_REQ,
		    NULL,
		    DBUS_TYPE_STRING,
		    url,
		    DBUS_TYPE_INVALID);
  }
  g_object_set_data(G_OBJECT(window), "url", NULL);
}


static void
terminal_window_action_new_window (GtkAction    *action,
                             TerminalWindow  *window)
{
  g_signal_emit(window, sigs[S_NEW_WINDOW], 0, NULL);
}

static void
terminal_window_action_close_tab (GtkAction    *action,
                               TerminalWindow  *window)
{
  gtk_widget_hide (GTK_WIDGET (window));
  gtk_widget_destroy (GTK_WIDGET (window));
}


static void
terminal_window_action_copy (GtkAction    *action,
                          TerminalWindow  *window)
{
  TerminalWidget *terminal;

  terminal = terminal_window_get_widget (window);
  if (G_LIKELY (terminal != NULL))
    terminal_widget_copy_clipboard (terminal);
}


static void
terminal_window_action_paste (GtkAction    *action,
                           TerminalWindow  *window)
{
  TerminalWidget *terminal;

  terminal = terminal_window_get_widget (window);
  if (G_LIKELY (terminal != NULL))
    terminal_widget_paste_clipboard (terminal);
}

static void
terminal_window_action_edit_shortcuts (GtkAction    *action,
                           TerminalWindow  *window)
{
  (void)action;
  (void)window;

//  update_shortcut_keys();
}


static void
terminal_window_action_reverse (GtkToggleAction *action,
                             TerminalWindow     *window)
{
    GConfClient *client;
    gboolean reverse;

    client = gconf_client_get_default();
    reverse = gtk_toggle_action_get_active (action);

    gconf_client_set_bool (client,
                           OSSO_XTERM_GCONF_REVERSE,
                           reverse,
                           NULL);

    g_object_unref(G_OBJECT(client));
}

static void
terminal_window_action_fullscreen (GtkToggleAction *action,
                                TerminalWindow     *window)
{
  gboolean fullscreen;

  fullscreen = gtk_toggle_action_get_active (action);
  if (fullscreen) {
      GtkToggleAction *toolbar_fs =
	      GTK_TOGGLE_ACTION(
			      gtk_action_group_get_action(window->action_group,
			      "show-full-screen"));
      gtk_window_fullscreen(GTK_WINDOW(window));

      window->is_fs = TRUE;
      
      if (gtk_toggle_action_get_active(toolbar_fs)) {
	      gtk_widget_show(window->tbar);
      } else {
	      gtk_widget_hide(window->tbar);
      }
  } else {
      GtkToggleAction *toolbar =
	      GTK_TOGGLE_ACTION(
			      gtk_action_group_get_action(window->action_group,
			      "show-normal-screen"));
      gtk_window_unfullscreen(GTK_WINDOW(window));

      window->is_fs = FALSE;
      
      if (gtk_toggle_action_get_active(toolbar)) {
	      gtk_widget_show(window->tbar);
      } else {
	      gtk_widget_hide(window->tbar);
      }
  }
}

static void
terminal_window_set_toolbar (TerminalWindow *window, gboolean show)
{
    gconf_client_set_bool (window->gconf_client,
                           OSSO_XTERM_GCONF_TOOLBAR,
                           show,
                           NULL);
}

static void
terminal_window_set_toolbar_fs (TerminalWindow *window, gboolean show)
{
    gconf_client_set_bool (window->gconf_client,
                           OSSO_XTERM_GCONF_TOOLBAR_FS,
                           show,
                           NULL);
}

static void
terminal_window_action_toolbar (GtkToggleAction *action,
                               TerminalWindow     *window)
{
	(void)action;
	(void)window;
}

static void
terminal_window_action_scrollbar (GtkToggleAction *action,
                               TerminalWindow     *window)
{
    GConfClient *client;
    gboolean show;

    client = gconf_client_get_default();
    show = gtk_toggle_action_get_active (action);

    gconf_client_set_bool (client,
                           OSSO_XTERM_GCONF_SCROLLBAR,
                           show,
                           NULL);

    g_object_unref(G_OBJECT(client));
}

static void
terminal_window_action_font_size (GtkRadioAction *action,
                               GtkRadioAction *current,
                               TerminalWindow    *window)
{
    GConfClient *client;
    gint size;

    client = gconf_client_get_default();
    size = gtk_radio_action_get_current_value(current);

    gconf_client_set_int (client,
                          OSSO_XTERM_GCONF_FONT_SIZE,
                          size,
                          NULL);

    g_object_unref(G_OBJECT(client));
}


static void
terminal_window_action_reset (GtkAction   *action,
                           TerminalWindow *window)
{
  TerminalWidget *widget;

  widget = terminal_window_get_widget (window);
  terminal_widget_reset (widget, FALSE);
}


static void
terminal_window_action_reset_and_clear (GtkAction    *action,
                                     TerminalWindow  *window)
{
  TerminalWidget *widget;

  widget = terminal_window_get_widget (window);
  terminal_widget_reset (widget, TRUE);
}

static void
terminal_window_send_ctrl_key(TerminalWindow *window,
                           const char *str)
{
  GdkEventKey *key;

  key = (GdkEventKey *) gdk_event_new(GDK_KEY_PRESS);

  key->window = GTK_WIDGET(window)->window;
  key->time = GDK_CURRENT_TIME;
  key->state = GDK_CONTROL_MASK;
  key->keyval = gdk_keyval_from_name(str);
  gdk_event_put ((GdkEvent *) key);

  key->type = GDK_KEY_RELEASE;
  key->state |= GDK_RELEASE_MASK;
  gdk_event_put ((GdkEvent *) key);

  gdk_event_free((GdkEvent *) key);
}

static gboolean ctrl_dialog_focus(GtkWidget *dialog,
                                  GdkEventFocus *event,
                                  GtkIMContext *imctx)
{
  if (event->in) {
    gtk_im_context_focus_in(imctx);
    gtk_im_context_show(imctx);
  } else
    gtk_im_context_focus_out(imctx);
  return FALSE;
}

static gboolean
im_context_commit (GtkIMContext *ctx,
                   const gchar *str,
                   ctrl_dialog_data *data)
{
    if (strlen(str) > 0) {
      data->ret = g_strdup(str);
      gtk_dialog_response(GTK_DIALOG(data->dialog), GTK_RESPONSE_ACCEPT);
    }

    return TRUE;
}

static void
terminal_window_action_ctrl (GtkAction    *action,
                          TerminalWindow  *window)
{
  ctrl_dialog_data *data;
  GtkWidget *dialog, *label;
  GtkIMContext *imctx;

  dialog = gtk_dialog_new_with_buttons("Control",
                                       GTK_WINDOW(window),
                                       GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
                                       "Cancel", GTK_RESPONSE_CANCEL,
                                       NULL);

  imctx = gtk_im_multicontext_new();

  data = g_new0(ctrl_dialog_data, 1);
  data->dialog = dialog;
  g_signal_connect(imctx, "commit", G_CALLBACK(im_context_commit), data);

  label = gtk_label_new("Press a key (or cancel)");
  gtk_box_pack_start_defaults(GTK_BOX(GTK_DIALOG(dialog)->vbox), label);

  gtk_widget_show_all(dialog);

  gtk_im_context_set_client_window(imctx, GTK_WIDGET(dialog)->window);

  g_signal_connect( G_OBJECT(dialog), "focus-in-event",
          G_CALLBACK(ctrl_dialog_focus), imctx);
  g_signal_connect( G_OBJECT(dialog), "focus-out-event",
          G_CALLBACK(ctrl_dialog_focus), imctx);

  gtk_dialog_run(GTK_DIALOG(dialog));

  gtk_widget_hide(dialog);
  gtk_widget_destroy(dialog);

  gtk_im_context_focus_out(imctx);
  g_object_unref(G_OBJECT(imctx));

  if (data->ret != NULL) {
    terminal_window_send_ctrl_key(window, data->ret);
    g_free(data->ret);
  }

  g_free(data);
}


static void
terminal_window_action_settings (GtkAction    *action,
                              TerminalWindow  *window)
{
    GtkWidget *settings;
    
    settings = terminal_settings_new(GTK_WINDOW(window));
	gtk_widget_set_size_request (GTK_WIDGET (settings), 300, 200);

    switch (gtk_dialog_run(GTK_DIALOG(settings))) {
        case GTK_RESPONSE_OK:
            terminal_settings_store(TERMINAL_SETTINGS(settings));
            break;
        case GTK_RESPONSE_CANCEL:
            break;
        default:
            break;
    }
    gtk_widget_destroy(settings);
}


static void
terminal_window_action_quit (GtkAction    *action,
                          TerminalWindow  *window)
{
  //  gtk_widget_destroy(GTK_WIDGET(window));
  gtk_main_quit ();
}


/**
 * terminal_window_new:
 *
 * Return value :
 **/
GtkWidget*
terminal_window_new (void)
{
  return g_object_new (TERMINAL_TYPE_WINDOW, NULL);
}


static void
terminal_window_real_add (TerminalWindow    *window,
		       TerminalWidget *widget)
{
  /*
    gchar *title = terminal_widget_get_title(widget);
    g_object_set(GTK_WINDOW (window), "title", title, NULL);
    g_free(title);
  */
    gtk_container_add(GTK_CONTAINER (window), GTK_WIDGET(widget));

    /* add terminal to window */
    TERMINAL_WINDOW (window)->terminal = widget;

    g_signal_connect(G_OBJECT(widget), "notify::title",
  		    G_CALLBACK(terminal_window_notify_title), window);
    g_signal_connect (G_OBJECT (widget), "context-menu",
                      G_CALLBACK (terminal_window_context_menu), window);
    g_signal_connect_swapped (G_OBJECT (widget), "selection-changed",
                              G_CALLBACK (terminal_window_update_actions), window);
    g_signal_connect_swapped(G_OBJECT(widget), "destroy",
		    G_CALLBACK (gtk_widget_destroy), window);
    terminal_window_update_actions (window);

}
/**
 * terminal_window_add:
 * @app    : A #TerminalWindow.
 * @widget : A #TerminalWidget.
 **/
void
terminal_window_add (TerminalWindow    *window,
                  TerminalWidget *widget)
{
  g_return_if_fail (TERMINAL_IS_WINDOW (window));
  g_return_if_fail (TERMINAL_IS_WIDGET (widget));

  gtk_widget_show (GTK_WIDGET (widget));

  terminal_window_real_add (window, widget);

  g_object_ref (widget);
}


/**
 * terminal_window_remove:
 * @app     :
 * @widget  :
 **/
void
terminal_window_remove (TerminalWindow    *window,
                     TerminalWidget *widget)
{
  g_return_if_fail (TERMINAL_IS_WINDOW (window));
  g_return_if_fail (TERMINAL_IS_WIDGET (widget));

  gtk_widget_destroy (GTK_WIDGET (widget));
}

/**
 * terminal_window_launch
 * @app         : A #TerminalWindow.
 * @error       : Location to store error to, or %NULL.
 *
 * Return value : %TRUE on success, %FALSE on error.
 **/
gboolean
terminal_window_launch (TerminalWindow     *window,
		             const gchar     *command,
                     GError          **error)
{
  GtkWidget *terminal;
  g_return_val_if_fail (TERMINAL_IS_WINDOW (window), FALSE);

  /* setup the terminal widget */
  terminal = terminal_widget_new ();
  terminal_widget_set_working_directory(TERMINAL_WIDGET(terminal),
		 g_get_home_dir()); 

  terminal_window_add (window, TERMINAL_WIDGET (terminal));
  if (command) {
    gint argc;
    gchar **argv;

    if (g_shell_parse_argv(command,
	  &argc,
	  &argv,
	  NULL)) {
      terminal_widget_set_custom_command(TERMINAL_WIDGET (terminal),
	  argv);
      g_strfreev(argv);
    }
  }
  terminal_widget_launch_child (TERMINAL_WIDGET (terminal));

  /* Keep IM open on startup */
  hildon_gtk_im_context_show(TERMINAL_WIDGET(terminal)->im_context);

  gtk_widget_show_all(GTK_WIDGET(window));
  terminal_window_action_fullscreen (GTK_TOGGLE_ACTION(gtk_action_group_get_action(window->action_group, "fullscreen")), window);

  return TRUE;
}

#if 1
static void
terminal_window_close_window(GtkAction *action, TerminalWindow *window)
{
  g_assert (window);
  g_assert (TERMINAL_IS_WINDOW (window));

  g_debug (__FUNCTION__);

  /* Remove window menuitems from all windows */
  //  g_object_unref (window->menuaction);

    //gtk_widget_hide (GTK_WIDGET (window));
  // g_object_unref (G_OBJECT (window));
  gtk_widget_destroy (GTK_WIDGET (window));
}
#endif

#if 0
static void
terminal_window_on_close_window(GtkWidget *widget, TerminalWindow *window)
{
    g_assert (window);
    g_assert (TERMINAL_IS_WINDOW (window));
    

}
#endif

static void            
terminal_window_action_show_full_screen (GtkToggleAction *action,
                                      TerminalWindow     *window)
{
    gboolean toolbar_fs = gtk_toggle_action_get_active (action);
    if (window->is_fs) {
	if (toolbar_fs) {
	    gtk_widget_show(window->tbar);
	} else {
	    gtk_widget_hide(window->tbar);
	}
    }
    terminal_window_set_toolbar_fs(window, toolbar_fs);
}

static void
terminal_window_action_show_normal_screen(GtkToggleAction *action,
                                       TerminalWindow     *window)
{
    gboolean toolbar = gtk_toggle_action_get_active (action);
    if (!window->is_fs) {
	if (toolbar) {
	    gtk_widget_show(window->tbar);
	} else {
	    gtk_widget_hide(window->tbar);
	}
    }
    terminal_window_set_toolbar(window, toolbar);
}

static void
terminal_window_select_all (GtkAction    *action,
                              TerminalWindow  *window)
{
    g_assert (window != NULL);
    g_assert (TERMINAL_IS_WINDOW (window));
    /* terminal_widget_select_all (TERMINAL_WIDGET (window->terminal)); */
    g_debug(__FUNCTION__);
}

static void
terminal_window_update_keys (TerminalWindow *window, GSList *keys, GSList *key_labels)
{
	g_slist_foreach(window->keys, (GFunc)gtk_widget_destroy, NULL);
	g_slist_free(window->keys);
	window->keys = NULL;
	guint i = 0;

	while (keys && key_labels) {
		GtkToolItem *button = gtk_tool_button_new(NULL, key_labels->data);
		g_object_set_data_full(G_OBJECT(button), "keys", g_strdup(keys->data), g_free);

		gtk_widget_show(GTK_WIDGET(button));
		gtk_toolbar_insert(GTK_TOOLBAR(window->tbar), 
				button, i++);

		g_signal_connect(G_OBJECT(button),
				"clicked",
				G_CALLBACK(terminal_window_do_key_button),
				window);

		window->keys = g_slist_append(window->keys,
				button);

		keys = g_slist_next(keys);
		key_labels = g_slist_next(key_labels);
	}
}

static void
terminal_window_gconf_keys(GConfClient    *client,
                                guint           conn_id,
                                GConfEntry     *entry,
                                TerminalWindow *window)
{
    GSList *keys;
    GSList *key_labels;

    (void)entry;
    (void)conn_id;

    key_labels = gconf_client_get_list(client,
		OSSO_XTERM_GCONF_KEY_LABELS,
		GCONF_VALUE_STRING,
		NULL);
    keys = gconf_client_get_list(client,
		OSSO_XTERM_GCONF_KEYS,
		GCONF_VALUE_STRING,
		NULL);

    terminal_window_update_keys(window, keys, key_labels);

    g_slist_foreach(keys, (GFunc)g_free, NULL);
    g_slist_foreach(key_labels, (GFunc)g_free, NULL);
    g_slist_free(keys);
    g_slist_free(key_labels);
}

static void terminal_window_do_key_button(GObject *button,
		TerminalWindow *window)
{
	terminal_widget_send_keys(terminal_window_get_widget(window),
				  g_object_get_data(button, "keys"));
}

static void
terminal_window_gconf_toolbar(GConfClient    *client,
                                guint           conn_id,
                                GConfEntry     *entry,
                                TerminalWindow *window) {
    GConfValue *value;
    GtkAction *action = gtk_action_group_get_action(window->action_group,
		    "show-normal-screen");
    gboolean toolbar;

    value = gconf_entry_get_value(entry);
    toolbar = gconf_value_get_bool(value);

    if (toolbar ^ gtk_toggle_action_get_active(GTK_TOGGLE_ACTION(action))) {
	    gtk_action_activate(action);
    }
}

static void
terminal_window_gconf_toolbar_fs(GConfClient    *client,
                                guint           conn_id,
                                GConfEntry     *entry,
                                TerminalWindow *window) {
    GConfValue *value;
    GtkAction *action = gtk_action_group_get_action(window->action_group,
		    "show-full-screen");
    gboolean toolbar_fs;

    value = gconf_entry_get_value(entry);
    toolbar_fs = gconf_value_get_bool(value);


    if (toolbar_fs ^ gtk_toggle_action_get_active(GTK_TOGGLE_ACTION(action))) {
	    gtk_action_activate(action);
    }
}

static void
terminal_window_ctrl_clicked (GtkButton    *item,
				TerminalWindow *window)
{
//  ctrl_dialog_data *data;
  GtkWidget *dialog, *label, *input;
//  GtkIMContext *imctx;
  gchar *label_text;
  gchar *text = NULL;

  dialog = gtk_dialog_new_with_buttons("Control",
                                       GTK_WINDOW(window),
                                       GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
                                       _("weba_bd_ok"), GTK_RESPONSE_OK,
                                       NULL);

//  imctx = gtk_im_multicontext_new();

//  data = g_new0(ctrl_dialog_data, 1);
//  data->dialog = dialog;
//  g_signal_connect(imctx, "commit", G_CALLBACK(im_context_commit), data);

  label_text = g_strdup_printf ("Ctrl + [%s]", 
              dgettext("osso-applet-textinput", "tein_ti_text_input_title"));
  label = gtk_label_new(label_text);
  g_free(label_text);
  gtk_box_pack_start_defaults(GTK_BOX(GTK_DIALOG(dialog)->vbox), label);

  input = gtk_entry_new ();
  gtk_entry_set_max_length (GTK_ENTRY (input), 1);
  gtk_entry_set_width_chars (GTK_ENTRY (input), 1);
  gtk_box_pack_start_defaults(GTK_BOX(GTK_DIALOG(dialog)->vbox), input);
  gtk_widget_grab_focus (input);
  gtk_widget_show_all(dialog);

/*
  gtk_im_context_set_client_window(imctx, GTK_WIDGET(dialog)->window);

  g_signal_connect( G_OBJECT(dialog), "focus-in-event",
          G_CALLBACK(ctrl_dialog_focus), imctx);
  g_signal_connect( G_OBJECT(dialog), "focus-out-event",
          G_CALLBACK(ctrl_dialog_focus), imctx);
*/

  switch (gtk_dialog_run(GTK_DIALOG(dialog))) {
    case GTK_RESPONSE_OK:
      text = g_strdup (gtk_entry_get_text (GTK_ENTRY (input)));
      g_debug ("%s - %s",__FUNCTION__ , text);
	  if (strlen (text) > 1) {
	      text[1] = '\0';
	  }
      break;
    default:
      break;
  }


  gtk_widget_hide(dialog);
  gtk_widget_destroy(dialog);
/*
  gtk_im_context_focus_out(imctx);
  gtk_object_unref(GTK_OBJECT(imctx));
*/
  if (text != NULL) {
    g_debug ("text != NULL");
    terminal_widget_send_ctrl_key(TERMINAL_WIDGET(terminal_window_get_widget(window)),
		    text);
    g_free (text);
  }

//  g_free(data);

}
