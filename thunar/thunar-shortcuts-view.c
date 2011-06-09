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
#include <thunar/thunar-file.h>
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



typedef void (*ThunarShortcutsViewForeachRowFunc) (ThunarShortcutsView *view,
                                                   ThunarShortcutRow   *row,
                                                   gpointer             user_data);



static void               thunar_shortcuts_view_constructed              (GObject                          *object);
static void               thunar_shortcuts_view_finalize                 (GObject                          *object);
static void               thunar_shortcuts_view_get_property             (GObject                          *object,
                                                                          guint                             prop_id,
                                                                          GValue                           *value,
                                                                          GParamSpec                       *pspec);
static void               thunar_shortcuts_view_set_property             (GObject                          *object,
                                                                          guint                             prop_id,
                                                                          const GValue                     *value,
                                                                          GParamSpec                       *pspec);
static void               thunar_shortcuts_view_row_inserted             (ThunarShortcutsView              *view,
                                                                          GtkTreePath                      *path,
                                                                          GtkTreeIter                      *iter,
                                                                          GtkTreeModel                     *model);
static void               thunar_shortcuts_view_row_deleted              (ThunarShortcutsView              *view,
                                                                          GtkTreePath                      *path,
                                                                          GtkTreeModel                     *model);
static void               thunar_shortcuts_view_row_changed              (ThunarShortcutsView              *view,
                                                                          GtkTreePath                      *path,
                                                                          GtkTreeIter                      *iter,
                                                                          GtkTreeModel                     *model);
static GtkWidget *        thunar_shortcuts_view_get_expander_at          (ThunarShortcutsView              *view,
                                                                          gint                              index);
static void               thunar_shortcuts_view_row_activated            (ThunarShortcutsView              *view,
                                                                          ThunarFile                       *file,
                                                                          gboolean                          open_in_new_window,
                                                                          ThunarShortcutRow                *row);
static void               thunar_shortcuts_view_row_state_changed        (ThunarShortcutsView              *view,
                                                                          GtkStateType                      previous_state,
                                                                          ThunarShortcutRow                *row);
static gboolean           thunar_shortcuts_view_row_context_menu         (ThunarShortcutsView              *view,
                                                                          GtkWidget                        *widget);
static void               thunar_shortcuts_view_row_open                 (ThunarShortcutsView              *view);
static void               thunar_shortcuts_view_row_open_new_window      (ThunarShortcutsView              *view);
static void               thunar_shortcuts_view_open                     (ThunarShortcutsView              *view,
                                                                          ThunarFile                       *file,
                                                                          gboolean                          new_window);
static ThunarShortcutRow *thunar_shortcuts_view_get_selected_row         (ThunarShortcutsView              *view);
static void               thunar_shortcuts_view_find_selected_row        (ThunarShortcutsView              *view,
                                                                          ThunarShortcutRow                *row,
                                                                          gpointer                          user_data);
static void               thunar_shortcuts_view_foreach_row              (ThunarShortcutsView              *view,
                                                                          ThunarShortcutsViewForeachRowFunc func,
                                                                          gpointer                          user_data);
static void               thunar_shortcuts_view_unselect_rows            (ThunarShortcutsView              *view,
                                                                          ThunarShortcutRow                *row,
                                                                          gpointer                          user_data);
static void               thunar_shortcuts_view_unprelight_rows          (ThunarShortcutsView              *view,
                                                                          ThunarShortcutRow                *row,
                                                                          gpointer                          user_data);
static void               thunar_shortcuts_view_update_selection_by_file (ThunarShortcutsView              *view,
                                                                          ThunarShortcutRow                *row,
                                                                          gpointer                          user_data);



struct _ThunarShortcutsViewClass
{
  GtkEventBoxClass __parent__;
};

struct _ThunarShortcutsView
{
  GtkEventBox             __parent__;

  ThunarxProviderFactory *provider_factory;

  GtkTreeModel           *model;
  GtkWidget              *expander_box;
};



static guint view_signals[LAST_SIGNAL];



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
                  G_TYPE_NONE, 1, THUNAR_TYPE_FILE);

#if 0
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

  /* grab a reference on the provider factory */
  view->provider_factory = thunarx_provider_factory_get_default ();

  alignment = gtk_alignment_new (0.0f, 0.0f, 1.0f, 1.0f);
  gtk_alignment_set_padding (GTK_ALIGNMENT (alignment), 4, 4, 0, 0);
  gtk_container_add (GTK_CONTAINER (view), alignment);
  gtk_widget_show (alignment);

  view->expander_box = gtk_vbox_new (FALSE, 10);
  gtk_container_add (GTK_CONTAINER (alignment), view->expander_box);
  gtk_widget_show (view->expander_box);
}



static void
thunar_shortcuts_view_constructed (GObject *object)
{
  ThunarShortcutsView *view = THUNAR_SHORTCUTS_VIEW (object);
  GtkTreeIter          iter;
  GtkTreeIter          child_iter;
  GtkTreePath         *path;
  const gchar         *markup_format = "<span size='medium' weight='bold' color='#353535'>%s</span>";
  GtkWidget           *box;
  GtkWidget           *expander;
  gboolean             category;
  gboolean             valid_iter = FALSE;
  gboolean             valid_child = FALSE;
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
      /* read category information from the model */
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
          box = gtk_vbox_new (TRUE, 0);
          gtk_container_add (GTK_CONTAINER (expander), box);
          gtk_widget_show (box);

          /* check if the categories has any entries */
          valid_child = gtk_tree_model_iter_children (view->model, &child_iter, &iter);
          while (valid_child)
            {
              /* create a tree path for the shortcut */
              path = gtk_tree_model_get_path (view->model, &child_iter);

              /* add the shortcut */
              thunar_shortcuts_view_row_inserted (view, path, &child_iter, view->model);

              /* release the path */
              gtk_tree_path_free (path);

              /* advance to the next child row */
              valid_child = gtk_tree_model_iter_next (view->model, &child_iter);
            }
        }

      /* advance to the next row */
      valid_iter = gtk_tree_model_iter_next (view->model, &iter);
    }

  /* be notified when a new shortcut is added to the model */
  g_signal_connect_swapped (view->model, "row-inserted",
                            G_CALLBACK (thunar_shortcuts_view_row_inserted), view);

  /* be notified when a shortcut is removed from the model */
  g_signal_connect_swapped (view->model, "row-deleted",
                            G_CALLBACK (thunar_shortcuts_view_row_deleted), view);

  /* be notified when a shortcut changes */
  g_signal_connect_swapped (view->model, "row-changed",
                            G_CALLBACK (thunar_shortcuts_view_row_changed), view);
}



static void
thunar_shortcuts_view_finalize (GObject *object)
{
  ThunarShortcutsView *view = THUNAR_SHORTCUTS_VIEW (object);

  /* release the provider factory */
  g_object_unref (G_OBJECT (view->provider_factory));

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
  ThunarShortcutType shortcut_type;
  GtkWidget         *box;
  GtkWidget         *expander;
  GtkWidget         *shortcut_row;
  gboolean           category;
  gboolean           visible;
  GVolume           *volume;
  GMount            *mount;
  GFile             *location;
  GIcon             *eject_icon;
  GIcon             *icon;
  gchar             *name;
  gint               category_index;
  gint               shortcut_index;

  _thunar_return_if_fail (THUNAR_IS_SHORTCUTS_VIEW (view));
  _thunar_return_if_fail (path != NULL);
  _thunar_return_if_fail (iter != NULL);
  _thunar_return_if_fail (GTK_IS_TREE_MODEL (model) && model == view->model);

  /* read information from the new row */
  gtk_tree_model_get (model, iter,
                      THUNAR_SHORTCUTS_MODEL_COLUMN_CATEGORY, &category,
                      THUNAR_SHORTCUTS_MODEL_COLUMN_ICON, &icon,
                      THUNAR_SHORTCUTS_MODEL_COLUMN_NAME, &name,
                      THUNAR_SHORTCUTS_MODEL_COLUMN_LOCATION, &location,
                      THUNAR_SHORTCUTS_MODEL_COLUMN_VOLUME, &volume,
                      THUNAR_SHORTCUTS_MODEL_COLUMN_MOUNT, &mount,
                      THUNAR_SHORTCUTS_MODEL_COLUMN_EJECT_ICON, &eject_icon,
                      THUNAR_SHORTCUTS_MODEL_COLUMN_VISIBLE, &visible,
                      THUNAR_SHORTCUTS_MODEL_COLUMN_SHORTCUT_TYPE, &shortcut_type,
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
      shortcut_row = g_object_new (THUNAR_TYPE_SHORTCUT_ROW,
                                   "location", location,
                                   "volume", volume,
                                   "mount", mount,
                                   "shortcut-type", shortcut_type,
                                   "icon", icon,
                                   "eject-icon", eject_icon,
                                   "label", name,
                                   NULL);

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
      gtk_widget_set_visible (shortcut_row, visible);

      /* be notified when the user wishes to open the shortcut */
      g_signal_connect_swapped (shortcut_row, "activated",
                                G_CALLBACK (thunar_shortcuts_view_row_activated), view);

      /* be notified when the state of the row changes (e.g. when it is
       * being hovered or selected by the user) */
      g_signal_connect_swapped (shortcut_row, "state-changed",
                                G_CALLBACK (thunar_shortcuts_view_row_state_changed),
                                view);

      /* be notified when a context menu should be displayed for the row */
      g_signal_connect_swapped (shortcut_row, "context-menu",
                                G_CALLBACK (thunar_shortcuts_view_row_context_menu),
                                view);
    }
}



static void
thunar_shortcuts_view_row_deleted (ThunarShortcutsView *view,
                                   GtkTreePath         *path,
                                   GtkTreeModel        *model)
{
  GtkWidget *expander;
  GtkWidget *box;
  GList     *rows;
  GList     *row_element;
  gint       category_index;
  gint       shortcut_index;

  _thunar_return_if_fail (THUNAR_SHORTCUTS_VIEW (view));
  _thunar_return_if_fail (GTK_IS_TREE_MODEL (model));


  /* get the category and shortcut index */
  category_index = gtk_tree_path_get_indices (path)[0];
  shortcut_index = gtk_tree_path_get_indices (path)[1];

  /* find the expander for the row widget */
  expander = thunar_shortcuts_view_get_expander_at (view, category_index);

  /* if this fails then we are out of sync with the model */
  g_assert (expander != NULL);

  /* get the box widget that holds the shortcut row */
  box = gtk_bin_get_child (GTK_BIN (expander));

  /* get a list of all shortcut rows */
  rows = gtk_container_get_children (GTK_CONTAINER (box));

  /* get the shortcut row we want to remove */
  row_element = g_list_nth (rows, shortcut_index);

  /* remove the shortcut row */
  gtk_container_remove (GTK_CONTAINER (box), row_element->data);

  /* free the row list */
  g_list_free (rows);
}



static void
thunar_shortcuts_view_row_changed (ThunarShortcutsView *view,
                                   GtkTreePath         *path,
                                   GtkTreeIter         *iter,
                                   GtkTreeModel        *model)
{
  ThunarShortcutRow *row;
  ThunarFile        *file;
  GtkWidget         *expander;
  GtkWidget         *box;
  GIcon             *icon;
  GList             *rows;
  GList             *row_element;
  gchar             *name;
  gint               category_index;
  gint               shortcut_index;

  _thunar_return_if_fail (THUNAR_SHORTCUTS_VIEW (view));
  _thunar_return_if_fail (GTK_IS_TREE_MODEL (model));

  /* get the category and shortcut index */
  category_index = gtk_tree_path_get_indices (path)[0];
  shortcut_index = gtk_tree_path_get_indices (path)[1];

  /* find the expander for the row widget */
  expander = thunar_shortcuts_view_get_expander_at (view, category_index);

  /* if this fails then we are out of sync with the model */
  g_assert (expander != NULL);

  /* get the box widget that holds the shortcut row */
  box = gtk_bin_get_child (GTK_BIN (expander));

  /* get a list of all shortcut rows */
  rows = gtk_container_get_children (GTK_CONTAINER (box));

  /* get the shortcut row we want to update */
  row_element = g_list_nth (rows, shortcut_index);
  if (row_element != NULL)
    {
      /* cast so that we have a proper row object to work with */
      row = THUNAR_SHORTCUT_ROW (row_element->data);

      /* read updated information from the tree row */
      gtk_tree_model_get (model, iter,
                          THUNAR_SHORTCUTS_MODEL_COLUMN_NAME, &name,
                          THUNAR_SHORTCUTS_MODEL_COLUMN_ICON, &icon,
                          THUNAR_SHORTCUTS_MODEL_COLUMN_FILE, &file,
                          -1);

      /* update the row */
      g_object_set (row, "label", name, "icon", icon, "file", file, NULL);

      /* release the values */
      g_free (name);
      if (icon != NULL)
        g_object_unref (icon);
      if (file != NULL)
        g_object_unref (file);
    }

  /* free the row list */
  g_list_free (rows);
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



static void
thunar_shortcuts_view_row_activated (ThunarShortcutsView *view,
                                     ThunarFile          *file,
                                     gboolean             open_in_new_window,
                                     ThunarShortcutRow   *row)
{
  _thunar_return_if_fail (THUNAR_IS_SHORTCUTS_VIEW (view));
  _thunar_return_if_fail (THUNAR_IS_FILE (file));
  _thunar_return_if_fail (THUNAR_IS_SHORTCUT_ROW (row));

  thunar_shortcuts_view_open (view, file, open_in_new_window);
}



static void
thunar_shortcuts_view_row_state_changed (ThunarShortcutsView *view,
                                         GtkStateType         previous_state,
                                         ThunarShortcutRow   *row)
{
  _thunar_return_if_fail (THUNAR_IS_SHORTCUTS_VIEW (view));
  _thunar_return_if_fail (THUNAR_IS_SHORTCUT_ROW (row));

  /* check if the row has been selected or highlighted */
  if (gtk_widget_get_state (GTK_WIDGET (row)) == GTK_STATE_SELECTED)
    {
      /* unselect all other rows */
      thunar_shortcuts_view_foreach_row (view, thunar_shortcuts_view_unselect_rows, row);
    }
  else if (gtk_widget_get_state (GTK_WIDGET (row)) == GTK_STATE_PRELIGHT)
    {
      /* unprelight all other rows */
      thunar_shortcuts_view_foreach_row (view, thunar_shortcuts_view_unprelight_rows,
                                         row);
    }
}



static gboolean
thunar_shortcuts_view_row_context_menu (ThunarShortcutsView *view,
                                        GtkWidget           *widget)
{
  ThunarShortcutType shortcut_type;
  ThunarShortcutRow *row = THUNAR_SHORTCUT_ROW (widget);
  ThunarFile        *file;
  GtkWidget         *image;
  GtkWidget         *item;
  GtkWidget         *menu;
  GtkWidget         *window;
  GVolume           *volume;
  GList             *lp;
  GList             *providers;
  GList             *actions = NULL;
  GList             *tmp;

  _thunar_return_val_if_fail (THUNAR_IS_SHORTCUTS_VIEW (view), FALSE);
  _thunar_return_val_if_fail (THUNAR_IS_SHORTCUT_ROW (row), FALSE);

  /* prepare the popup menu */
  menu = gtk_menu_new ();

  /* append the "Open" menu action */
  item = gtk_image_menu_item_new_with_mnemonic (_("_Open"));
  g_signal_connect_swapped (item, "activate",
                            G_CALLBACK (thunar_shortcuts_view_row_open), view);
  gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
  gtk_widget_show (item);

  /* set the stock icon */
  image = gtk_image_new_from_stock (GTK_STOCK_OPEN, GTK_ICON_SIZE_MENU);
  gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (item), image);
  gtk_widget_show (image);

  /* append the "Open in New Window" menu action */
  item = gtk_image_menu_item_new_with_mnemonic (_("Open in New Window"));
  g_signal_connect_swapped (item, "activate",
                            G_CALLBACK (thunar_shortcuts_view_row_open_new_window),
                            view);
  gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
  gtk_widget_show (item);

  /* append a menu separator */
  item = gtk_separator_menu_item_new ();
  gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
  gtk_widget_show (item);

  /* determine the type of the row */
  shortcut_type = thunar_shortcut_row_get_shortcut_type (row);

  /* check if we are dealing with a mount */
  if (shortcut_type == THUNAR_SHORTCUT_STANDALONE_MOUNT)
    {
      /* append the "Disconnect" item */
      item = gtk_image_menu_item_new_with_mnemonic (_("Disconn_ect"));
      gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
      gtk_widget_show (item);

      /* append a menu separator */
      item = gtk_separator_menu_item_new ();
      gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
      gtk_widget_show (item);
    }

  /* check if we're dealing with a volume */
  if (shortcut_type == THUNAR_SHORTCUT_REGULAR_VOLUME
      || shortcut_type == THUNAR_SHORTCUT_EJECTABLE_VOLUME)
    {
      /* get the volume from the shortcut row */
      volume = thunar_shortcut_row_get_volume (row);
      
      /* check if we have a mounted volume */
      /* append the "Mount" item */
      item = gtk_image_menu_item_new_with_mnemonic (_("_Mount"));
      gtk_widget_set_sensitive (item, !thunar_g_volume_is_mounted (volume));
      gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
      gtk_widget_show (item);

      if (shortcut_type == THUNAR_SHORTCUT_REGULAR_VOLUME)
        {
          /* append the "Unmount" item */
          item = gtk_image_menu_item_new_with_mnemonic (_("_Unmount"));
          gtk_widget_set_sensitive (item, thunar_g_volume_is_mounted (volume));
          gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
          gtk_widget_show (item);
        }

      /* append the "Disconnect" (eject + safely remove drive) item */
      item = gtk_image_menu_item_new_with_mnemonic (_("Disconn_ect"));
      gtk_widget_set_sensitive (item, thunar_g_volume_is_mounted (volume));
      gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
      gtk_widget_show (item);
    }

  /* get the ThunarFile from the row */
  file = thunar_shortcut_row_get_file (row);

  /* check if we're dealing with the trash */
  if (shortcut_type == THUNAR_SHORTCUT_REGULAR_FILE
      && file != NULL
      && thunar_file_is_trashed (file) 
      && thunar_file_is_root (file))
    {
      /* append the "Empty Trash" menu action */
      item = gtk_image_menu_item_new_with_mnemonic (_("_Empty Trash"));
      gtk_widget_set_sensitive (item, (thunar_file_get_item_count (file) > 0));
      gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
      gtk_widget_show (item);

      /* append a menu separator */
      item = gtk_separator_menu_item_new ();
      gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
      gtk_widget_show (item);
    }

  /* create provider menu items if there is a non-trashed file */
  if (G_LIKELY (file != NULL && !thunar_file_is_trashed (file)))
    {
      /* load the menu providers from the provider factory */
      providers = thunarx_provider_factory_list_providers (view->provider_factory,
                                                           THUNARX_TYPE_MENU_PROVIDER);
      if (G_LIKELY (providers != NULL))
        {
          /* determine the toplevel window we belong to */
          window = gtk_widget_get_toplevel (GTK_WIDGET (view));

          /* load the actions offered by the menu providers */
          for (lp = providers; lp != NULL; lp = lp->next)
            {
              tmp = thunarx_menu_provider_get_folder_actions (lp->data, window,
                                                              THUNARX_FILE_INFO (file));
              actions = g_list_concat (actions, tmp);
              g_object_unref (G_OBJECT (lp->data));
            }
          g_list_free (providers);

          /* add the actions to the menu */
          for (lp = actions; lp != NULL; lp = lp->next)
            {
              item = gtk_action_create_menu_item (GTK_ACTION (lp->data));
              gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
              gtk_widget_show (item);

              /* release the reference on the action */
              g_object_unref (G_OBJECT (lp->data));
            }

          /* add a separator to the end of the menu */
          if (G_LIKELY (lp != actions))
            {
              /* append a menu separator */
              item = gtk_separator_menu_item_new ();
              gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
              gtk_widget_show (item);
            }

          /* cleanup */
          g_list_free (actions);
        }
    }

#if 0
  if (thunar_shortcut_row_get_mutable (row))
#endif
    {
      /* append the remove menu item */
      item = gtk_image_menu_item_new_with_mnemonic (_("_Remove Shortcut"));
      gtk_widget_set_sensitive (item, FALSE);
      gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
      gtk_widget_show (item);

      /* set the remove stock icon */
      image = gtk_image_new_from_stock (GTK_STOCK_REMOVE, GTK_ICON_SIZE_MENU);
      gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (item), image);
      gtk_widget_show (image);

      /* append the rename menu item */
      item = gtk_image_menu_item_new_with_mnemonic (_("Re_name Shortcut"));
      gtk_widget_set_sensitive (item, FALSE);
      gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
      gtk_widget_show (item);
    }

  /* run the menu on the view's screen (taking over the floating
   * reference on the menu) */
  thunar_gtk_menu_run (GTK_MENU (menu), GTK_WIDGET (view), NULL, NULL,
                       0, gtk_get_current_event_time ());

  return TRUE;
}



static void
thunar_shortcuts_view_row_open (ThunarShortcutsView *view)
{
  ThunarShortcutRow *row;

  _thunar_return_if_fail (THUNAR_IS_SHORTCUTS_VIEW (view));

  row = thunar_shortcuts_view_get_selected_row (view);

  if (row != NULL)
    thunar_shortcut_row_resolve_and_activate (row, FALSE);
}



static void
thunar_shortcuts_view_row_open_new_window (ThunarShortcutsView *view)
{
  ThunarShortcutRow *row;

  _thunar_return_if_fail (THUNAR_IS_SHORTCUTS_VIEW (view));

  row = thunar_shortcuts_view_get_selected_row (view);

  if (row != NULL)
    thunar_shortcut_row_resolve_and_activate (row, TRUE);
}



static ThunarShortcutRow *
thunar_shortcuts_view_get_selected_row (ThunarShortcutsView *view)
{
  ThunarShortcutRow *result = NULL;

  _thunar_return_val_if_fail (THUNAR_IS_SHORTCUTS_VIEW (view), NULL);

  thunar_shortcuts_view_foreach_row (view, thunar_shortcuts_view_find_selected_row,
                                     &result);

  return result;
}



static void
thunar_shortcuts_view_find_selected_row (ThunarShortcutsView *view,
                                         ThunarShortcutRow   *row,
                                         gpointer             user_data)
{
  ThunarShortcutRow **return_value = user_data;

  _thunar_return_if_fail (THUNAR_IS_SHORTCUTS_VIEW (view));
  _thunar_return_if_fail (THUNAR_IS_SHORTCUT_ROW (row));
  _thunar_return_if_fail (return_value != NULL);

  if (gtk_widget_get_state (GTK_WIDGET (row)) == GTK_STATE_SELECTED)
    *return_value = row;
}



static void
thunar_shortcuts_view_foreach_row (ThunarShortcutsView              *view,
                                   ThunarShortcutsViewForeachRowFunc func,
                                   gpointer                          user_data)
{
  GtkWidget *box;
  GList     *expanders;
  GList     *ep;
  GList     *rows;
  GList     *rp;

  _thunar_return_if_fail (THUNAR_IS_SHORTCUTS_VIEW (view));
  _thunar_return_if_fail (func != NULL);

  /* get a list of all expanders */
  expanders = gtk_container_get_children (GTK_CONTAINER (view->expander_box));

  /* iterate over all expanders */
  for (ep = expanders; ep != NULL; ep = ep->next)
    {
      /* get the box that holds the rows */
      box = gtk_bin_get_child (GTK_BIN (ep->data));

      /* get a list of all rows in the box */
      rows = gtk_container_get_children (GTK_CONTAINER (box));

      /* iterate over all these rows */
      for (rp = rows; rp != NULL; rp = rp->next)
        {
          /* call the foreach func */
          (func) (view, THUNAR_SHORTCUT_ROW (rp->data), user_data);
        }

      /* free the list of rows */
      g_list_free (rows);
    }

  /* free the list of expanders */
  g_list_free (expanders);
}



static void
thunar_shortcuts_view_unselect_rows (ThunarShortcutsView *view,
                                     ThunarShortcutRow   *row,
                                     gpointer             user_data)
{
  ThunarShortcutRow *selected_row = THUNAR_SHORTCUT_ROW (user_data);

  _thunar_return_if_fail (THUNAR_IS_SHORTCUTS_VIEW (view));
  _thunar_return_if_fail (THUNAR_IS_SHORTCUT_ROW (row));
  _thunar_return_if_fail (THUNAR_IS_SHORTCUT_ROW (selected_row));

  /* reset the row state if it is not the selected row */
  if (row != selected_row && 
      gtk_widget_get_state (GTK_WIDGET (row)) == GTK_STATE_SELECTED)
    {
      gtk_widget_set_state (GTK_WIDGET (row), GTK_STATE_NORMAL);
    }
}



static void
thunar_shortcuts_view_unprelight_rows (ThunarShortcutsView *view,
                                       ThunarShortcutRow   *row,
                                       gpointer             user_data)
{
  ThunarShortcutRow *selected_row = THUNAR_SHORTCUT_ROW (user_data);

  _thunar_return_if_fail (THUNAR_IS_SHORTCUTS_VIEW (view));
  _thunar_return_if_fail (THUNAR_IS_SHORTCUT_ROW (row));
  _thunar_return_if_fail (THUNAR_IS_SHORTCUT_ROW (selected_row));

  /* reset the row state if it is not the selected row */
  if (row != selected_row && 
      gtk_widget_get_state (GTK_WIDGET (row)) == GTK_STATE_PRELIGHT)
    {
      gtk_widget_set_state (GTK_WIDGET (row), GTK_STATE_NORMAL);
    }
}



static void
thunar_shortcuts_view_update_selection_by_file (ThunarShortcutsView *view,
                                                ThunarShortcutRow   *row,
                                                gpointer             user_data)
{
  ThunarFile *file = THUNAR_FILE (user_data);
  gboolean    select_row = FALSE;
  GVolume    *row_volume;
  GMount     *mount;
  GFile      *mount_point;
  GFile      *row_file;

  _thunar_return_if_fail (THUNAR_IS_SHORTCUTS_VIEW (view));
  _thunar_return_if_fail (THUNAR_IS_SHORTCUT_ROW (row));
  _thunar_return_if_fail (THUNAR_IS_FILE (file));

  /* get the file and volume of the view */
  row_file = thunar_shortcut_row_get_location (row);
  row_volume = thunar_shortcut_row_get_volume (row);

  /* check if we have a volume */
  if (row_volume != NULL)
    {
      /* get the mount point */
      mount = g_volume_get_mount (row_volume);
      if (mount != NULL)
        {
          mount_point = g_mount_get_root (mount);

          /* select the row if the mount point and the selected file are equal */
          if (g_file_equal (file->gfile, mount_point))
            select_row = TRUE;

          /* release mount point and mount */
          g_object_unref (mount_point);
          g_object_unref (mount);
        }
    }
  else if (row_file != NULL)
    {
      /* select the row if the bookmark and the selected file are equal */
      if (g_file_equal (file->gfile, row_file))
        select_row = TRUE;
    }

  /* apply the selection / unselection */
  if (select_row)
    {
      gtk_widget_set_state (GTK_WIDGET (row), GTK_STATE_SELECTED);
    }
  else if (gtk_widget_get_state (GTK_WIDGET (row)) == GTK_STATE_SELECTED)
    {
      gtk_widget_set_state (GTK_WIDGET (row), GTK_STATE_NORMAL);
    }
}



static void
thunar_shortcuts_view_open (ThunarShortcutsView *view,
                            ThunarFile          *file,
                            gboolean             new_window)
{
  ThunarApplication *application;

  _thunar_return_if_fail (THUNAR_IS_SHORTCUTS_VIEW (view));
  _thunar_return_if_fail (THUNAR_IS_FILE (file));

  if (new_window)
    {
      /* open the file in a new window */
      application = thunar_application_get ();
      thunar_application_open_window (application, file,
                                      gtk_widget_get_screen (GTK_WIDGET (view)),
                                      NULL);
      g_object_unref (application);
    }
  else
    {
      /* invoke the signal to change to the folder */
      g_signal_emit (view, view_signals[SHORTCUT_ACTIVATED], 0, file);
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

  thunar_shortcuts_view_foreach_row (view,
                                     thunar_shortcuts_view_update_selection_by_file,
                                     file);
}


