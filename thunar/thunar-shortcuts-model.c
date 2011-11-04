/* vi:set et ai sw=2 sts=2 ts=2: */
/*-
 * Copyright (c) 2005-2006 Benedikt Meurer <benny@xfce.org>
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

#ifdef HAVE_ERRNO_H
#include <errno.h>
#endif
#ifdef HAVE_STDIO_H
#include <stdio.h>
#endif
#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif
#ifdef HAVE_LOCALE_H
#include <locale.h>
#endif

#include <glib.h>
#include <glib/gstdio.h>

#include <thunar/thunar-file.h>
#include <thunar/thunar-private.h>
#include <thunar/thunar-shortcuts-model.h>
#include <thunar/thunar-shortcut.h>



/* I don't particularly like this here, but it's shared across a few
 * files. */
const gchar *_thunar_user_directory_names[9] = {
  "Desktop", "Documents", "Download", "Music", "Pictures", "Public",
  "Templates", "Videos", NULL,
};



typedef struct _ThunarShortcutCategory ThunarShortcutCategory;



typedef enum
{
  THUNAR_SHORTCUT_CATEGORY_DEVICES,
  THUNAR_SHORTCUT_CATEGORY_PLACES,
  THUNAR_SHORTCUT_CATEGORY_NETWORK,
} ThunarShortcutCategoryType;



static void                    thunar_shortcuts_model_tree_model_init       (GtkTreeModelIface         *iface);
static void                    thunar_shortcuts_model_finalize              (GObject                   *object);
static GtkTreeModelFlags       thunar_shortcuts_model_get_flags             (GtkTreeModel              *tree_model);
static gint                    thunar_shortcuts_model_get_n_columns         (GtkTreeModel              *tree_model);
static GType                   thunar_shortcuts_model_get_column_type       (GtkTreeModel              *tree_model,
                                                                             gint                       idx);
static gboolean                thunar_shortcuts_model_get_iter              (GtkTreeModel              *tree_model,
                                                                             GtkTreeIter               *iter,
                                                                             GtkTreePath               *path);
static GtkTreePath            *thunar_shortcuts_model_get_path              (GtkTreeModel              *tree_model,
                                                                             GtkTreeIter               *iter);
static void                    thunar_shortcuts_model_get_value             (GtkTreeModel              *tree_model,
                                                                             GtkTreeIter               *iter,
                                                                             gint                       column,
                                                                             GValue                    *value);
static gboolean                thunar_shortcuts_model_iter_next             (GtkTreeModel              *tree_model,
                                                                             GtkTreeIter               *iter);
static gboolean                thunar_shortcuts_model_iter_children         (GtkTreeModel              *tree_model,
                                                                             GtkTreeIter               *iter,
                                                                             GtkTreeIter               *parent);
static gboolean                thunar_shortcuts_model_iter_has_child        (GtkTreeModel              *tree_model,
                                                                             GtkTreeIter               *iter);
static gint                    thunar_shortcuts_model_iter_n_children       (GtkTreeModel              *tree_model,
                                                                             GtkTreeIter               *iter);
static gboolean                thunar_shortcuts_model_iter_nth_child        (GtkTreeModel              *tree_model,
                                                                             GtkTreeIter               *iter,
                                                                             GtkTreeIter               *parent,
                                                                             gint                       n);
static gboolean                thunar_shortcuts_model_iter_parent           (GtkTreeModel              *tree_model,
                                                                             GtkTreeIter               *iter,
                                                                             GtkTreeIter               *child);
static gboolean                thunar_shortcuts_model_parse_path            (ThunarShortcutsModel      *model,
                                                                             GtkTreePath               *path,
                                                                             gint                      *category_index,
                                                                             gint                      *shortcut_index);
static gboolean                thunar_shortcuts_model_parse_iter            (ThunarShortcutsModel      *model,
                                                                             GtkTreeIter               *iter,
                                                                             gint                      *category_index,
                                                                             gint                      *shortcut_index);
static gboolean                thunar_shortcuts_model_find_category         (ThunarShortcutsModel      *model,
                                                                             ThunarShortcut            *shortcut,
                                                                             ThunarShortcutCategory   **category,
                                                                             gint                      *category_index);
static gboolean                thunar_shortcuts_model_find_shortcut         (ThunarShortcutsModel      *model,
                                                                             ThunarShortcut            *shortcut,
                                                                             gint                      *category_index,
                                                                             gint                      *shortcut_index);
static gboolean                thunar_shortcuts_model_find_volume           (ThunarShortcutsModel      *model,
                                                                             GVolume                   *volume,
                                                                             gint                      *category_index,
                                                                             gint                      *shortcut_index);
static gboolean                thunar_shortcuts_model_find_mount            (ThunarShortcutsModel      *model,
                                                                             GMount                    *mount,
                                                                             gint                      *category_index,
                                                                             gint                      *shortcut_index);
static void                    thunar_shortcuts_model_add_shortcut          (ThunarShortcutsModel      *model,
                                                                             ThunarShortcut            *shortcut);
static void                    thunar_shortcuts_model_shortcut_changed      (ThunarShortcutsModel      *model,
                                                                             GParamSpec                *pspec,
                                                                             ThunarShortcut            *shortcut);
static gboolean                thunar_shortcuts_model_load_system_shortcuts (gpointer                   user_data);
static gboolean                thunar_shortcuts_model_load_user_dirs        (gpointer                   user_data);
static gboolean                thunar_shortcuts_model_load_bookmarks        (gpointer                   user_data);
static gboolean                thunar_shortcuts_model_load_volumes          (gpointer                   user_data);
static void                    thunar_shortcuts_model_volume_added          (ThunarShortcutsModel      *model,
                                                                             GVolume                   *volume,
                                                                             GVolumeMonitor            *monitor);
static void                    thunar_shortcuts_model_volume_removed        (ThunarShortcutsModel      *model,
                                                                             GVolume                   *volume,
                                                                             GVolumeMonitor            *monitor);
static void                    thunar_shortcuts_model_mount_added           (ThunarShortcutsModel      *model,
                                                                             GMount                    *mount,
                                                                             GVolumeMonitor            *monitor);
static void                    thunar_shortcuts_model_mount_removed         (ThunarShortcutsModel      *model,
                                                                             GMount                    *mount,
                                                                             GVolumeMonitor            *monitor);
static ThunarShortcutCategory *thunar_shortcut_category_new                 (ThunarShortcutCategoryType type);
static void                    thunar_shortcut_category_free                (ThunarShortcutCategory    *category);



struct _ThunarShortcutsModelClass
{
  GObjectClass __parent__;
};

struct _ThunarShortcutsModel
{
  GObject         __parent__;

  /* the model stamp is only used when debugging is enabled 
   * in order to make sure we don't accept iterators generated 
   * by another model */
#ifndef NDEBUG
  gint            stamp;
#endif

  GPtrArray      *categories;

  guint           load_idle_id;

  GVolumeMonitor *volume_monitor;
};

struct _ThunarShortcutCategory
{
  ThunarShortcutCategoryType type;
  GPtrArray                 *shortcuts;
  gchar                     *name;
  guint                      visible : 1;
};



G_DEFINE_TYPE_WITH_CODE (ThunarShortcutsModel, thunar_shortcuts_model, G_TYPE_OBJECT,
                         G_IMPLEMENT_INTERFACE (GTK_TYPE_TREE_MODEL, 
                                                thunar_shortcuts_model_tree_model_init))


    
static void
thunar_shortcuts_model_class_init (ThunarShortcutsModelClass *klass)
{
  GObjectClass *gobject_class;

  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->finalize = thunar_shortcuts_model_finalize;
}



static void
thunar_shortcuts_model_tree_model_init (GtkTreeModelIface *iface)
{
  iface->get_flags = thunar_shortcuts_model_get_flags;
  iface->get_n_columns = thunar_shortcuts_model_get_n_columns;
  iface->get_column_type = thunar_shortcuts_model_get_column_type;
  iface->get_iter = thunar_shortcuts_model_get_iter;
  iface->get_path = thunar_shortcuts_model_get_path;
  iface->get_value = thunar_shortcuts_model_get_value;
  iface->iter_next = thunar_shortcuts_model_iter_next;
  iface->iter_children = thunar_shortcuts_model_iter_children;
  iface->iter_has_child = thunar_shortcuts_model_iter_has_child;
  iface->iter_n_children = thunar_shortcuts_model_iter_n_children;
  iface->iter_nth_child = thunar_shortcuts_model_iter_nth_child;
  iface->iter_parent = thunar_shortcuts_model_iter_parent;
}



static void
thunar_shortcuts_model_init (ThunarShortcutsModel *model)
{
  ThunarShortcutCategory *category;

#ifndef NDEBUG
  model->stamp = g_random_int ();
#endif

  /* allocate an array for the shortcut categories */
  model->categories = 
    g_ptr_array_new_with_free_func ((GDestroyNotify) thunar_shortcut_category_free);

  /* create the devices category */
  category = thunar_shortcut_category_new (THUNAR_SHORTCUT_CATEGORY_DEVICES);
  g_ptr_array_add (model->categories, category);

  /* create the places category */
  category = thunar_shortcut_category_new (THUNAR_SHORTCUT_CATEGORY_PLACES);
  g_ptr_array_add (model->categories, category);

  /* create the network category */
  category = thunar_shortcut_category_new (THUNAR_SHORTCUT_CATEGORY_NETWORK);
  g_ptr_array_add (model->categories, category);

  /* start the load chain */
  model->load_idle_id = g_idle_add (thunar_shortcuts_model_load_system_shortcuts, model);
}



static void
thunar_shortcuts_model_finalize (GObject *object)
{
  ThunarShortcutsModel *model = THUNAR_SHORTCUTS_MODEL (object);

  _thunar_return_if_fail (THUNAR_IS_SHORTCUTS_MODEL (model));

  if (model->load_idle_id != 0)
    g_source_remove (model->load_idle_id);
  
  /* check if we have a volume monitor */
  if (model->volume_monitor != NULL)
    {
      /* disconnect from the monitor signals */
      g_signal_handlers_disconnect_matched (model->volume_monitor, 
                                            G_SIGNAL_MATCH_DATA,
                                            0, 0, 0, NULL, model);

      /* release the volume monitor */
      g_object_unref (model->volume_monitor);
    }

  /* free shortcut categories and their shortcuts */
  g_ptr_array_free (model->categories, TRUE);

  (*G_OBJECT_CLASS (thunar_shortcuts_model_parent_class)->finalize) (object);
}



static GtkTreeModelFlags
thunar_shortcuts_model_get_flags (GtkTreeModel *tree_model)
{
  return GTK_TREE_MODEL_ITERS_PERSIST;
}



static gint
thunar_shortcuts_model_get_n_columns (GtkTreeModel *tree_model)
{
  return THUNAR_SHORTCUTS_MODEL_N_COLUMNS;
}



static GType
thunar_shortcuts_model_get_column_type (GtkTreeModel *tree_model,
                                        gint          idx)
{
  switch (idx)
    {
    case THUNAR_SHORTCUTS_MODEL_COLUMN_ICON:
      return G_TYPE_ICON;

    case THUNAR_SHORTCUTS_MODEL_COLUMN_NAME:
      return G_TYPE_STRING;

    case THUNAR_SHORTCUTS_MODEL_COLUMN_LOCATION:
      return G_TYPE_FILE;

    case THUNAR_SHORTCUTS_MODEL_COLUMN_FILE:
      return THUNAR_TYPE_FILE;

    case THUNAR_SHORTCUTS_MODEL_COLUMN_VOLUME:
      return G_TYPE_VOLUME;

    case THUNAR_SHORTCUTS_MODEL_COLUMN_MOUNT:
      return G_TYPE_MOUNT;

    case THUNAR_SHORTCUTS_MODEL_COLUMN_MUTABLE:
      return G_TYPE_BOOLEAN;

    case THUNAR_SHORTCUTS_MODEL_COLUMN_EJECT_ICON:
      return G_TYPE_ICON;

    case THUNAR_SHORTCUTS_MODEL_COLUMN_CATEGORY:
      return G_TYPE_BOOLEAN;

    case THUNAR_SHORTCUTS_MODEL_COLUMN_PERSISTENT:
      return G_TYPE_BOOLEAN;

    case THUNAR_SHORTCUTS_MODEL_COLUMN_VISIBLE:
      return G_TYPE_BOOLEAN;
    }

  _thunar_assert_not_reached ();
  return G_TYPE_INVALID;
}



static gboolean
thunar_shortcuts_model_get_iter (GtkTreeModel *tree_model,
                                 GtkTreeIter  *iter,
                                 GtkTreePath  *path)
{
  ThunarShortcutsModel *model = THUNAR_SHORTCUTS_MODEL (tree_model);
  gint                  category_index;
  gint                  shortcut_index;

  _thunar_return_val_if_fail (THUNAR_IS_SHORTCUTS_MODEL (model), FALSE);
  _thunar_return_val_if_fail (gtk_tree_path_get_depth (path) > 0 && gtk_tree_path_get_depth (path) <= 2, FALSE);

  /* parse the path and abort if it is invalid */
  if (!thunar_shortcuts_model_parse_path (model, path, &category_index, &shortcut_index))
    return FALSE;

#ifndef NDEBUG
  (*iter).stamp = model->stamp;
#endif
  (*iter).user_data = GINT_TO_POINTER (category_index);
  (*iter).user_data2 = GINT_TO_POINTER (shortcut_index);

  return TRUE;
}



static GtkTreePath*
thunar_shortcuts_model_get_path (GtkTreeModel *tree_model,
                                 GtkTreeIter  *iter)
{
  ThunarShortcutsModel *model = THUNAR_SHORTCUTS_MODEL (tree_model);
  gint                  category_index;
  gint                  shortcut_index;

  _thunar_return_val_if_fail (THUNAR_IS_SHORTCUTS_MODEL (model), NULL);
  _thunar_return_val_if_fail (iter->stamp == model->stamp, NULL);

  /* parse the iter and abort if it is invalid */
  if (!thunar_shortcuts_model_parse_iter (model, iter, &category_index, &shortcut_index))
    return NULL;

  /* create a new tree path from the category and shortcut indices.
   * note that if the iter refers to a category only, the shortcut 
   * index will be -1 and thus, the path will be constructed correctly
   * with only the category index */
  return gtk_tree_path_new_from_indices (category_index, shortcut_index, -1);
}



static void
thunar_shortcuts_model_get_value (GtkTreeModel *tree_model,
                                  GtkTreeIter  *iter,
                                  gint          column,
                                  GValue       *value)
{
  ThunarShortcutCategory *category;
  ThunarShortcutsModel   *model = THUNAR_SHORTCUTS_MODEL (tree_model);
  ThunarShortcut         *shortcut;
  const gchar            *name;
  GIcon                  *icon;
  gint                    category_index;
  gint                    shortcut_index;

  _thunar_return_if_fail (THUNAR_IS_SHORTCUTS_MODEL (tree_model));
  _thunar_return_if_fail (iter->stamp == model->stamp);

  if (!thunar_shortcuts_model_parse_iter (model, iter, &category_index, &shortcut_index))
    _thunar_assert_not_reached ();

  /* get the category and shortcut for the iter */
  category = g_ptr_array_index (model->categories, category_index);
  shortcut = shortcut_index < 0 ? NULL : g_ptr_array_index (category->shortcuts, 
                                                            shortcut_index);

  switch (column)
    {
    case THUNAR_SHORTCUTS_MODEL_COLUMN_ICON:
      g_value_init (value, G_TYPE_ICON);
      if (shortcut != NULL)
        {
          icon = thunar_shortcut_get_custom_icon (shortcut);
          if (icon == NULL)
            icon = thunar_shortcut_get_icon (shortcut);

          g_value_set_object (value, icon);
        }
      else
        {
          g_value_set_object (value, NULL);
        }
      break;

    case THUNAR_SHORTCUTS_MODEL_COLUMN_NAME:
      g_value_init (value, G_TYPE_STRING);
      if (shortcut != NULL)
        {
          name = thunar_shortcut_get_custom_name (shortcut);
          if (name == NULL)
            name = thunar_shortcut_get_name (shortcut);

          g_value_set_static_string (value, name);
        }
      else
        {
          g_value_set_static_string (value, category->name);
        }
      break;

    case THUNAR_SHORTCUTS_MODEL_COLUMN_LOCATION:
      g_value_init (value, G_TYPE_FILE);
      if (shortcut != NULL)
        g_value_set_object (value, thunar_shortcut_get_location (shortcut));
      else
        g_value_set_object (value, NULL);
      break;

    case THUNAR_SHORTCUTS_MODEL_COLUMN_FILE:
      g_value_init (value, THUNAR_TYPE_FILE);
      if (shortcut != NULL)
        g_value_set_object (value, thunar_shortcut_get_file (shortcut));
      else
        g_value_set_object (value, NULL);
      break;

    case THUNAR_SHORTCUTS_MODEL_COLUMN_VOLUME:
      g_value_init (value, G_TYPE_VOLUME);
      if (shortcut != NULL)
        g_value_set_object (value, thunar_shortcut_get_volume (shortcut));
      else
        g_value_set_object (value, NULL);
      break;

    case THUNAR_SHORTCUTS_MODEL_COLUMN_MOUNT:
      g_value_init (value, G_TYPE_MOUNT);
      if (shortcut != NULL)
        g_value_set_object (value, thunar_shortcut_get_mount (shortcut));
      else
        g_value_set_object (value, NULL);
      break;

    case THUNAR_SHORTCUTS_MODEL_COLUMN_SHORTCUT_TYPE:
      g_value_init (value, THUNAR_TYPE_SHORTCUT_TYPE);
      if (shortcut != NULL)
        g_value_set_enum (value, thunar_shortcut_get_shortcut_type (shortcut));
      else
        g_value_set_enum (value, 0);
      break;

    case THUNAR_SHORTCUTS_MODEL_COLUMN_MUTABLE:
      g_value_init (value, G_TYPE_BOOLEAN);
      if (shortcut != NULL)
        g_value_set_boolean (value, thunar_shortcut_get_mutable (shortcut));
      else
        g_value_set_boolean (value, FALSE);
      break;

    case THUNAR_SHORTCUTS_MODEL_COLUMN_EJECT_ICON:
      g_value_init (value, G_TYPE_ICON);
      if (shortcut != NULL)
        g_value_set_object (value, thunar_shortcut_get_eject_icon (shortcut));
      else
        g_value_set_object (value, NULL);
      break;

    case THUNAR_SHORTCUTS_MODEL_COLUMN_CATEGORY:
      g_value_init (value, G_TYPE_BOOLEAN);
      g_value_set_boolean (value, shortcut == NULL);
      break;

    case THUNAR_SHORTCUTS_MODEL_COLUMN_PERSISTENT:
      g_value_init (value, G_TYPE_BOOLEAN);
      if (shortcut != NULL)
        g_value_set_boolean (value, thunar_shortcut_get_persistent (shortcut));
      else
        g_value_set_boolean (value, FALSE);
      break;

    case THUNAR_SHORTCUTS_MODEL_COLUMN_VISIBLE:
      g_value_init (value, G_TYPE_BOOLEAN);
      if (shortcut != NULL)
        g_value_set_boolean (value, !thunar_shortcut_get_hidden (shortcut));
      else
        g_value_set_boolean (value, category->visible);
      break;

    default:
      _thunar_assert_not_reached ();
      break;
    }
}



static gboolean
thunar_shortcuts_model_iter_next (GtkTreeModel *tree_model,
                                  GtkTreeIter  *iter)
{
  ThunarShortcutCategory *category;
  ThunarShortcutsModel   *model = THUNAR_SHORTCUTS_MODEL (tree_model);
  ThunarShortcut         *shortcut;
  gint                    category_index;
  gint                    shortcut_index;

  _thunar_return_val_if_fail (THUNAR_IS_SHORTCUTS_MODEL (tree_model), FALSE);
  _thunar_return_val_if_fail (iter->stamp == model->stamp, FALSE);

  /* parse the iter and abort if it is invalid */
  if (!thunar_shortcuts_model_parse_iter (model, iter, &category_index, &shortcut_index))
    return FALSE;

  /* get the category and shortcut for the iter */
  category = g_ptr_array_index (model->categories, category_index);
  shortcut = shortcut_index < 0 ? NULL : g_ptr_array_index (category->shortcuts, 
                                                            shortcut_index);

  if (shortcut != NULL)
    {
      /* check if we can advance to another shortcut */
      if ((guint) shortcut_index < category->shortcuts->len - 1)
        {
          (*iter).user_data2 = GINT_TO_POINTER (shortcut_index + 1);
          return TRUE;
        }
    }
  else
    {
      /* check if we can advance to another category */
      if ((guint) category_index < model->categories->len - 1)
        {
          (*iter).user_data = GINT_TO_POINTER (category_index + 1);
          return TRUE;
        }
    }

  return FALSE;
}



static gboolean
thunar_shortcuts_model_iter_children (GtkTreeModel *tree_model,
                                      GtkTreeIter  *iter,
                                      GtkTreeIter  *parent)
{
  ThunarShortcutCategory *category;
  ThunarShortcutsModel   *model = THUNAR_SHORTCUTS_MODEL (tree_model);
  gint                    category_index;
  gint                    shortcut_index;

  _thunar_return_val_if_fail (THUNAR_IS_SHORTCUTS_MODEL (tree_model), FALSE);
  _thunar_return_val_if_fail (parent == NULL || parent->stamp == model->stamp, 0);

  /* determine whether to return the first category or the 
   * first shortcut of a category */
  if (parent == NULL)
    {
      /* check whether we have at least one category to offer */
      if (model->categories->len > 0)
        {
#ifndef NDEBUG
          (*iter).stamp = model->stamp;
#endif
          (*iter).user_data = GINT_TO_POINTER (0);
          (*iter).user_data2 = GINT_TO_POINTER (-1);
          return TRUE;
        }
    }
  else
    {
      /* parse the parent iter and abort if it is invalid */
      if (!thunar_shortcuts_model_parse_iter (model, parent, 
                                              &category_index, &shortcut_index))
        {
          return FALSE;
        }

      /* shortcuts have no children */
      if (shortcut_index >= 0)
        return FALSE;

      /* get the category */
      category = g_ptr_array_index (model->categories, category_index);

      /* check whether we have at least one shortcut to offer 
       * in the category */
      if (category->shortcuts->len > 0)
        {
#ifndef NDEBUG
          (*iter).stamp = model->stamp;
#endif
          (*iter).user_data = GINT_TO_POINTER (category_index);
          (*iter).user_data2 = GINT_TO_POINTER (0);
          return TRUE;
        }
    }

  return FALSE;
}



static gboolean
thunar_shortcuts_model_iter_has_child (GtkTreeModel *tree_model,
                                       GtkTreeIter  *iter)
{
  ThunarShortcutCategory *category;
  ThunarShortcutsModel   *model = THUNAR_SHORTCUTS_MODEL (tree_model);
  gint                    category_index;
  gint                    shortcut_index;

  _thunar_return_val_if_fail (THUNAR_IS_SHORTCUTS_MODEL (tree_model), FALSE);
  _thunar_return_val_if_fail (iter->stamp == model->stamp, FALSE);

  /* parse the iter and abort if it is invalid */
  if (!thunar_shortcuts_model_parse_iter (model, iter, &category_index, &shortcut_index))
    return FALSE;

  /* shortcuts don't hve children */
  if (shortcut_index >= 0)
    return FALSE;

  /* get the category */
  category = g_ptr_array_index (model->categories, category_index);

  /* check whether the category has at least one shortcut */
  return category->shortcuts->len > 0;
}



static gint
thunar_shortcuts_model_iter_n_children (GtkTreeModel *tree_model,
                                        GtkTreeIter  *iter)
{
  ThunarShortcutCategory *category;
  ThunarShortcutsModel   *model = THUNAR_SHORTCUTS_MODEL (tree_model);
  gint                    category_index;
  gint                    shortcut_index;

  _thunar_return_val_if_fail (THUNAR_IS_SHORTCUTS_MODEL (tree_model), 0);
  _thunar_return_val_if_fail (iter == NULL || iter->stamp == model->stamp, 0);

  if (iter == NULL)
    {
      /* return the number of categories */
      return model->categories->len;
    }
  else
    {
      /* parse the iter and abort if it is invalid */
      if (!thunar_shortcuts_model_parse_iter (model, iter, 
                                              &category_index, &shortcut_index))
        {
          return 0;
        }

      /* shortcuts don't hve children */
      if (shortcut_index >= 0)
        return 0;

      /* get the category */
      category = g_ptr_array_index (model->categories, category_index);

      /* return the number of shortcuts of the category */
      return category->shortcuts->len;
    }
}



static gboolean
thunar_shortcuts_model_iter_nth_child (GtkTreeModel *tree_model,
                                       GtkTreeIter  *iter,
                                       GtkTreeIter  *parent,
                                       gint          n)
{
  ThunarShortcutCategory *category;
  ThunarShortcutsModel   *model = THUNAR_SHORTCUTS_MODEL (tree_model);
  gint                    category_index;
  gint                    shortcut_index;

  _thunar_return_val_if_fail (THUNAR_IS_SHORTCUTS_MODEL (tree_model), FALSE);
  _thunar_return_val_if_fail (parent == NULL || parent->stamp == model->stamp, FALSE);

  if (iter == NULL)
    {
      /* check whether we have n categories to offer at all */
      if ((guint) n >= model->categories->len)
        return FALSE;

      /* return the nth category */
#ifndef NDEBUG
      (*iter).stamp = model->stamp;
#endif
      (*iter).user_data = GINT_TO_POINTER (n);
      (*iter).user_data2 = GINT_TO_POINTER (-1);
      return TRUE;
    }
  else
    {
      /* parse the iter and abort if it is invalid */
      if (!thunar_shortcuts_model_parse_iter (model, parent, 
                                              &category_index, &shortcut_index))
        {
          return FALSE;
        }

      /* shortcuts don't hve children */
      if (shortcut_index >= 0)
        return FALSE;

      /* get the category */
      category = g_ptr_array_index (model->categories, category_index);

      /* check whether we have n shortcuts to offer at all */
      if ((guint) n >= category->shortcuts->len)
        return FALSE;

      /* return the nth shortcut of the category */
#ifndef NDEBUG
      (*iter).stamp = model->stamp;
#endif
      (*iter).user_data = GINT_TO_POINTER (category_index);
      (*iter).user_data2 = GINT_TO_POINTER (n);
      return TRUE;
    }
}



static gboolean
thunar_shortcuts_model_iter_parent (GtkTreeModel *tree_model,
                                    GtkTreeIter  *iter,
                                    GtkTreeIter  *child)
{
  ThunarShortcutsModel *model = THUNAR_SHORTCUTS_MODEL (tree_model);
  gint                  category_index;
  gint                  shortcut_index;

  _thunar_return_val_if_fail (THUNAR_IS_SHORTCUTS_MODEL (tree_model), 0);
  _thunar_return_val_if_fail (child->stamp == child->stamp, 0);

  /* parse the iter and abort if it is invalid */
  if (!thunar_shortcuts_model_parse_iter (model, child, 
                                          &category_index, &shortcut_index))
    {
      return FALSE;
    }

  /* categories don't have a parent */
  if (shortcut_index < 0)
    return FALSE;

  /* return an iter for the category */
#ifndef NDEBUG
  (*iter).stamp = model->stamp;
#endif
  (*iter).user_data = GINT_TO_POINTER (category_index);
  (*iter).user_data2 = GINT_TO_POINTER (-1);
  return TRUE;
}



static gboolean
thunar_shortcuts_model_parse_path (ThunarShortcutsModel *model,
                                   GtkTreePath          *path,
                                   gint                 *category_index,
                                   gint                 *shortcut_index)
{
  ThunarShortcutCategory *category;
  gint                   *indices;

  _thunar_return_val_if_fail (THUNAR_IS_SHORTCUTS_MODEL (model), FALSE);
  _thunar_return_val_if_fail (path != NULL, FALSE);
  _thunar_return_val_if_fail (gtk_tree_path_get_depth (path) > 0 && gtk_tree_path_get_depth (path) <= 2, FALSE);

  /* read indices from the path */
  indices = gtk_tree_path_get_indices (path);

  /* abort if the category index is out of range */
  if (indices[0] < 0 || (guint) indices[0] >= model->categories->len)
    return FALSE;

  /* set the category index */
  if (category_index != NULL)
    *category_index = indices[0];

  /* get the category */
  category = g_ptr_array_index (model->categories, indices[0]);
  g_assert (category != NULL);

  /* parse the shortcut information of the path */
  if (gtk_tree_path_get_depth (path) < 2 || indices[1] < 0)
    {
      /* no shortcut information given, set shortcut index to -1 */
      if (shortcut_index != NULL)
        *shortcut_index = -1;

      /* we're done */
      return TRUE;
    }
  else
    {
      /* abort if the shortcut index is set but out of range */
      if (indices[1] >= 0 && (guint) indices[1] >= category->shortcuts->len)
        return FALSE;

      /* valid shortcut index given, return it */
      if (shortcut_index != NULL)
        *shortcut_index = indices[1];

      /* we're done */
      return TRUE;
    }

  return FALSE;
}



static gboolean
thunar_shortcuts_model_parse_iter (ThunarShortcutsModel *model,
                                   GtkTreeIter          *iter,
                                   gint                 *category_index,
                                   gint                 *shortcut_index)
{
  GtkTreePath *path;
  gint         indices[2] = { -1, -1 };

  _thunar_return_val_if_fail (THUNAR_IS_SHORTCUTS_MODEL (model), FALSE);
  _thunar_return_val_if_fail (iter != NULL, FALSE);
  _thunar_return_val_if_fail (iter->stamp == model->stamp, FALSE);

  /* read indices from the iter */
  indices[0] = GPOINTER_TO_INT ((*iter).user_data);
  indices[1] = GPOINTER_TO_INT ((*iter).user_data2);

  /* create a tree path for the indices */
  path = gtk_tree_path_new_from_indices (indices[0], indices[1], -1);

  /* re-use the code we have to parse a tree path */
  return thunar_shortcuts_model_parse_path (model, path, category_index, shortcut_index);
}



static gboolean
thunar_shortcuts_model_find_category (ThunarShortcutsModel    *model,
                                      ThunarShortcut          *shortcut,
                                      ThunarShortcutCategory **category,
                                      gint                    *category_index)
{
  ThunarShortcutCategory *current_category = NULL;
  ThunarShortcutType      type;
  gboolean                item_belongs_here = FALSE;
  GFile                  *file;
  guint                   n;

  _thunar_return_val_if_fail (THUNAR_IS_SHORTCUTS_MODEL (model), FALSE);

  /* reset category return parameter */
  if (category != NULL)
    *category = NULL;

  /* reset category index return parameter */
  if (category_index != NULL)
    *category_index = -1;

  /* read information needed to find the category from the shortcut */
  type = thunar_shortcut_get_shortcut_type (shortcut);
  file = thunar_shortcut_get_location (shortcut);
  
  /* iterate over all available categories */
  for (n = 0; !item_belongs_here && n < model->categories->len; ++n)
    {
      /* get the nth category */
      current_category = g_ptr_array_index (model->categories, n);

      switch (current_category->type)
        {
        case THUNAR_SHORTCUT_CATEGORY_DEVICES:
          /* regular volumes belong here */
          if (type == THUNAR_SHORTCUT_REGULAR_VOLUME)
            item_belongs_here = TRUE;

          /* directly ejectable volumes belong here */
          if (type == THUNAR_SHORTCUT_EJECTABLE_VOLUME)
            item_belongs_here = TRUE;

          /* mounts with mount points that are in archive:// belong here */
          if (type == THUNAR_SHORTCUT_REGULAR_MOUNT 
              && file != NULL 
              && g_file_has_uri_scheme (file, "archive"))
            {
              item_belongs_here = TRUE;
            }

          /* mounts with mount points that are in file:// belong here */
          if (type == THUNAR_SHORTCUT_REGULAR_MOUNT 
              && file != NULL 
              && g_file_has_uri_scheme (file, "file"))
            {
              item_belongs_here = TRUE;
            }
          break;

        case THUNAR_SHORTCUT_CATEGORY_PLACES:
          /* regular files belong here */
          if (type == THUNAR_SHORTCUT_REGULAR_FILE)
            item_belongs_here = TRUE;

          /* trash directories belong here */
          if (file != NULL && g_file_has_uri_scheme (file, "trash"))
            item_belongs_here = TRUE;
          break;

        case THUNAR_SHORTCUT_CATEGORY_NETWORK:
          /* remote files belong here */
          if (type == THUNAR_SHORTCUT_NETWORK_FILE)
            item_belongs_here = TRUE;

          /* remote mounts belong here */
          if (type == THUNAR_SHORTCUT_REGULAR_MOUNT
              && file != NULL 
              && !g_file_has_uri_scheme (file, "archive")
              && !g_file_has_uri_scheme (file, "file")
              && !g_file_has_uri_scheme (file, "trash"))
            {
              item_belongs_here = TRUE;
            }
          break;

        default:
          _thunar_assert_not_reached ();
          break;
        }

      /* check if the item belongs into this category */
      if (item_belongs_here)
        {
          /* return the category if requested */
          if (category != NULL)
            *category = current_category;

          /* return the category index if requested */
          if (category_index != NULL)
            *category_index = n;
        }
    }

  return item_belongs_here;
}



static gboolean
thunar_shortcuts_model_find_shortcut (ThunarShortcutsModel *model,
                                      ThunarShortcut       *shortcut,
                                      gint                 *category_index,
                                      gint                 *shortcut_index)
{
  ThunarShortcutCategory *category;
  ThunarShortcut         *current_shortcut;
  gboolean                shortcut_found = FALSE;
  gint                    c;
  gint                    s;

  _thunar_return_val_if_fail (THUNAR_IS_SHORTCUTS_MODEL (model), FALSE);
  _thunar_return_val_if_fail (THUNAR_IS_SHORTCUT (shortcut), FALSE);

  for (c = 0; !shortcut_found && (guint) c < model->categories->len; ++c)
    {
      category = g_ptr_array_index (model->categories, c);

      for (s = 0; !shortcut_found && (guint) s < category->shortcuts->len; ++s)
        {
          current_shortcut = g_ptr_array_index (category->shortcuts, s);

          if (current_shortcut == shortcut)
            {
              if (category_index != NULL)
                *category_index = c;

              if (shortcut_index != NULL)
                *shortcut_index = s;

              shortcut_found = TRUE;
            }
        }
    }

  return shortcut_found;
}



static gboolean
thunar_shortcuts_model_find_volume (ThunarShortcutsModel *model,
                                    GVolume              *volume,
                                    gint                 *category_index,
                                    gint                 *shortcut_index)
{
  ThunarShortcutCategory *category;
  ThunarShortcut         *shortcut;
  gboolean                shortcut_found = FALSE;
  GVolume                *shortcut_volume;
  gint                    c;
  gint                    s;

  _thunar_return_val_if_fail (THUNAR_IS_SHORTCUTS_MODEL (model), FALSE);
  _thunar_return_val_if_fail (G_IS_VOLUME (volume), FALSE);

  for (c = 0; !shortcut_found && (guint) c < model->categories->len; ++c)
    {
      category = g_ptr_array_index (model->categories, c);

      for (s = 0; !shortcut_found && (guint) s < category->shortcuts->len; ++s)
        {
          shortcut = g_ptr_array_index (category->shortcuts, s);
          shortcut_volume = thunar_shortcut_get_volume (shortcut);

          if (shortcut_volume == volume)
            {
              if (category_index != NULL)
                *category_index = c;

              if (shortcut_index != NULL)
                *shortcut_index = s;

              shortcut_found = TRUE;
            }
        }
    }

  return shortcut_found;
}



static gboolean
thunar_shortcuts_model_find_mount (ThunarShortcutsModel *model,
                                   GMount               *mount,
                                   gint                 *category_index,
                                   gint                 *shortcut_index)
{
  ThunarShortcutCategory *category;
  ThunarShortcut         *shortcut;
  gboolean                shortcut_found = FALSE;
  GMount                 *shortcut_mount;
  gint                    c;
  gint                    s;

  _thunar_return_val_if_fail (THUNAR_IS_SHORTCUTS_MODEL (model), FALSE);
  _thunar_return_val_if_fail (G_IS_MOUNT (mount), FALSE);

  for (c = 0; !shortcut_found && (guint) c < model->categories->len; ++c)
    {
      category = g_ptr_array_index (model->categories, c);

      for (s = 0; !shortcut_found && (guint) s < category->shortcuts->len; ++s)
        {
          shortcut = g_ptr_array_index (category->shortcuts, s);
          shortcut_mount = thunar_shortcut_get_mount (shortcut);

          if (shortcut_mount == mount)
            {
              if (category_index != NULL)
                *category_index = c;

              if (shortcut_index != NULL)
                *shortcut_index = s;

              shortcut_found = TRUE;
            }
        }
    }

  return shortcut_found;
}



static void
thunar_shortcuts_model_add_shortcut (ThunarShortcutsModel *model,
                                     ThunarShortcut       *shortcut)
{
  ThunarShortcutCategory *category;
  GtkTreePath            *path;
  GtkTreeIter             iter;
  gint                    category_index;

  _thunar_return_if_fail (THUNAR_IS_SHORTCUTS_MODEL (model));
  _thunar_return_if_fail (THUNAR_IS_SHORTCUT (shortcut));

  /* find the destination category for the shortcut */
  if (!thunar_shortcuts_model_find_category (model, shortcut, 
                                             &category, &category_index))
    {
      g_debug ("failed to add shortcut \"%s\": no category found",
               thunar_shortcut_get_name (shortcut));
      return;
    }

  /* insert the shortcut into the category if we have one for it */
  /* add the shortcut to the category */
  g_ptr_array_add (category->shortcuts, shortcut);

  /* create a tree path for the new row */
  path = gtk_tree_path_new_from_indices (category_index, 
                                         category->shortcuts->len - 1,
                                         -1);

  /* create a tree iter for the new row */
#ifndef NDEBUG
  iter.stamp = model->stamp;
#endif
  iter.user_data = GINT_TO_POINTER (category_index);
  iter.user_data2 = GINT_TO_POINTER (category->shortcuts->len - 1);

  /* create a tree path for the new row */
  g_signal_emit_by_name (model, "row-inserted", path, &iter, NULL);

  /* release the tree path */
  gtk_tree_path_free (path);

  /* be notified whenever the shortcut changes */
  g_signal_connect_swapped (shortcut, "notify", 
                            G_CALLBACK (thunar_shortcuts_model_shortcut_changed),
                            model);
}



static void
thunar_shortcuts_model_shortcut_changed (ThunarShortcutsModel *model,
                                         GParamSpec           *pspec,
                                         ThunarShortcut       *shortcut)
{
  GtkTreePath *path;
  GtkTreeIter  iter;
  gint         category_index;
  gint         shortcut_index;

  _thunar_return_if_fail (THUNAR_IS_SHORTCUTS_MODEL (model));
  _thunar_return_if_fail (THUNAR_IS_SHORTCUT (shortcut));

  /* try find the shortcut in the model */
  if (thunar_shortcuts_model_find_shortcut (model, shortcut, 
                                            &category_index, &shortcut_index))
    {
      /* create a tree path for the row */
      path = gtk_tree_path_new_from_indices (category_index, shortcut_index, -1);

      /* create a tree iter for the row */
      gtk_tree_model_get_iter (GTK_TREE_MODEL (model), &iter, path);

      /* notify others that the row has changed */
      g_signal_emit_by_name (model, "row-changed", path, &iter, NULL);

      /* release the tree path */
      gtk_tree_path_free (path);
    }
}



static gboolean
thunar_shortcuts_model_load_system_shortcuts (gpointer user_data)
{
  ThunarShortcutsModel *model = THUNAR_SHORTCUTS_MODEL (user_data);
  ThunarShortcut       *shortcut;
  GFile                *home_file;
  GFile                *location;
  GIcon                *icon;
  gchar                *name = NULL;
  gchar                *path;

  _thunar_return_val_if_fail (THUNAR_IS_SHORTCUTS_MODEL (model), FALSE);
  
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
  thunar_shortcuts_model_add_shortcut (model, shortcut);

  /* release $HOME information */
  g_free (path);
  g_object_unref (icon);

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
      thunar_shortcuts_model_add_shortcut (model, shortcut);

      /* release desktop information */
      g_free (name);
      g_object_unref (icon);
    }

  /* release desktop and home files */
  g_object_unref (location);
  g_object_unref (home_file);

  /* create trash information */
  location = thunar_g_file_new_for_trash ();
  icon = g_themed_icon_new ("user-trash");
  name = g_strdup (_("Trash"));

  /* create the trash shortcut */
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
  thunar_shortcuts_model_add_shortcut (model, shortcut);

  /* release trash information */
  g_free (name);
  g_object_unref (icon);
  g_object_unref (location);

  /* create network information */
  /* TODO we need a new "uri" property for ThunarShortcut that is 
   * resolved into a real GFile and then into a ThunarFile 
   * asynchronously */
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
  thunar_shortcuts_model_add_shortcut (model, shortcut);

  /* release network information */
  g_free (name);
  g_object_unref (icon);
  g_object_unref (location);

  /* load rest of the user dirs next */
  model->load_idle_id = g_idle_add (thunar_shortcuts_model_load_user_dirs, model);

  return FALSE;
}



static gboolean
thunar_shortcuts_model_load_user_dirs (gpointer user_data)
{
  ThunarShortcutsModel *model = THUNAR_SHORTCUTS_MODEL (user_data);

  _thunar_return_val_if_fail (THUNAR_IS_SHORTCUTS_MODEL (model), FALSE);
  
  /* load GTK+ bookmarks next */
  model->load_idle_id = g_idle_add (thunar_shortcuts_model_load_bookmarks, model);

  return FALSE;
}



static gboolean
thunar_shortcuts_model_load_bookmarks (gpointer user_data)
{
  ThunarShortcutsModel *model = THUNAR_SHORTCUTS_MODEL (user_data);
  ThunarShortcutType    type;
  ThunarShortcut       *shortcut;
  gboolean              is_local;
  GFile                *bookmarks_file;
  GFile                *home_file;
  GFile                *location;
  GIcon                *eject_icon;
  GIcon                *icon;
  gchar                *bookmarks_path;
  gchar                 line[2048];
  gchar                *name;
  FILE                 *fp;

  _thunar_return_val_if_fail (THUNAR_IS_SHORTCUTS_MODEL (model), FALSE);

  /* create an eject icon */
  eject_icon = g_themed_icon_new ("media-eject");

  /* resolve the bookmarks file */
  home_file = thunar_g_file_new_for_home ();
  bookmarks_file = g_file_resolve_relative_path (home_file, ".gtk-bookmarks");
  bookmarks_path = g_file_get_path (bookmarks_file);
  g_object_unref (bookmarks_file);
  g_object_unref (home_file);

  /* TODO remove all existing bookmarks */
  
  /* open the GTK+ bookmarks file for reading */
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
              thunar_shortcuts_model_add_shortcut (model, shortcut);

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
  model->load_idle_id = g_idle_add (thunar_shortcuts_model_load_volumes, model);

  return FALSE;
}



static gboolean
thunar_shortcuts_model_load_volumes (gpointer user_data)
{
  ThunarShortcutsModel *model = THUNAR_SHORTCUTS_MODEL (user_data);
  GList                *mounts;
  GList                *volumes;
  GList                *lp;

  _thunar_return_val_if_fail (THUNAR_IS_SHORTCUTS_MODEL (model), FALSE);

  /* get the default volume monitor */
  model->volume_monitor = g_volume_monitor_get ();

  /* get a list of all volumes available */
  volumes = g_volume_monitor_get_volumes (model->volume_monitor);

  /* create shortcuts for the volumes */
  for (lp = volumes; lp != NULL; lp = lp->next)
    {
      /* add a new volume shortcut */
      thunar_shortcuts_model_volume_added (model, lp->data, model->volume_monitor);
    }

  /* release the volume list */
  g_list_free (volumes);

  /* get a list of all mounts available */
  mounts = g_volume_monitor_get_mounts (model->volume_monitor);

  /* create shortcuts for the mounts */
  for (lp = mounts; lp != NULL; lp = lp->next)
    {
      /* add a new mount shortcut */
      thunar_shortcuts_model_mount_added (model, lp->data, model->volume_monitor);
    }

  /* release the mount list */
  g_list_free (mounts);

  /* be notified of new and removed volumes on the system */
  g_signal_connect_swapped (model->volume_monitor, "volume-added",
                            G_CALLBACK (thunar_shortcuts_model_volume_added), model);
  g_signal_connect_swapped (model->volume_monitor, "volume-removed",
                            G_CALLBACK (thunar_shortcuts_model_volume_removed), model);
  g_signal_connect_swapped (model->volume_monitor, "mount-added",
                            G_CALLBACK (thunar_shortcuts_model_mount_added), model);
  g_signal_connect_swapped (model->volume_monitor, "mount-removed",
                            G_CALLBACK (thunar_shortcuts_model_mount_removed), model);

  /* reset the load idle ID */
  model->load_idle_id = 0;

  return FALSE;
};



static void
thunar_shortcuts_model_volume_added (ThunarShortcutsModel *model,
                                     GVolume              *volume,
                                     GVolumeMonitor       *monitor)
{
  ThunarShortcut *shortcut;
  gboolean        hidden = FALSE;
  GIcon          *eject_icon;

  _thunar_return_if_fail (THUNAR_IS_SHORTCUTS_MODEL (model));
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

  /* add the shortcut to the model */
  thunar_shortcuts_model_add_shortcut (model, shortcut);

  /* release volume information */
  g_object_unref (eject_icon);
}



static void
thunar_shortcuts_model_volume_removed (ThunarShortcutsModel *model,
                                       GVolume              *volume,
                                       GVolumeMonitor       *monitor)
{
  ThunarShortcutCategory *category;
  GtkTreePath            *path;
  gint                    category_index;
  gint                    shortcut_index;
  
  _thunar_return_if_fail (THUNAR_IS_SHORTCUTS_MODEL (model));
  _thunar_return_if_fail (G_IS_VOLUME (volume));
  _thunar_return_if_fail (G_IS_VOLUME_MONITOR (monitor));

  /* try to find the shortcut for this volume */
  if (thunar_shortcuts_model_find_volume (model, volume,
                                          &category_index, &shortcut_index))
    {
      /* create a tree path for the row */
      path = gtk_tree_path_new_from_indices (category_index, shortcut_index, -1);

      /* notify others that the shortcut was removed */
      g_signal_emit_by_name (model, "row-deleted", path, NULL);

      /* get the category and remove the shortcut from it */
      category = g_ptr_array_index (model->categories, category_index);
      g_ptr_array_remove_index (category->shortcuts, shortcut_index);

      /* release the tree path */
      gtk_tree_path_free (path);
    }
}



static void
thunar_shortcuts_model_mount_added (ThunarShortcutsModel *model,
                                    GMount               *mount,
                                    GVolumeMonitor       *monitor)
{
  ThunarShortcut *shortcut;
  GVolume        *volume;
  GFile          *location;
  GIcon          *eject_icon;

  _thunar_return_if_fail (THUNAR_IS_SHORTCUTS_MODEL (model));
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

      /* create a shortcut for the mount */
      shortcut = g_object_new (THUNAR_TYPE_SHORTCUT,
                               "shortcut-type", THUNAR_SHORTCUT_REGULAR_MOUNT,
                               "location", location,
                               "mount", mount,
                               "eject-icon", eject_icon,
                               "hidden", FALSE,
                               "mutable", FALSE,
                               "persistent", FALSE,
                               NULL);

      /* add the shortcut to the model */
      thunar_shortcuts_model_add_shortcut (model, shortcut);

      /* release volume information */
      g_object_unref (eject_icon);
      g_object_unref (location);
    }
}



static void
thunar_shortcuts_model_mount_removed (ThunarShortcutsModel *model,
                                      GMount               *mount,
                                      GVolumeMonitor       *monitor)
{
  ThunarShortcutCategory *category;
  GtkTreePath            *path;
  GVolume                *volume;
  gint                    category_index;
  gint                    shortcut_index;
  
  _thunar_return_if_fail (THUNAR_IS_SHORTCUTS_MODEL (model));
  _thunar_return_if_fail (G_IS_MOUNT (mount));
  _thunar_return_if_fail (G_IS_VOLUME_MONITOR (monitor));

  /* ignore mounts that have a volume */
  volume = g_mount_get_volume (mount);
  if (volume != NULL)
    {
      /* release the volume again */
      g_object_unref (volume);
    }
  else
    {
      /* try to find the shortcut for this volume */
      if (thunar_shortcuts_model_find_mount (model, mount, 
                                             &category_index, &shortcut_index))
        {
          /* create a tree path for the row */
          path = gtk_tree_path_new_from_indices (category_index, shortcut_index, -1);

          /* notify others that the shortcut was removed */
          g_signal_emit_by_name (model, "row-deleted", path, NULL);

          /* get the category and remove the shortcut from it */
          category = g_ptr_array_index (model->categories, category_index);
          g_ptr_array_remove_index (category->shortcuts, shortcut_index);

          /* release the tree path */
          gtk_tree_path_free (path);
        }
    }
}



static ThunarShortcutCategory *
thunar_shortcut_category_new (ThunarShortcutCategoryType type)
{
  ThunarShortcutCategory *category;

  /* allocate a new category */
  category = g_slice_new0 (ThunarShortcutCategory);

  /* set its type */
  category->type = type;

  /* set its name */
  switch (type)
    {
    case THUNAR_SHORTCUT_CATEGORY_DEVICES:
      category->name = g_strdup (_("DEVICES"));
      break;

    case THUNAR_SHORTCUT_CATEGORY_PLACES:
      category->name = g_strdup (_("PLACES"));
      break;

    case THUNAR_SHORTCUT_CATEGORY_NETWORK:
      category->name = g_strdup (_("NETWORK"));
      break;

    default:
      _thunar_assert_not_reached ();
      break;
    }

  /* assume the category is visible by default */
  category->visible = TRUE;

  /* allocate an empty array for the shortcuts in the category */
  category->shortcuts = g_ptr_array_new_with_free_func (g_object_unref);

  return category;
}



static void
thunar_shortcut_category_free (ThunarShortcutCategory *category)
{
  _thunar_return_if_fail (category != NULL);

  /* do nothing if the category has already been released */
  if (category == NULL)
    return;

  /* release the category name */
  g_free (category->name);

  /* release the array of shortcuts */
  g_ptr_array_free (category->shortcuts, TRUE);

  /* release the category itself */
  g_slice_free (ThunarShortcutCategory, category);
}



/* Reads the current xdg user dirs locale from ~/.config/xdg-user-dirs.locale
 * Notice that the result shall be freed by using g_free (). */
gchar *
_thunar_get_xdg_user_dirs_locale (void)
{
  gchar *file    = NULL;
  gchar *content = NULL;
  gchar *locale  = NULL;

  /* get the file pathname */
  file = g_build_filename (g_get_user_config_dir (), LOCALE_FILE_NAME, NULL);

  /* grab the contents and get ride of the surrounding spaces */
  if (g_file_get_contents (file, &content, NULL, NULL))
    locale = g_strdup (g_strstrip (content));

  g_free (content);
  g_free (file);

  /* if we got nothing, let's set the default locale as C */
  if (exo_str_is_equal (locale, ""))
    {
      g_free (locale);
      locale = g_strdup ("C");
    }

  return locale;
}



/**
 * thunar_shortcuts_model_get_default:
 *
 * Returns the default #ThunarShortcutsModel instance shared by
 * all #ThunarShortcutsView instances.
 *
 * Call #g_object_unref() on the returned object when you
 * don't need it any longer.
 *
 * Return value: the default #ThunarShortcutsModel instance.
 **/
ThunarShortcutsModel*
thunar_shortcuts_model_get_default (void)
{
  static ThunarShortcutsModel *model = NULL;

  if (G_UNLIKELY (model == NULL))
    {
      model = g_object_new (THUNAR_TYPE_SHORTCUTS_MODEL, NULL);
      g_object_add_weak_pointer (G_OBJECT (model), (gpointer) &model);
    }
  else
    {
      g_object_ref (G_OBJECT (model));
    }

  return model;
}
