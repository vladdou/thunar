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
#include <thunar/thunar-shortcut-row.h>
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



static void       thunar_shortcuts_view_constructed     (GObject             *object);
static void       thunar_shortcuts_view_finalize        (GObject             *object);
static void       thunar_shortcuts_view_get_property    (GObject             *object,
                                                         guint                prop_id,
                                                         GValue              *value,
                                                         GParamSpec          *pspec);
static void       thunar_shortcuts_view_set_property    (GObject             *object,
                                                         guint                prop_id,
                                                         const GValue        *value,
                                                         GParamSpec          *pspec);
static void       thunar_shortcuts_view_row_inserted    (ThunarShortcutsView *view,
                                                         GtkTreePath         *path,
                                                         GtkTreeIter         *iter,
                                                         GtkTreeModel        *model);
static GtkWidget *thunar_shortcuts_view_get_expander_at (ThunarShortcutsView *view, 
                                                         gint                 index);



struct _ThunarShortcutsViewClass
{
  GtkEventBoxClass __parent__;
};

struct _ThunarShortcutsView
{
  GtkEventBox   __parent__;

  GtkTreeModel *model;
  GtkWidget    *expander_box;
};



static guint view_signals[LAST_SIGNAL] G_GNUC_UNUSED;



G_DEFINE_TYPE_WITH_CODE (ThunarShortcutsView, thunar_shortcuts_view, GTK_TYPE_EVENT_BOX,
                         G_IMPLEMENT_INTERFACE (THUNAR_TYPE_BROWSER, NULL))



static void
thunar_shortcuts_view_class_init (ThunarShortcutsViewClass *klass)
{
  GObjectClass   *gobject_class;

  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->constructed = thunar_shortcuts_view_constructed;
  gobject_class->finalize = thunar_shortcuts_view_finalize;
  gobject_class->get_property = thunar_shortcuts_view_get_property;
  gobject_class->set_property = thunar_shortcuts_view_set_property;

  /**
   * ThunarShortcutsView:model:
   *
   * The #GtkTreeModel associated with this view.
   **/
  g_object_class_install_property (gobject_class,
                                   PROP_MODEL,
                                   g_param_spec_object ("model",
                                                        "model",
                                                        "model",
                                                        GTK_TYPE_TREE_MODEL,
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
  GtkWidget *alignment;

  view->model = NULL;

  alignment = gtk_alignment_new (0.0f, 0.0f, 1.0f, 1.0f);
  gtk_alignment_set_padding (GTK_ALIGNMENT (alignment), 4, 4, 0, 0);
  gtk_container_add (GTK_CONTAINER (view), alignment);
  gtk_widget_show (alignment);

  view->expander_box = gtk_vbox_new (FALSE, 6);
  gtk_container_add (GTK_CONTAINER (alignment), view->expander_box);
  gtk_widget_show (view->expander_box);
}



static void
thunar_shortcuts_view_constructed (GObject *object)
{
  ThunarShortcutsView *view = THUNAR_SHORTCUTS_VIEW (object);
  GtkTreeIter          iter;
  const gchar         *markup_format = "<span size='medium' weight='bold' color='#353535'>%s</span>";
  GtkWidget           *box;
  GtkWidget           *expander;
  gboolean             category;
  gboolean             valid_iter = FALSE;
  gchar               *markup;
  gchar               *name;

  /* chain up to the parent class */
  (*G_OBJECT_CLASS (thunar_shortcuts_view_parent_class)->constructed) (object);

  /* do nothing if we don't have a model set */
  if (view->model == NULL)
    return;

  /* iterate over all items in the shortcuts model */
  valid_iter = gtk_tree_model_get_iter_first (view->model, &iter);
  while (valid_iter)
    {
      /* TODO read values from the row and create an expander, 
       * shortcut row or drop placeholder, depending on the 
       * row values */
      gtk_tree_model_get (view->model, &iter,
                          THUNAR_SHORTCUTS_MODEL_COLUMN_CATEGORY, &category,
                          THUNAR_SHORTCUTS_MODEL_COLUMN_NAME, &name,
                          -1);

      if (category)
        {
          /* create the category expander */
          expander = gtk_expander_new (NULL);
          markup = g_markup_printf_escaped (markup_format, name);
          gtk_expander_set_label (GTK_EXPANDER (expander), markup);
          g_free (markup);
          gtk_expander_set_use_markup (GTK_EXPANDER (expander), TRUE);
          gtk_expander_set_expanded (GTK_EXPANDER (expander), TRUE);
          gtk_container_set_border_width (GTK_CONTAINER (expander), 0);
          gtk_expander_set_spacing (GTK_EXPANDER (expander), 0);

          /* add it to the expander box and show it */
          gtk_box_pack_start (GTK_BOX (view->expander_box), expander, FALSE, TRUE, 0);
          gtk_widget_show (expander);

          /* create a box for the shortcuts of the category */
          box = gtk_vbox_new (FALSE, 0);
          gtk_container_add (GTK_CONTAINER (expander), box);
          gtk_widget_show (box);
        }
      else
        {
          /* we will have to implement this at some point but right now, the
           * model data is loaded in an idle handler, so there is no data other
           * than categories during construction */
          _thunar_assert_not_reached ();
        }

      /* advance to the next row */
      valid_iter = gtk_tree_model_iter_next (view->model, &iter);
    }

  /* be notified when a new shortcut is added to the model */
  g_signal_connect_swapped (view->model, "row-inserted",
                            G_CALLBACK (thunar_shortcuts_view_row_inserted), view);
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



static void
thunar_shortcuts_view_row_inserted (ThunarShortcutsView *view,
                                    GtkTreePath         *path,
                                    GtkTreeIter         *iter,
                                    GtkTreeModel        *model)
{
  GtkWidget *box;
  GtkWidget *expander;
  GtkWidget *shortcut_row;
  gboolean   category;
  gboolean   visible;
  GVolume   *volume;
  GFile     *file;
  GIcon     *eject_icon;
  GIcon     *icon;
  gchar     *name;
  gint       category_index;
  gint       shortcut_index;

  _thunar_return_if_fail (THUNAR_IS_SHORTCUTS_VIEW (view));
  _thunar_return_if_fail (path != NULL);
  _thunar_return_if_fail (iter != NULL);
  _thunar_return_if_fail (GTK_IS_TREE_MODEL (model) && model == view->model);

  /* read information from the new row */
  gtk_tree_model_get (model, iter,
                      THUNAR_SHORTCUTS_MODEL_COLUMN_CATEGORY, &category,
                      THUNAR_SHORTCUTS_MODEL_COLUMN_ICON, &icon,
                      THUNAR_SHORTCUTS_MODEL_COLUMN_NAME, &name,
                      THUNAR_SHORTCUTS_MODEL_COLUMN_FILE, &file,
                      THUNAR_SHORTCUTS_MODEL_COLUMN_VOLUME, &volume,
                      THUNAR_SHORTCUTS_MODEL_COLUMN_EJECT_ICON, &eject_icon,
                      THUNAR_SHORTCUTS_MODEL_COLUMN_VISIBLE, &visible,
                      -1);

  if (category)
    {
      /* we will have to implement this at some point but right now the
       * categories are hard-coded and will not change */
      _thunar_assert_not_reached ();
    }
  else
    {
      /* create a row widget for the shortcut */
      shortcut_row = thunar_shortcut_row_new (icon, name, eject_icon);
      thunar_shortcut_row_set_file (THUNAR_SHORTCUT_ROW (shortcut_row), file);
      thunar_shortcut_row_set_volume (THUNAR_SHORTCUT_ROW (shortcut_row), volume);

      /* get the category and shortcut index */
      category_index = gtk_tree_path_get_indices (path)[0];
      shortcut_index = gtk_tree_path_get_indices (path)[1];

      /* find the expander for the row widget */
      expander = thunar_shortcuts_view_get_expander_at (view, category_index);
      
      /* if this fails then we are out of sync with the model */
      g_assert (expander != NULL);

      /* get the box widget for placing the row */
      box = gtk_bin_get_child (GTK_BIN (expander));

      /* add the new row to the box and show it */
      gtk_container_add (GTK_CONTAINER (box), shortcut_row);

      /* move the row to the correct location */
      gtk_box_reorder_child (GTK_BOX (box), shortcut_row, shortcut_index);
      
      /* show the row now (unless it was hidden by the user) */
      if (visible)
        gtk_widget_show (shortcut_row);
    }
}



static GtkWidget *
thunar_shortcuts_view_get_expander_at (ThunarShortcutsView *view, 
                                       gint                 expander_index)
{
  GtkWidget *expander = NULL;
  GList     *expanders;
  GList     *lp = NULL;

  _thunar_return_val_if_fail (THUNAR_IS_SHORTCUTS_VIEW (view), NULL);
  _thunar_return_val_if_fail (expander_index >= 0, NULL);

  expanders = gtk_container_get_children (GTK_CONTAINER (view->expander_box));
  
  lp = g_list_nth (expanders, expander_index);
  if (lp != NULL)
    expander = lp->data;

  g_list_free (expanders);

  return expander;
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



gboolean
thunar_shortcuts_view_has_file (ThunarShortcutsView *view,
                                ThunarFile          *file)
{
  _thunar_return_val_if_fail (THUNAR_IS_SHORTCUTS_VIEW (view), FALSE);
  _thunar_return_val_if_fail (THUNAR_IS_FILE (file), FALSE);

  return FALSE;
}



void
thunar_shortcuts_view_add_file (ThunarShortcutsView *view,
                                ThunarFile          *file)
{
  _thunar_return_if_fail (THUNAR_IS_SHORTCUTS_VIEW (view));
  _thunar_return_if_fail (THUNAR_IS_FILE (file));
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


