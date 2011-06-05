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
#include <thunar/thunar-shortcuts-model.h>
#include <thunar/thunar-private.h>



#define THUNAR_SHORTCUT(obj) ((ThunarShortcut *) (obj))



/* I don't particularly like this here, but it's shared across a few
 * files. */
const gchar *_thunar_user_directory_names[9] = {
  "Desktop", "Documents", "Download", "Music", "Pictures", "Public",
  "Templates", "Videos", NULL,
};



typedef struct _ThunarShortcutCategory ThunarShortcutCategory;
typedef struct _ThunarShortcut         ThunarShortcut;



typedef enum
{
  THUNAR_SHORTCUT_SYSTEM_VOLUME,
  THUNAR_SHORTCUT_VOLUME,
  THUNAR_SHORTCUT_USER_DIRECTORY,
  THUNAR_SHORTCUT_LOCAL_FILE,
  THUNAR_SHORTCUT_REMOTE_FILE,
} ThunarShortcutType;



static void                    thunar_shortcuts_model_tree_model_init    (GtkTreeModelIface         *iface);
static void                    thunar_shortcuts_model_finalize           (GObject                   *object);
static GtkTreeModelFlags       thunar_shortcuts_model_get_flags          (GtkTreeModel              *tree_model);
static gint                    thunar_shortcuts_model_get_n_columns      (GtkTreeModel              *tree_model);
static GType                   thunar_shortcuts_model_get_column_type    (GtkTreeModel              *tree_model,
                                                                          gint                       idx);
static gboolean                thunar_shortcuts_model_get_iter           (GtkTreeModel              *tree_model,
                                                                          GtkTreeIter               *iter,
                                                                          GtkTreePath               *path);
static GtkTreePath            *thunar_shortcuts_model_get_path           (GtkTreeModel              *tree_model,
                                                                          GtkTreeIter               *iter);
static void                    thunar_shortcuts_model_get_value          (GtkTreeModel              *tree_model,
                                                                          GtkTreeIter               *iter,
                                                                          gint                       column,
                                                                          GValue                    *value);
static gboolean                thunar_shortcuts_model_iter_next          (GtkTreeModel              *tree_model,
                                                                          GtkTreeIter               *iter);
static gboolean                thunar_shortcuts_model_iter_children      (GtkTreeModel              *tree_model,
                                                                          GtkTreeIter               *iter,
                                                                          GtkTreeIter               *parent);
static gboolean                thunar_shortcuts_model_iter_has_child     (GtkTreeModel              *tree_model,
                                                                          GtkTreeIter               *iter);
static gint                    thunar_shortcuts_model_iter_n_children    (GtkTreeModel              *tree_model,
                                                                          GtkTreeIter               *iter);
static gboolean                thunar_shortcuts_model_iter_nth_child     (GtkTreeModel              *tree_model,
                                                                          GtkTreeIter               *iter,
                                                                          GtkTreeIter               *parent,
                                                                          gint                       n);
static gboolean                thunar_shortcuts_model_iter_parent        (GtkTreeModel              *tree_model,
                                                                          GtkTreeIter               *iter,
                                                                          GtkTreeIter               *child);
static gboolean                thunar_shortcuts_model_parse_path         (ThunarShortcutsModel      *model,
                                                                          GtkTreePath               *path,
                                                                          gint                      *category_index,
                                                                          gint                      *shortcut_index);
static gboolean                thunar_shortcuts_model_parse_iter         (ThunarShortcutsModel      *model,
                                                                          GtkTreeIter               *iter,
                                                                          gint                      *category_index,
                                                                          gint                      *shortcut_index);
static ThunarShortcutCategory *thunar_shortcut_category_new              (const gchar               *name);
static void                    thunar_shortcut_category_free             (ThunarShortcutCategory    *category);
static void                    thunar_shortcut_free                      (ThunarShortcut            *shortcut);



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
};

struct _ThunarShortcutCategory
{
  gchar     *name;
  GPtrArray *shortcuts;
};

struct _ThunarShortcut
{
  ThunarShortcutType type;

  GIcon              *icon;
  GIcon              *eject_icon;

  gchar              *name;
  GFile              *file;
  GVolume            *volume;

  guint               mutable : 1;
  guint               category : 1;
  guint               persistent : 1;
};



G_DEFINE_TYPE_WITH_CODE (ThunarShortcutsModel, thunar_shortcuts_model, G_TYPE_OBJECT,
    G_IMPLEMENT_INTERFACE (GTK_TYPE_TREE_MODEL, thunar_shortcuts_model_tree_model_init))


    
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
  category = thunar_shortcut_category_new (_("DEVICES"));
  g_ptr_array_add (model->categories, category);

  /* create the places category */
  category = thunar_shortcut_category_new (_("PLACES"));
  g_ptr_array_add (model->categories, category);

  /* create the network category */
  category = thunar_shortcut_category_new (_("NETWORK"));
  g_ptr_array_add (model->categories, category);
}



static void
thunar_shortcuts_model_finalize (GObject *object)
{
  ThunarShortcutsModel *model = THUNAR_SHORTCUTS_MODEL (object);

  _thunar_return_if_fail (THUNAR_IS_SHORTCUTS_MODEL (model));

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

    case THUNAR_SHORTCUTS_MODEL_COLUMN_FILE:
      return G_TYPE_FILE;

    case THUNAR_SHORTCUTS_MODEL_COLUMN_VOLUME:
      return G_TYPE_VOLUME;

    case THUNAR_SHORTCUTS_MODEL_COLUMN_MUTABLE:
      return G_TYPE_BOOLEAN;

    case THUNAR_SHORTCUTS_MODEL_COLUMN_EJECT_ICON:
      return G_TYPE_ICON;

    case THUNAR_SHORTCUTS_MODEL_COLUMN_CATEGORY:
      return G_TYPE_BOOLEAN;

    case THUNAR_SHORTCUTS_MODEL_COLUMN_PERSISTENT:
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

  (*iter).stamp = model->stamp;
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
        g_value_set_object (value, shortcut->icon);
      else
        g_value_set_object (value, NULL);
      break;

    case THUNAR_SHORTCUTS_MODEL_COLUMN_NAME:
      g_value_init (value, G_TYPE_STRING);
      if (shortcut != NULL)
        g_value_set_static_string (value, shortcut->name);
      else
        g_value_set_static_string (value, category->name);
      break;

    case THUNAR_SHORTCUTS_MODEL_COLUMN_FILE:
      g_value_init (value, G_TYPE_FILE);
      if (shortcut != NULL)
        g_value_set_object (value, shortcut->file);
      else
        g_value_set_object (value, NULL);
      break;

    case THUNAR_SHORTCUTS_MODEL_COLUMN_VOLUME:
      g_value_init (value, G_TYPE_VOLUME);
      if (shortcut != NULL)
        g_value_set_object (value, shortcut->volume);
      else
        g_value_set_object (value, NULL);
      break;

    case THUNAR_SHORTCUTS_MODEL_COLUMN_MUTABLE:
      g_value_init (value, G_TYPE_BOOLEAN);
      if (shortcut != NULL)
        g_value_set_boolean (value, shortcut->mutable);
      else
        g_value_set_boolean (value, FALSE);
      break;

    case THUNAR_SHORTCUTS_MODEL_COLUMN_EJECT_ICON:
      g_value_init (value, G_TYPE_ICON);
      if (shortcut != NULL)
        g_value_set_object (value, shortcut->eject_icon);
      else
        g_value_set_object (value, NULL);
      break;

    case THUNAR_SHORTCUTS_MODEL_COLUMN_CATEGORY:
      g_value_init (value, G_TYPE_BOOLEAN);
      g_value_set_boolean (value, shortcut != NULL);
      break;

    case THUNAR_SHORTCUTS_MODEL_COLUMN_PERSISTENT:
      g_value_init (value, G_TYPE_BOOLEAN);
      if (shortcut != NULL)
        g_value_set_boolean (value, shortcut->persistent);
      else
        g_value_set_boolean (value, FALSE);
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
          (*iter).user_data = GINT_TO_POINTER (0);
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



static ThunarShortcutCategory *
thunar_shortcut_category_new (const gchar *name)
{
  ThunarShortcutCategory *category;

  /* allocate a new category */
  category = g_slice_new0 (ThunarShortcutCategory);

  /* set its name */
  category->name = g_strdup (name);

  /* allocate an empty array for the shortcuts in the category */
  category->shortcuts = 
    g_ptr_array_new_with_free_func ((GDestroyNotify) thunar_shortcut_free);

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



static void
thunar_shortcut_free (ThunarShortcut *shortcut)
{
  /* release the shortcut icon */
  if (shortcut->icon != NULL)
    g_object_unref (shortcut->icon);

  /* release the eject icon */
  if (shortcut->eject_icon != NULL)
    g_object_unref (shortcut->eject_icon);

  /* release the shortcut name */
  g_free (shortcut->name);

  /* release the shortcut file */
  if (shortcut->file != NULL)
    g_object_unref (shortcut->file);

  /* release the shortcut volume */
  if (shortcut->volume != NULL)
    g_object_unref (shortcut->volume);

  /* release the shortcut itself */
  g_slice_free (ThunarShortcut, shortcut);
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
