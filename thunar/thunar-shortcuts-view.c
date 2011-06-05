/* vi:set et ai sw=2 sts=2 ts=2: */
/*-
 * Copyright (c) 2005-2007 Benedikt Meurer <benny@xfce.org>
 * Copyright (c) 2009-2011 Jannis Pohlmann <jannis@xfce.org>
 *
 * This program is free software; you can redistribute it and/or 
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of 
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the 
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public 
 * License along with this program; if not, write to the Free 
 * Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef HAVE_MEMORY_H
#include <memory.h>
#endif
#ifdef HAVE_STRING_H
#include <string.h>
#endif

#ifdef HAVE_LIBNOTIFY
#include <thunar/thunar-notify.h>
#endif
#include <thunar/thunar-application.h>
#include <thunar/thunar-browser.h>
#include <thunar/thunar-dialogs.h>
#include <thunar/thunar-dnd.h>
#include <thunar/thunar-gio-extensions.h>
#include <thunar/thunar-gtk-extensions.h>
#include <thunar/thunar-preferences.h>
#include <thunar/thunar-private.h>
#include <thunar/thunar-shortcuts-model.h>
#include <thunar/thunar-shortcuts-view.h>



/* identifiers for properties */
enum
{
  PROP_0,
  PROP_MODEL,
};



/* identifiers for signals */
enum
{
  SHORTCUT_ACTIVATED,
  SHORTCUT_DISCONNECT,
  SHORTCUT_DISCONNECT_CANCELED,
  LAST_SIGNAL,
};



static void thunar_shortcuts_view_finalize     (GObject      *object);
static void thunar_shortcuts_view_get_property (GObject      *object,
                                                guint         prop_id,
                                                GValue       *value,
                                                GParamSpec   *pspec);
static void thunar_shortcuts_view_set_property (GObject      *object,
                                                guint         prop_id,
                                                const GValue *value,
                                                GParamSpec   *pspec);



struct _ThunarShortcutsViewClass
{
  GtkEventBoxClass __parent__;
};

struct _ThunarShortcutsView
{
  GtkEventBox           __parent__;

  ThunarShortcutsModel *model;
};



static guint view_signals[LAST_SIGNAL] G_GNUC_UNUSED;



G_DEFINE_TYPE_WITH_CODE (ThunarShortcutsView, thunar_shortcuts_view, GTK_TYPE_EVENT_BOX,
                         G_IMPLEMENT_INTERFACE (THUNAR_TYPE_BROWSER, NULL))



static void
thunar_shortcuts_view_class_init (ThunarShortcutsViewClass *klass)
{
  GObjectClass   *gobject_class;

  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->finalize = thunar_shortcuts_view_finalize;
  gobject_class->get_property = thunar_shortcuts_view_get_property;
  gobject_class->set_property = thunar_shortcuts_view_set_property;

  /**
   * ThunarShortcutsView:model:
   *
   * The #ThunarShortcutsModel associated with this view.
   **/
  g_object_class_install_property (gobject_class,
                                   PROP_MODEL,
                                   g_param_spec_object ("model",
                                                        "model",
                                                        "model",
                                                        THUNAR_TYPE_SHORTCUTS_MODEL,
                                                        EXO_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT));

#if 0
  /**
   * ThunarShortcutsView:row-activated:
   *
   * Invoked whenever a shortcut is activated by the user.
   **/
  view_signals[SHORTCUT_ACTIVATED] =
    g_signal_new (I_("shortcut-activated"),
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_LAST,
                  0, NULL, NULL,
                  g_cclosure_marshal_VOID__OBJECT,
                  G_TYPE_NONE, 1, THUNAR_TYPE_SHORTCUT);

  /**
   * ThunarShortcutsView:shortcut-disconnect:
   *
   * Invoked whenever the user wishes to disconnect or eject a shortcut.
   **/
  view_signals[SHORTCUT_DISCONNECT] =
    g_signal_new (I_("shortcut-disconnect"),
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_LAST,
                  0, NULL, NULL,
                  g_cclosure_marshal_VOID__OBJECT,
                  G_TYPE_NONE, 1, THUNAR_TYPE_SHORTCUT);

  /**
   * ThunarShortcutsView:shortcut-disconnect-canceled:
   *
   * Invoked whenever the disconnect or eject action of a shortcut is
   * canceled by the user.
   **/
  view_signals[SHORTCUT_DISCONNECT_CANCELED] =
    g_signal_new (I_("shortcut-disconnect-canceled"),
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_LAST,
                  0, NULL, NULL,
                  g_cclosure_marshal_VOID__OBJECT,
                  G_TYPE_NONE, 1, THUNAR_TYPE_SHORTCUT);
#endif
}



static void
thunar_shortcuts_view_init (ThunarShortcutsView *view)
{
  view->model = NULL;
}



static void
thunar_shortcuts_view_finalize (GObject *object)
{
  ThunarShortcutsView *view = THUNAR_SHORTCUTS_VIEW (object);

  /* release the shortcuts model */
  if (view->model != NULL)
    g_object_unref (view->model);

  (*G_OBJECT_CLASS (thunar_shortcuts_view_parent_class)->finalize) (object);
}



static void
thunar_shortcuts_view_get_property (GObject    *object,
                                    guint       prop_id,
                                    GValue     *value,
                                    GParamSpec *pspec)
{
  ThunarShortcutsView *view = THUNAR_SHORTCUTS_VIEW (object);

  switch (prop_id)
    {
    case PROP_MODEL:
      g_value_set_object (value, view->model);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}



static void
thunar_shortcuts_view_set_property (GObject      *object,
                                    guint         prop_id,
                                    const GValue *value,
                                    GParamSpec   *pspec)
{
  ThunarShortcutsView *view = THUNAR_SHORTCUTS_VIEW (object);

  switch (prop_id)
    {
    case PROP_MODEL:
      view->model = g_value_dup_object (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}



/**
 * thunar_shortcuts_view_new:
 *
 * Allocates a new #ThunarShortcutsView instance and associates
 * it with the default #ThunarShortcutsModel instance.
 *
 * Return value: the newly allocated #ThunarShortcutsView instance.
 **/
GtkWidget*
thunar_shortcuts_view_new (void)
{
  ThunarShortcutsModel *model;
  GtkWidget            *view;

  model = thunar_shortcuts_model_get_default ();
  view = g_object_new (THUNAR_TYPE_SHORTCUTS_VIEW, "model", model, NULL);
  g_object_unref (G_OBJECT (model));

  return view;
}



/**
 * thunar_shortcuts_view_select_by_file:
 * @view : a #ThunarShortcutsView instance.
 * @file : a #ThunarFile instance.
 *
 * Looks up the first shortcut that refers to @file in @view and selects it.
 * If @file is not present in the underlying #ThunarShortcutsModel, no
 * shortcut will be selected afterwards.
 **/
void
thunar_shortcuts_view_select_by_file (ThunarShortcutsView *view,
                                      ThunarFile          *file)
{
  _thunar_return_if_fail (THUNAR_IS_SHORTCUTS_VIEW (view));
  _thunar_return_if_fail (THUNAR_IS_FILE (file));
}


