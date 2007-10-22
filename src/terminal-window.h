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
 * Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef __TERMINAL_WINDOW_H__
#define __TERMINAL_WINDOW_H__

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#if HILDON == 0
#include <hildon-widgets/hildon-window.h>
#elif HILDON == 1
#include <hildon/hildon-window.h>
#endif

#include "terminal-widget.h"

G_BEGIN_DECLS;

#define TERMINAL_TYPE_WINDOW            (terminal_window_get_type ())
#define TERMINAL_WINDOW(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), TERMINAL_TYPE_WINDOW, TerminalWindow))
#define TERMINAL_WINDOW_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), TERMINAL_TYPE_WINDOW, TerminalWindowClass))
#define TERMINAL_IS_WINDOW(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), TERMINAL_TYPE_WINDOW))
#define TERMINAL_IS_WINDOW_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), TERMINAL_TYPE_WINDOW))
#define TERMINAL_WINDOW_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), TERMINAL_TYPE_WINDOW, TerminalWindowClass))

typedef struct _TerminalWindowClass TerminalWindowClass;
typedef struct _TerminalWindow      TerminalWindow;

struct _TerminalWindowClass
{
  HildonWindowClass __parent__;

  /* signals */
  void (*new_window) (TerminalWindow *app, const gchar *command);
};

GType      terminal_window_get_type (void) G_GNUC_CONST;

GtkWidget *terminal_window_new      (void);

void       terminal_window_add      (TerminalWindow    *app,
                                     TerminalWidget *widget);

void       terminal_window_remove   (TerminalWindow *app,
                                     TerminalWidget *widget);

gboolean   terminal_window_launch (TerminalWindow     *app,
    				   const gchar     *command,
                                   GError          **error);

G_END_DECLS;

#endif /* !__TERMINAL_WINDOW_H__ */
