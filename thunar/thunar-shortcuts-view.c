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
#include <thunar/thunar-shortcut-group.h>
#include <thunar/thunar-shortcut.h>
#include <thunar/thunar-shortcuts-model.h>
#include <thunar/thunar-shortcuts-view.h>



#define THUNAR_SHORTCUTS_VIEW_FLASH_TIMEOUT 300
#define THUNAR_SHORTCUTS_VIEW_FLASH_TIMES     3



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



typedef void (*ThunarShortcutsViewForeachGroupFunc) (ThunarShortcutsView *view,
                                                     ThunarShortcutGroup *group,
                                                     gpointer             user_data);



static void               thunar_shortcuts_view_constructed              (GObject                            *object);
static void               thunar_shortcuts_view_finalize                 (GObject                            *object);
static void               thunar_shortcuts_view_get_property             (GObject                            *object,
                                                                          guint                               prop_id,
                                                                          GValue                             *value,
                                                                          GParamSpec                         *pspec);
static void               thunar_shortcuts_view_set_property             (GObject                            *object,
                                                                          guint                               prop_id,
                                                                          const GValue                       *value,
                                                                          GParamSpec                         *pspec);
static gboolean           thunar_shortcuts_view_load_system_shortcuts    (gpointer                            user_data);
static void               thunar_shortcuts_view_create_home_shortcut     (ThunarShortcutsView                *view);
static void               thunar_shortcuts_view_create_desktop_shortcut  (ThunarShortcutsView                *view);
static void               thunar_shortcuts_view_create_trash_shortcut    (ThunarShortcutsView                *view);
static void               thunar_shortcuts_view_create_network_shortcut  (ThunarShortcutsView                *view);
static gboolean           thunar_shortcuts_view_load_user_dirs           (gpointer                            user_data);
static gboolean           thunar_shortcuts_view_load_bookmarks           (gpointer                            user_data);
static gboolean           thunar_shortcuts_view_load_volumes             (gpointer                            user_data);
static void               thunar_shortcuts_view_volume_added             (ThunarShortcutsView                *view,
                                                                          GVolume                            *volume,
                                                                          GVolumeMonitor                     *monitor);
static void               thunar_shortcuts_view_mount_added              (ThunarShortcutsView                *view,
                                                                          GMount                             *mount,
                                                                          GVolumeMonitor                     *monitor);
static void               thunar_shortcuts_view_add_shortcut             (ThunarShortcutsView                *view,
                                                                          ThunarShortcut                     *shortcut);
static void               thunar_shortcuts_view_shortcut_activated       (ThunarShortcutsView                *view,
                                                                          ThunarFile                         *file,
                                                                          gboolean                            open_in_new_window,
                                                                          ThunarShortcut                     *shortcut);
static void               thunar_shortcuts_view_shortcut_state_changed   (ThunarShortcutsView                *view,
                                                                          GtkStateType                        previous_state,
                                                                          ThunarShortcut                     *shortcut);
static void               thunar_shortcuts_view_unselect_shortcuts       (ThunarShortcutsView                *view,
                                                                          ThunarShortcutGroup                *group,
                                                                          gpointer                            user_data);
static void               thunar_shortcuts_view_unprelight_shortcuts     (ThunarShortcutsView                *view,
                                                                          ThunarShortcutGroup                *group,
                                                                          gpointer                            user_data);
static void               thunar_shortcuts_view_foreach_group            (ThunarShortcutsView                *view,
                                                                          ThunarShortcutsViewForeachGroupFunc func,
                                                                          gpointer                            user_data);
static void               thunar_shortcuts_view_open                     (ThunarShortcutsView                *view,
                                                                          ThunarFile                         *file,
                                                                          gboolean                            new_window);
#if 0
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
static gboolean           thunar_shortcuts_view_flash_expander           (gpointer                          user_data);
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
static void               thunar_shortcuts_view_row_disconnect           (ThunarShortcutsView              *view);
static void               thunar_shortcuts_view_row_mount                (ThunarShortcutsView              *view);
static void               thunar_shortcuts_view_row_unmount              (ThunarShortcutsView              *view);
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
#endif
static void               thunar_shortcuts_view_update_selection         (ThunarShortcutsView              *view,
                                                                          ThunarShortcutGroup              *group,
                                                                          gpointer                          user_data);



struct _ThunarShortcutsViewClass
{
  GtkEventBoxClass __parent__;
};

struct _ThunarShortcutsView
{
  GtkEventBox             __parent__;

  ThunarxProviderFactory *provider_factory;

  GVolumeMonitor         *volume_monitor;
  GtkWidget              *group_box;

  guint                   load_idle_id;
};



static GQuark thunar_shortcuts_view_idle_quark;
static GQuark thunar_shortcuts_view_counter_quark;
static guint  view_signals[LAST_SIGNAL];



G_DEFINE_TYPE_WITH_CODE (ThunarShortcutsView, thunar_shortcuts_view, GTK_TYPE_EVENT_BOX,
                         G_IMPLEMENT_INTERFACE (THUNAR_TYPE_BROWSER, NULL))



static void
thunar_shortcuts_view_class_init (ThunarShortcutsViewClass *klass)
{
  GObjectClass   *gobject_class;

  thunar_shortcuts_view_idle_quark = 
    g_quark_from_static_string ("thunar-shortcuts-view-idle");
  thunar_shortcuts_view_counter_quark =
    g_quark_from_static_string ("thunar-shortcuts-view-counter");

  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->constructed = thunar_shortcuts_view_constructed;
  gobject_class->finalize = thunar_shortcuts_view_finalize;
  gobject_class->get_property = thunar_shortcuts_view_get_property;
  gobject_class->set_property = thunar_shortcuts_view_set_property;

  /**
   * ThunarShortcutsView:shortcut-activated:
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
  GtkWidget *group;

  /* grab a reference on the provider factory */
  view->provider_factory = thunarx_provider_factory_get_default ();

  /* add 4 pixels top and bottom padding around the contents of the shortcuts view */
  alignment = gtk_alignment_new (0.0f, 0.0f, 1.0f, 1.0f);
  gtk_alignment_set_padding (GTK_ALIGNMENT (alignment), 4, 4, 0, 0);
  gtk_container_add (GTK_CONTAINER (view), alignment);
  gtk_widget_show (alignment);

  /* create a box to put the groups of shortcuts in */
  view->group_box = gtk_vbox_new (FALSE, 10);
  gtk_container_add (GTK_CONTAINER (alignment), view->group_box);
  gtk_widget_show (view->group_box);

  /* create the devices group */
  group = thunar_shortcut_group_new (_("DEVICES"), 
                                     THUNAR_SHORTCUT_REGULAR_VOLUME 
                                     | THUNAR_SHORTCUT_EJECTABLE_VOLUME 
                                     | THUNAR_SHORTCUT_REGULAR_MOUNT
                                     | THUNAR_SHORTCUT_ARCHIVE_MOUNT);
  gtk_box_pack_start (GTK_BOX (view->group_box), group, FALSE, TRUE, 0);
  gtk_widget_show (group);

  /* create the places group */
  group = thunar_shortcut_group_new (_("PLACES"),
                                     THUNAR_SHORTCUT_REGULAR_FILE
                                     | THUNAR_SHORTCUT_TRASH_FILE);
  gtk_box_pack_start (GTK_BOX (view->group_box), group, FALSE, TRUE, 0);
  gtk_widget_show (group);

  /* create the network group */
  group = thunar_shortcut_group_new (_("NETWORK"),
                                     THUNAR_SHORTCUT_NETWORK_FILE 
                                     | THUNAR_SHORTCUT_NETWORK_MOUNT);
  gtk_box_pack_start (GTK_BOX (view->group_box), group, FALSE, TRUE, 0);
  gtk_widget_show (group);
}



static void
thunar_shortcuts_view_constructed (GObject *object)
{
  ThunarShortcutsView *view = THUNAR_SHORTCUTS_VIEW (object);

  /* load shortcuts in a series of idle handlers */
  view->load_idle_id = g_idle_add (thunar_shortcuts_view_load_system_shortcuts, view);
}



static void
thunar_shortcuts_view_finalize (GObject *object)
{
  ThunarShortcutsView *view = THUNAR_SHORTCUTS_VIEW (object);

  /* release the provider factory */
  g_object_unref (G_OBJECT (view->provider_factory));

  /* disconnect and release the volume monitor */
  g_signal_handlers_disconnect_matched (view->volume_monitor,
                                        G_SIGNAL_MATCH_DATA,
                                        0, 0, NULL, NULL, view);
  g_object_unref (view->volume_monitor);

  (*G_OBJECT_CLASS (thunar_shortcuts_view_parent_class)->finalize) (object);
}



static void
thunar_shortcuts_view_get_property (GObject    *object,
                                    guint       prop_id,
                                    GValue     *value,
                                    GParamSpec *pspec)
{
#if 0
  ThunarShortcutsView *view = THUNAR_SHORTCUTS_VIEW (object);
#endif

  switch (prop_id)
    {
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
#if 0
  ThunarShortcutsView *view = THUNAR_SHORTCUTS_VIEW (object);
#endif

  switch (prop_id)
    {
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}



#if 0
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
  GMount            *mount;
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
      g_signal_connect_swapped (item, "activate",
                                G_CALLBACK (thunar_shortcuts_view_row_disconnect),
                                view);
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
      /* get the volume and mount from the shortcut row */
      volume = thunar_shortcut_row_get_volume (row);
      mount = g_volume_get_mount (volume);
      
      /* append the "Mount" item */
      item = gtk_image_menu_item_new_with_mnemonic (_("_Mount"));
      gtk_widget_set_sensitive (item, mount == NULL && g_volume_can_mount (volume));
      g_signal_connect_swapped (item, "activate",
                                G_CALLBACK (thunar_shortcuts_view_row_mount),
                                view);
      gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
      gtk_widget_show (item);

      if (shortcut_type == THUNAR_SHORTCUT_REGULAR_VOLUME)
        {
          /* append the "Unmount" item */
          item = gtk_image_menu_item_new_with_mnemonic (_("_Unmount"));
          gtk_widget_set_sensitive (item, mount != NULL && g_mount_can_unmount (mount));
          g_signal_connect_swapped (item, "activate",
                                    G_CALLBACK (thunar_shortcuts_view_row_unmount),
                                    view);
          gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
          gtk_widget_show (item);
        }

      /* append the "Disconnect" (eject + safely remove drive) item */
      item = gtk_image_menu_item_new_with_mnemonic (_("Disconn_ect"));
      gtk_widget_set_sensitive (item, 
                                (mount != NULL && g_mount_can_eject (mount))
                                || g_volume_can_eject (volume));
      g_signal_connect_swapped (item, "activate",
                                G_CALLBACK (thunar_shortcuts_view_row_disconnect),
                                view);
      gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
      gtk_widget_show (item);

      if (mount != NULL)
        g_object_unref (mount);
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



static void
thunar_shortcuts_view_row_disconnect (ThunarShortcutsView *view)
{
  ThunarShortcutRow *row;

  _thunar_return_if_fail (THUNAR_IS_SHORTCUTS_VIEW (view));

  row = thunar_shortcuts_view_get_selected_row (view);

  if (row != NULL)
    thunar_shortcut_row_disconnect (row);
}



static void
thunar_shortcuts_view_row_mount (ThunarShortcutsView *view)
{
  ThunarShortcutRow *row;

  _thunar_return_if_fail (THUNAR_IS_SHORTCUTS_VIEW (view));

  row = thunar_shortcuts_view_get_selected_row (view);

  if (row != NULL)
    thunar_shortcut_row_mount (row);
}



static void
thunar_shortcuts_view_row_unmount (ThunarShortcutsView *view)
{
  ThunarShortcutRow *row;

  _thunar_return_if_fail (THUNAR_IS_SHORTCUTS_VIEW (view));

  row = thunar_shortcuts_view_get_selected_row (view);

  if (row != NULL)
    thunar_shortcut_row_unmount (row);
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
#endif



static gboolean
thunar_shortcuts_view_load_system_shortcuts (gpointer user_data)
{
  ThunarShortcutsView *view = THUNAR_SHORTCUTS_VIEW (user_data);

  _thunar_return_val_if_fail (THUNAR_IS_SHORTCUTS_VIEW (view), FALSE);

  thunar_shortcuts_view_create_home_shortcut (view);
  thunar_shortcuts_view_create_desktop_shortcut (view);
  thunar_shortcuts_view_create_trash_shortcut (view);
  thunar_shortcuts_view_create_network_shortcut (view);

  /* load rest of the user dirs next */
  view->load_idle_id = g_idle_add (thunar_shortcuts_view_load_user_dirs, view);

  return FALSE;
}



static void
thunar_shortcuts_view_create_home_shortcut (ThunarShortcutsView *view)
{
  ThunarShortcut       *shortcut;
  GFile                *home_file;
  GIcon                *icon;
  gchar                *path;

  _thunar_return_if_fail (THUNAR_IS_SHORTCUTS_VIEW (view));

  /* request a GFile for the home directory */
  home_file = thunar_g_file_new_for_home ();

  /* create $HOME information */
  icon = g_themed_icon_new ("user-home");
  path = g_file_get_path (home_file);
  
  /* create the $HOME shortcut */
  shortcut = g_object_new (THUNAR_TYPE_SHORTCUT,
                           "shortcut-type", THUNAR_SHORTCUT_REGULAR_FILE,
                           "location", home_file,
                           "icon", icon,
                           "custom-name", NULL,
                           "hidden", FALSE,
                           "mutable", FALSE,
                           "persistent", TRUE,
                           NULL);

  /* add the shortcut */
  thunar_shortcuts_view_add_shortcut (view, shortcut);

  /* release information */
  g_free (path);
  g_object_unref (icon);
  g_object_unref (home_file);
}



static void
thunar_shortcuts_view_create_desktop_shortcut (ThunarShortcutsView *view)
{
  ThunarShortcut       *shortcut;
  GFile                *home_file;
  GFile                *location;
  GIcon                *icon;
  gchar                *name = NULL;

  _thunar_return_if_fail (THUNAR_IS_SHORTCUTS_VIEW (view));

  /* request a GFile for the home directory */
  home_file = thunar_g_file_new_for_home ();

  /* request a GFile for the desktop directory */
  location = thunar_g_file_new_for_desktop ();

  /* check if desktop is set to home (in that case, ignore it) */
  if (!g_file_equal (home_file, location))
    {
      /* create desktop information */
      icon = g_themed_icon_new ("user-desktop");
      name = g_strdup (_("Desktop"));

      /* create the desktop shortcut */
      shortcut = g_object_new (THUNAR_TYPE_SHORTCUT,
                               "shortcut-type", THUNAR_SHORTCUT_REGULAR_FILE,
                               "location", location,
                               "icon", icon,
                               "custom-name", name,
                               "hidden", FALSE,
                               "mutable", FALSE,
                               "persistent", TRUE,
                               NULL);

      /* add the shortcut */
      thunar_shortcuts_view_add_shortcut (view, shortcut);

      /* release desktop information */
      g_free (name);
      g_object_unref (icon);
    }

  /* release desktop and home files */
  g_object_unref (location);
  g_object_unref (home_file);
}



static void
thunar_shortcuts_view_create_trash_shortcut (ThunarShortcutsView *view)
{
  ThunarShortcut       *shortcut;
  GFile                *location;
  GIcon                *icon;
  gchar                *name = NULL;

  _thunar_return_if_fail (THUNAR_IS_SHORTCUTS_VIEW (view));

  /* create trash information */
  location = thunar_g_file_new_for_trash ();
  icon = g_themed_icon_new ("user-trash");
  name = g_strdup (_("Trash"));

  /* create the trash shortcut */
  shortcut = g_object_new (THUNAR_TYPE_SHORTCUT,
                           "shortcut-type", THUNAR_SHORTCUT_TRASH_FILE,
                           "location", location,
                           "icon", icon,
                           "custom-name", name,
                           "hidden", FALSE,
                           "mutable", FALSE,
                           "persistent", TRUE,
                           NULL);

  /* add the shortcut */
  thunar_shortcuts_view_add_shortcut (view, shortcut);

  /* release trash information */
  g_free (name);
  g_object_unref (icon);
  g_object_unref (location);
}



static void
thunar_shortcuts_view_create_network_shortcut (ThunarShortcutsView *view)
{
  ThunarShortcut       *shortcut;
  GFile                *location;
  GIcon                *icon;
  gchar                *name = NULL;

  _thunar_return_if_fail (THUNAR_IS_SHORTCUTS_VIEW (view));

  /* create network information */
  /* TODO we need a new "uri" property for ThunarShortcut that is 
   * resolved into a real GFile and then into a ThunarFile 
   * asynchronously, otherwise allocating the network GFile may
   * slow down the Thunar startup */
  location = g_file_new_for_uri ("network://");
  icon = g_themed_icon_new (GTK_STOCK_NETWORK);
  name = g_strdup (_("Browse Network"));

  /* create the network shortcut */
  shortcut = g_object_new (THUNAR_TYPE_SHORTCUT,
                           "shortcut-type", THUNAR_SHORTCUT_NETWORK_FILE,
                           "location", location,
                           "icon", icon,
                           "custom-name", name,
                           "hidden", FALSE,
                           "mutable", FALSE,
                           "persistent", TRUE,
                           NULL);

  /* add the network shortcut */
  thunar_shortcuts_view_add_shortcut (view, shortcut);

  /* release network information */
  g_free (name);
  g_object_unref (icon);
  g_object_unref (location);
}



static gboolean
thunar_shortcuts_view_load_user_dirs (gpointer user_data)
{
  ThunarShortcutsView *view = THUNAR_SHORTCUTS_VIEW (user_data);

  _thunar_return_val_if_fail (THUNAR_IS_SHORTCUTS_VIEW (view), FALSE);
  
  /* load GTK bookmarks next */
  view->load_idle_id = g_idle_add (thunar_shortcuts_view_load_bookmarks, view);

  return FALSE;
}



static gboolean
thunar_shortcuts_view_load_bookmarks (gpointer user_data)
{
  ThunarShortcutsView *view = THUNAR_SHORTCUTS_VIEW (user_data);
  ThunarShortcutType   type;
  ThunarShortcut      *shortcut;
  gboolean             is_local;
  GFile               *bookmarks_file;
  GFile               *home_file;
  GFile               *location;
  GIcon               *eject_icon;
  GIcon               *icon;
  gchar               *bookmarks_path;
  gchar                line[2048];
  gchar               *name;
  FILE                *fp;

  _thunar_return_val_if_fail (THUNAR_IS_SHORTCUTS_VIEW (view), FALSE);

  /* create an eject icon */
  eject_icon = g_themed_icon_new ("media-eject");

  /* resolve the bookmarks file */
  home_file = thunar_g_file_new_for_home ();
  bookmarks_file = g_file_resolve_relative_path (home_file, ".gtk-bookmarks");
  bookmarks_path = g_file_get_path (bookmarks_file);
  g_object_unref (bookmarks_file);
  g_object_unref (home_file);

  /* TODO remove all existing bookmarks */
  
  /* open the GTK bookmarks file for reading */
  fp = fopen (bookmarks_path, "r");
  if (fp != NULL)
    {
      while (fgets (line, sizeof (line), fp) != NULL)
        {
          /* strip leading and trailing whitespace */
          g_strstrip (line);

          /* skip over the URI */
          for (name = line; *name != '\0' && !g_ascii_isspace (*name); ++name);

          /* zero-terminate the URI */
          *name++ = '\0';

          /* check if we have a name */
          for (; g_ascii_isspace (*name); ++name);

          /* check if we have something that looks like a URI */
          if (exo_str_looks_like_an_uri (line))
            {
              /* parse the URI */
              location = g_file_new_for_uri (line);

              /* only set the name property if the name is not empty */
              name = *name != '\0' ? g_strdup (name) : NULL;

              /* set initial icon and type based on the URI scheme */
              is_local = g_file_has_uri_scheme (location, "file");
              if (is_local)
                {
                  icon = g_themed_icon_new ("folder");
                  type = THUNAR_SHORTCUT_REGULAR_FILE;
                }
              else
                {
                  icon = g_themed_icon_new ("folder-remote");
                  type = THUNAR_SHORTCUT_NETWORK_FILE;
                }

              /* create the shortcut */
              shortcut = g_object_new (THUNAR_TYPE_SHORTCUT,
                                       "shortcut-type", type,
                                       "location", location,
                                       "icon", icon,
                                       "eject-icon", eject_icon,
                                       "custom-name", name,
                                       "hidden", FALSE,
                                       "mutable", TRUE,
                                       "persistent", TRUE,
                                       NULL);

              /* add the shortcut */
              thunar_shortcuts_view_add_shortcut (view, shortcut);

              /* release file information */
              g_free (name);
              g_object_unref (icon);
              g_object_unref (location);
            }
        }

      /* close the file handle */
      fclose (fp);
    }
  
  /* free the bookmarks file path */
  g_free (bookmarks_path);

  /* release the eject icon */
  g_object_unref (eject_icon);

  /* TODO monitor the bookmarks file for changes */

  /* load volumes next */
  view->load_idle_id = g_idle_add (thunar_shortcuts_view_load_volumes, view);

  return FALSE;
}



static gboolean
thunar_shortcuts_view_load_volumes (gpointer user_data)
{
  ThunarShortcutsView *view = THUNAR_SHORTCUTS_VIEW (user_data);
  GList               *mounts;
  GList               *volumes;
  GList               *lp;

  _thunar_return_val_if_fail (THUNAR_IS_SHORTCUTS_VIEW (view), FALSE);

  /* get the default volume monitor */
  view->volume_monitor = g_volume_monitor_get ();

  /* get a list of all volumes available */
  volumes = g_volume_monitor_get_volumes (view->volume_monitor);

  /* create shortcuts for the volumes */
  for (lp = volumes; lp != NULL; lp = lp->next)
    {
      /* add a new volume shortcut */
      thunar_shortcuts_view_volume_added (view, lp->data, view->volume_monitor);
    }

  /* release the volume list */
  g_list_free (volumes);

  /* get a list of all mounts available */
  mounts = g_volume_monitor_get_mounts (view->volume_monitor);

  /* create shortcuts for the mounts */
  for (lp = mounts; lp != NULL; lp = lp->next)
    {
      /* add a new mount shortcut */
      thunar_shortcuts_view_mount_added (view, lp->data, view->volume_monitor);
    }

  /* release the mount list */
  g_list_free (mounts);

  /* be notified of new and removed volumes on the system */
  g_signal_connect_swapped (view->volume_monitor, "volume-added",
                            G_CALLBACK (thunar_shortcuts_view_volume_added), view);
#if 0
  g_signal_connect_swapped (view->volume_monitor, "volume-removed",
                            G_CALLBACK (thunar_shortcuts_view_volume_removed), view);
#endif
  g_signal_connect_swapped (view->volume_monitor, "mount-added",
                            G_CALLBACK (thunar_shortcuts_view_mount_added), view);
#if 0
  g_signal_connect_swapped (view->volume_monitor, "mount-removed",
                            G_CALLBACK (thunar_shortcuts_view_mount_removed), view);
#endif

  /* reset the load idle ID */
  view->load_idle_id = 0;

  return FALSE;
};



static void
thunar_shortcuts_view_volume_added (ThunarShortcutsView *view,
                                    GVolume              *volume,
                                    GVolumeMonitor       *monitor)
{
  ThunarShortcut *shortcut;
  gboolean        hidden = FALSE;
  GIcon          *eject_icon;

  _thunar_return_if_fail (THUNAR_IS_SHORTCUTS_VIEW (view));
  _thunar_return_if_fail (G_IS_VOLUME (volume));
  _thunar_return_if_fail (G_IS_VOLUME_MONITOR (monitor));

  /* read information from the volume */
  eject_icon = g_themed_icon_new ("media-eject");

  /* hide the volume if there is no media */
  if (!thunar_g_volume_is_present (volume))
    hidden = TRUE;

  /* hide the volume if it is not removable (this can be 
   * overridden by the user in the shortcuts editor) */
  if (!thunar_g_volume_is_removable (volume))
    hidden = TRUE;

  /* create a shortcut for the volume */
  shortcut = g_object_new (THUNAR_TYPE_SHORTCUT,
                           "shortcut-type", THUNAR_SHORTCUT_REGULAR_VOLUME,
                           "volume", volume,
                           "eject-icon", eject_icon,
                           "hidden", hidden,
                           "mutable", FALSE,
                           "persistent", FALSE,
                           NULL);

  /* add the shortcut to the view */
  thunar_shortcuts_view_add_shortcut (view, shortcut);

  /* release volume information */
  g_object_unref (eject_icon);
}



static void
thunar_shortcuts_view_mount_added (ThunarShortcutsView *view,
                                    GMount               *mount,
                                    GVolumeMonitor       *monitor)
{
  ThunarShortcutType shortcut_type;
  ThunarShortcut    *shortcut;
  GVolume           *volume;
  GFile             *location;
  GIcon             *eject_icon;

  _thunar_return_if_fail (THUNAR_IS_SHORTCUTS_VIEW (view));
  _thunar_return_if_fail (G_IS_MOUNT (mount));
  _thunar_return_if_fail (G_IS_VOLUME_MONITOR (monitor));

  /* only create the shortcut if it has no volume */
  volume = g_mount_get_volume (mount);
  if (volume != NULL)
    {
      /* release the volume again */
      g_object_unref (volume);
    }
  else
    {
      /* read information from the mount */
      location = g_mount_get_root (mount);
      eject_icon = g_themed_icon_new ("media-eject");

      /* determine the shortcut type */
      if (g_file_has_uri_scheme (location, "file"))
        shortcut_type = THUNAR_SHORTCUT_REGULAR_MOUNT;
      else if (g_file_has_uri_scheme (location, "archive"))
        shortcut_type = THUNAR_SHORTCUT_ARCHIVE_MOUNT;
      else
        shortcut_type = THUNAR_SHORTCUT_NETWORK_MOUNT;

      /* create a shortcut for the mount */
      shortcut = g_object_new (THUNAR_TYPE_SHORTCUT,
                               "shortcut-type", shortcut_type,
                               "location", location,
                               "mount", mount,
                               "eject-icon", eject_icon,
                               "hidden", FALSE,
                               "mutable", FALSE,
                               "persistent", FALSE,
                               NULL);

      /* add the shortcut to the view */
      thunar_shortcuts_view_add_shortcut (view, shortcut);

      /* release volume information */
      g_object_unref (eject_icon);
      g_object_unref (location);
    }
}



static void
thunar_shortcuts_view_add_shortcut (ThunarShortcutsView *view,
                                    ThunarShortcut      *shortcut)
{
  ThunarShortcutGroup *group;
  gboolean             shortcut_inserted = FALSE;
  GList               *children;
  GList               *child;

  _thunar_return_if_fail (THUNAR_IS_SHORTCUTS_VIEW (view));
  _thunar_return_if_fail (THUNAR_IS_SHORTCUT (shortcut));

  children = gtk_container_get_children (GTK_CONTAINER (view->group_box));
  for (child = children; child != NULL && !shortcut_inserted; child = child->next)
    {
      group = THUNAR_SHORTCUT_GROUP (child->data);

      if (thunar_shortcut_group_try_add_shortcut (group, shortcut))
        shortcut_inserted = TRUE;
    }
  g_list_free (children);

  if (shortcut_inserted)
    {
      /* connect to the activated signal */
      g_signal_connect_swapped (shortcut, "activated",
                                G_CALLBACK (thunar_shortcuts_view_shortcut_activated),
                                view);

      /* react on state changes */
      g_signal_connect_swapped (shortcut, "state-changed",
                                G_CALLBACK (thunar_shortcuts_view_shortcut_state_changed),
                                view);
    }
  else
    {
      /* TODO improve this error message by including the URI or volume/mount name */
      g_warning ("Failed to add a shortcut to the side pane.");
    }
}



static void
thunar_shortcuts_view_shortcut_activated (ThunarShortcutsView *view,
                                          ThunarFile          *file,
                                          gboolean             open_in_new_window,
                                          ThunarShortcut      *shortcut)
{
  _thunar_return_if_fail (THUNAR_IS_SHORTCUTS_VIEW (view));
  _thunar_return_if_fail (THUNAR_IS_FILE (file));
  _thunar_return_if_fail (THUNAR_IS_SHORTCUT (shortcut));

  /* TODO cancel all other pending activations */
  
  /* open the activated shortcut */
  thunar_shortcuts_view_open (view, file, open_in_new_window);
}



static void
thunar_shortcuts_view_shortcut_state_changed (ThunarShortcutsView *view,
                                              GtkStateType         previous_state,
                                              ThunarShortcut      *shortcut)
{
  _thunar_return_if_fail (THUNAR_IS_SHORTCUTS_VIEW (view));
  _thunar_return_if_fail (THUNAR_IS_SHORTCUT (shortcut));

  /* check if the shortcut has been selected or highlighted */
  if (gtk_widget_get_state (GTK_WIDGET (shortcut)) == GTK_STATE_SELECTED)
    {
      /* unselect all other shortcuts */
      thunar_shortcuts_view_foreach_group (view,
                                           thunar_shortcuts_view_unselect_shortcuts,
                                           shortcut);
    }
  else if (gtk_widget_get_state (GTK_WIDGET (shortcut)) == GTK_STATE_PRELIGHT)
    {
      /* unprelight all other shortcuts */
      thunar_shortcuts_view_foreach_group (view,
                                           thunar_shortcuts_view_unprelight_shortcuts,
                                           shortcut);
    }
}



static void
thunar_shortcuts_view_unselect_shortcuts (ThunarShortcutsView *view,
                                          ThunarShortcutGroup *group,
                                          gpointer             user_data)
{
  _thunar_return_if_fail (THUNAR_IS_SHORTCUTS_VIEW (view));
  _thunar_return_if_fail (THUNAR_IS_SHORTCUT_GROUP (group));

  thunar_shortcut_group_unselect_shortcuts (group, THUNAR_SHORTCUT (user_data));
}



static void
thunar_shortcuts_view_unprelight_shortcuts (ThunarShortcutsView *view,
                                            ThunarShortcutGroup *group,
                                            gpointer             user_data)
{
  _thunar_return_if_fail (THUNAR_IS_SHORTCUTS_VIEW (view));
  _thunar_return_if_fail (THUNAR_IS_SHORTCUT_GROUP (group));

  thunar_shortcut_group_unprelight_shortcuts (group, THUNAR_SHORTCUT (user_data));
}



static void
thunar_shortcuts_view_foreach_group (ThunarShortcutsView                *view,
                                     ThunarShortcutsViewForeachGroupFunc func,
                                     gpointer                            user_data)
{
  GList *children;
  GList *iter;

  _thunar_return_if_fail (THUNAR_IS_SHORTCUTS_VIEW (view));

  children = gtk_container_get_children (GTK_CONTAINER (view->group_box));

  for (iter = children; iter != NULL; iter = iter->next)
    (*func) (view, THUNAR_SHORTCUT_GROUP (iter->data), user_data);

  g_list_free (children);
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



static void
thunar_shortcuts_view_update_selection (ThunarShortcutsView *view,
                                        ThunarShortcutGroup *group,
                                        gpointer             user_data)
{
  ThunarFile *file = THUNAR_FILE (user_data);

  _thunar_return_if_fail (THUNAR_IS_SHORTCUTS_VIEW (view));
  _thunar_return_if_fail (THUNAR_IS_SHORTCUT_GROUP (group));
  _thunar_return_if_fail (THUNAR_IS_FILE (file));

  thunar_shortcut_group_update_selection (group, file);
}



/**
 * thunar_shortcuts_view_new:
 *
 * Allocates a new #ThunarShortcutsView instance.
 *
 * Return value: the newly allocated #ThunarShortcutsView instance.
 **/
GtkWidget*
thunar_shortcuts_view_new (void)
{
  return g_object_new (THUNAR_TYPE_SHORTCUTS_VIEW, NULL);
}



gboolean
thunar_shortcuts_view_has_file (ThunarShortcutsView *view,
                                ThunarFile          *file)
{
  _thunar_return_val_if_fail (THUNAR_IS_SHORTCUTS_VIEW (view), FALSE);
  _thunar_return_val_if_fail (THUNAR_IS_FILE (file), FALSE);

  /* TODO */

  return FALSE;
}



void
thunar_shortcuts_view_add_file (ThunarShortcutsView *view,
                                ThunarFile          *file)
{
  _thunar_return_if_fail (THUNAR_IS_SHORTCUTS_VIEW (view));
  _thunar_return_if_fail (THUNAR_IS_FILE (file));

  /* TODO */
}



/**
 * thunar_shortcuts_view_select_by_file:
 * @view : a #ThunarShortcutsView instance.
 * @file : a #ThunarFile instance.
 *
 * Looks up the first shortcut that refers to @file in @view and selects it.
 * If @file is not present in the underlying data model, no shortcut will 
 * be selected afterwards.
 **/
void
thunar_shortcuts_view_select_by_file (ThunarShortcutsView *view,
                                      ThunarFile          *file)
{
  _thunar_return_if_fail (THUNAR_IS_SHORTCUTS_VIEW (view));
  _thunar_return_if_fail (THUNAR_IS_FILE (file));

  thunar_shortcuts_view_foreach_group (view, thunar_shortcuts_view_update_selection,
                                       file);
}


