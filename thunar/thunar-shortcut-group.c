/* vi:set et ai sw=2 sts=2 ts=2: */
/*-
 * Copyright (c) 2011 Jannis Pohlmann <jannis@xfce.org>
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

#include <glib.h>
#include <glib-object.h>

#include <gtk/gtk.h>

#include <thunar/thunar-enum-types.h>
#include <thunar/thunar-private.h>
#include <thunar/thunar-shortcut-group.h>



#define THUNAR_SHORTCUT_GROUP_FLASH_TIMEOUT 300
#define THUNAR_SHORTCUT_GROUP_FLASH_TIMES     3



/* property identifiers */
enum
{
  PROP_0,
  PROP_LABEL,
  PROP_ACCCEPTED_TYPES,
};



static void     thunar_shortcut_group_constructed       (GObject                *object);
static void     thunar_shortcut_group_dispose           (GObject                *object);
static void     thunar_shortcut_group_finalize          (GObject                *object);
static void     thunar_shortcut_group_get_property      (GObject                *object,
                                                         guint                   prop_id,
                                                         GValue                 *value,
                                                         GParamSpec             *pspec);
static void     thunar_shortcut_group_set_property      (GObject                *object,
                                                         guint                   prop_id,
                                                         const GValue           *value,
                                                         GParamSpec             *pspec);
static void     thunar_shortcut_group_attract_attention (ThunarShortcutGroup    *group);
static gboolean thunar_shortcut_group_flash_expander    (gpointer                user_data);
static void     thunar_shortcut_group_expand_style_set  (ThunarShortcutGroup    *group,
                                                         GtkStyle               *style,
                                                         GtkWidget              *expander);
static void     thunar_shortcut_group_apply_indentation (ThunarShortcutGroup    *group,
                                                         ThunarShortcut         *shortcut);



struct _ThunarShortcutGroupClass
{
  GtkEventBoxClass __parent__;
};

struct _ThunarShortcutGroup
{
  GtkEventBox        __parent__;

  ThunarShortcutType accepted_types;

  GtkWidget         *expander;
  GtkWidget         *shortcuts;

  guint              flash_idle_id;
  guint              flash_count;
};



G_DEFINE_TYPE (ThunarShortcutGroup, thunar_shortcut_group, GTK_TYPE_EVENT_BOX)



static void
thunar_shortcut_group_class_init (ThunarShortcutGroupClass *klass)
{
  GObjectClass *gobject_class;

  /* Determine the parent type class */
  thunar_shortcut_group_parent_class = g_type_class_peek_parent (klass);

  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->constructed = thunar_shortcut_group_constructed; 
  gobject_class->dispose = thunar_shortcut_group_dispose; 
  gobject_class->finalize = thunar_shortcut_group_finalize; 
  gobject_class->get_property = thunar_shortcut_group_get_property;
  gobject_class->set_property = thunar_shortcut_group_set_property;

  g_object_class_install_property (gobject_class,
                                   PROP_LABEL,
                                   g_param_spec_string ("label",
                                                        "label",
                                                        "label",
                                                        NULL,
                                                        G_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));

  g_object_class_install_property (gobject_class,
                                   PROP_ACCCEPTED_TYPES,
                                   g_param_spec_flags ("accepted-types",
                                                       "accepted-types",
                                                       "accepted-types",
                                                       THUNAR_TYPE_SHORTCUT_TYPE,
                                                       0,
                                                       G_PARAM_READWRITE |
                                                       G_PARAM_CONSTRUCT_ONLY));
}



static void
thunar_shortcut_group_init (ThunarShortcutGroup *group)
{
  /* create the expander for this group */
  group->expander = gtk_expander_new (NULL);
  gtk_expander_set_use_markup (GTK_EXPANDER (group->expander), TRUE);
  /* TODO make this depend on the last remembered state: */
  gtk_expander_set_expanded (GTK_EXPANDER (group->expander), TRUE);
  gtk_container_set_border_width (GTK_CONTAINER (group->expander), 0);
  gtk_expander_set_spacing (GTK_EXPANDER (group->expander), 0);
  gtk_container_add (GTK_CONTAINER (group), group->expander);
  gtk_widget_show (group->expander);

  g_signal_connect_swapped (group->expander, "style-set",
                            G_CALLBACK (thunar_shortcut_group_expand_style_set),
                            group);

  /* add a box for the individual shortcuts */
  group->shortcuts = gtk_vbox_new (TRUE, 0);
  gtk_container_add (GTK_CONTAINER (group->expander), group->shortcuts);
  gtk_widget_show (group->shortcuts);
}



static void
thunar_shortcut_group_constructed (GObject *object)
{
#if 0
  ThunarShortcutGroup *group = THUNAR_SHORTCUT_GROUP (object);
#endif
}



static void
thunar_shortcut_group_dispose (GObject *object)
{
#if 0
  ThunarShortcutGroup *group = THUNAR_SHORTCUT_GROUP (object);
#endif

  (*G_OBJECT_CLASS (thunar_shortcut_group_parent_class)->dispose) (object);
}



static void
thunar_shortcut_group_finalize (GObject *object)
{
  ThunarShortcutGroup *group = THUNAR_SHORTCUT_GROUP (object);

  /* abort flashing if still active */
  if (group->flash_idle_id > 0)
    g_source_remove (group->flash_idle_id);

  (*G_OBJECT_CLASS (thunar_shortcut_group_parent_class)->finalize) (object);
}



static void
thunar_shortcut_group_get_property (GObject    *object,
                                    guint       prop_id,
                                    GValue     *value,
                                    GParamSpec *pspec)
{
  ThunarShortcutGroup *group = THUNAR_SHORTCUT_GROUP (object);
  const gchar         *label;

  switch (prop_id)
    {
    case PROP_LABEL:
      label = gtk_expander_get_label (GTK_EXPANDER (group->expander));
      g_value_set_string (value, label);
      break;
    case PROP_ACCCEPTED_TYPES:
      g_value_set_flags (value, group->accepted_types);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}



static void
thunar_shortcut_group_set_property (GObject      *object,
                                    guint         prop_id,
                                    const GValue *value,
                                    GParamSpec   *pspec)
{
  ThunarShortcutGroup *group = THUNAR_SHORTCUT_GROUP (object);
  static const gchar  *markup_format = "<span size='medium' weight='bold' color='#353535'>%s</span>";
  const gchar         *label;
  gchar               *label_markup;

  switch (prop_id)
    {
    case PROP_LABEL:
      label = g_value_get_string (value);
      label_markup = g_markup_printf_escaped (markup_format, label);
      gtk_expander_set_label (GTK_EXPANDER (group->expander), label_markup);
      g_free (label_markup);
      break;
    case PROP_ACCCEPTED_TYPES:
      group->accepted_types = g_value_get_flags (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}



static void
thunar_shortcut_group_attract_attention (ThunarShortcutGroup *group)
{
  _thunar_return_if_fail (THUNAR_IS_SHORTCUT_GROUP (group));

  /* check if we are in collapsed mode */
  if (!gtk_expander_get_expanded (GTK_EXPANDER (group->expander)))
    {
      /* abort active flash idle handlers */
      if (group->flash_idle_id > 0)
        g_source_remove (group->flash_idle_id);

      /* reset the flash counter to 0 */
      group->flash_count = 0;

      /* reschedule the flash idle */
      group->flash_idle_id = g_timeout_add (THUNAR_SHORTCUT_GROUP_FLASH_TIMEOUT,
                                            thunar_shortcut_group_flash_expander,
                                            group);
    }
}



static gboolean
thunar_shortcut_group_flash_expander (gpointer user_data)
{
  ThunarShortcutGroup *group = THUNAR_SHORTCUT_GROUP (user_data);

  _thunar_return_val_if_fail (THUNAR_IS_SHORTCUT_GROUP (group), FALSE);

  /* change the base and background color */
  if ((group->flash_count % 2) == 0)
    {
      gtk_widget_set_state (GTK_WIDGET (group), GTK_STATE_PRELIGHT);
      gtk_widget_queue_draw (GTK_WIDGET (group));
    }
  else
    {
      gtk_widget_set_state (GTK_WIDGET (group), GTK_STATE_NORMAL);
      gtk_widget_queue_draw (GTK_WIDGET (group));
    }

  /* increment the flash counter */
  group->flash_count += 1;

  /* abort if we have flashed several times */
  if (group->flash_count >= (THUNAR_SHORTCUT_GROUP_FLASH_TIMES * 2))
    {
      /* reset the event box color */
      gtk_widget_set_state (GTK_WIDGET (group), GTK_STATE_NORMAL);
      gtk_widget_queue_draw (GTK_WIDGET (group));

      /* clear the flash counter */
      group->flash_count = 0;

      /* clear the idle handler */
      group->flash_idle_id = 0;

      /* abort the timeout */
      return FALSE;
    }
  else
    {
      /* flash one more time */
      return TRUE;
    }
}



static void
thunar_shortcut_group_expand_style_set (ThunarShortcutGroup *group,
                                        GtkStyle            *style,
                                        GtkWidget           *expander)
{
  GList *children;
  GList *iter;

  _thunar_return_if_fail (THUNAR_IS_SHORTCUT_GROUP (group));
  _thunar_return_if_fail (GTK_IS_EXPANDER (expander));
  _thunar_return_if_fail (expander == group->expander);
  
  children = gtk_container_get_children (GTK_CONTAINER (group->shortcuts));

  for (iter = children; iter != NULL; iter = iter->next)
    thunar_shortcut_group_apply_indentation (group, THUNAR_SHORTCUT (iter->data));

  g_list_free (children);
}



static void
thunar_shortcut_group_apply_indentation (ThunarShortcutGroup *group,
                                         ThunarShortcut      *shortcut)
{
  GtkWidget *alignment;
  gint       expander_size;
  gint       expander_spacing;

  _thunar_return_if_fail (THUNAR_IS_SHORTCUT_GROUP (group));
  _thunar_return_if_fail (THUNAR_IS_SHORTCUT (shortcut));

  /* get information about the expander arrow size */
  gtk_widget_style_get (group->expander,
                        "expander-size", &expander_size,
                        "expander-spacing", &expander_spacing,
                        NULL);

  /* apply the indentation to the shortcut alignment */
  alignment = thunar_shortcut_get_alignment (shortcut);
  g_object_set (alignment,
                "left-padding", expander_size + expander_spacing * 2 + 2,
                NULL);
}



GtkWidget *
thunar_shortcut_group_new (const gchar       *label,
                           ThunarShortcutType accepted_types)
{
  _thunar_return_val_if_fail (label != NULL && *label != '\0', NULL);

  return g_object_new (THUNAR_TYPE_SHORTCUT_GROUP,
                       "label", label,
                       "accepted-types", accepted_types,
                       NULL);
}



gboolean
thunar_shortcut_group_try_add_shortcut (ThunarShortcutGroup *group,
                                        ThunarShortcut      *shortcut)
{
  ThunarShortcutType shortcut_type;

  _thunar_return_val_if_fail (THUNAR_IS_SHORTCUT_GROUP (group), FALSE);
  _thunar_return_val_if_fail (THUNAR_IS_SHORTCUT (shortcut), FALSE);

  /* get the type of the shortcut */
  shortcut_type = thunar_shortcut_get_shortcut_type (shortcut);

  /* abort if the shortcut does not belong to this group */
  if ((group->accepted_types & shortcut_type) == 0)
    return FALSE;

  /* TODO find the sorting, remembered or user-defined position for the shortcut */

  /* properly align the shortcut contents with the group title */
  thunar_shortcut_group_apply_indentation (group, shortcut);

  gtk_box_pack_start (GTK_BOX (group->shortcuts), GTK_WIDGET (shortcut), FALSE, TRUE, 0);
  gtk_widget_show (GTK_WIDGET (shortcut));

  /* notify the user that a new item was added to this group */
  thunar_shortcut_group_attract_attention (group);

  return TRUE;
}



void
thunar_shortcut_group_unselect_shortcuts (ThunarShortcutGroup *group,
                                          ThunarShortcut      *exception)
{
  ThunarShortcut *shortcut;
  GList          *children;
  GList          *iter;

  _thunar_return_if_fail (THUNAR_IS_SHORTCUT_GROUP (group));
  _thunar_return_if_fail (exception == NULL || THUNAR_IS_SHORTCUT (exception));

  children = gtk_container_get_children (GTK_CONTAINER (group->shortcuts));

  for (iter = children; iter != NULL; iter = iter->next)
    {
      shortcut = THUNAR_SHORTCUT (iter->data);

      if (shortcut != exception
          && gtk_widget_get_state (GTK_WIDGET (shortcut)) == GTK_STATE_SELECTED)
        {
          gtk_widget_set_state (GTK_WIDGET (shortcut), GTK_STATE_NORMAL);
        }
    }

  g_list_free (children);
}



void
thunar_shortcut_group_unprelight_shortcuts (ThunarShortcutGroup *group,
                                            ThunarShortcut      *exception)
{
  ThunarShortcut *shortcut;
  GList          *children;
  GList          *iter;

  _thunar_return_if_fail (THUNAR_IS_SHORTCUT_GROUP (group));
  _thunar_return_if_fail (exception == NULL || THUNAR_IS_SHORTCUT (exception));

  children = gtk_container_get_children (GTK_CONTAINER (group->shortcuts));

  for (iter = children; iter != NULL; iter = iter->next)
    {
      shortcut = THUNAR_SHORTCUT (iter->data);

      if (shortcut != exception
          && gtk_widget_get_state (GTK_WIDGET (shortcut)) == GTK_STATE_PRELIGHT)
        {
          gtk_widget_set_state (GTK_WIDGET (shortcut), GTK_STATE_NORMAL);
        }
    }

  g_list_free (children);
}



void
thunar_shortcut_group_cancel_activations (ThunarShortcutGroup *group,
                                          ThunarShortcut      *exception)
{
  ThunarShortcut *shortcut;
  GList          *children;
  GList          *iter;

  _thunar_return_if_fail (THUNAR_IS_SHORTCUT_GROUP (group));
  _thunar_return_if_fail (exception == NULL || THUNAR_IS_SHORTCUT (exception));

  children = gtk_container_get_children (GTK_CONTAINER (group->shortcuts));

  for (iter = children; iter != NULL; iter = iter->next)
    {
      shortcut = THUNAR_SHORTCUT (iter->data);

      if (shortcut != exception)
        thunar_shortcut_cancel_activation (shortcut);
    }

  g_list_free (children);
}



void
thunar_shortcut_group_update_selection (ThunarShortcutGroup *group,
                                        ThunarFile          *file)
{
  ThunarShortcut *shortcut;
  GList          *children;
  GList          *iter;

  _thunar_return_if_fail (THUNAR_IS_SHORTCUT_GROUP (group));
  _thunar_return_if_fail (THUNAR_IS_FILE (file));

  children = gtk_container_get_children (GTK_CONTAINER (group->shortcuts));

  for (iter = children; iter != NULL; iter = iter->next)
    {
      shortcut = THUNAR_SHORTCUT (iter->data);

      if (thunar_shortcut_matches_file (shortcut, file))
        gtk_widget_set_state (GTK_WIDGET (shortcut), GTK_STATE_SELECTED);
      else if (gtk_widget_get_state (GTK_WIDGET (shortcut)) == GTK_STATE_SELECTED)
        gtk_widget_set_state (GTK_WIDGET (shortcut), GTK_STATE_NORMAL);
    }

  g_list_free (children);
}



void
thunar_shortcut_group_remove_volume_shortcut (ThunarShortcutGroup *group,
                                              GVolume             *volume)
{
  ThunarShortcut *shortcut;
  GVolume        *shortcut_volume;
  GList          *children;
  GList          *iter;

  _thunar_return_if_fail (THUNAR_IS_SHORTCUT_GROUP (group));
  _thunar_return_if_fail (G_IS_VOLUME (volume));

  children = gtk_container_get_children (GTK_CONTAINER (group->shortcuts));

  for (iter = children; iter != NULL; iter = iter->next)
    {
      shortcut = THUNAR_SHORTCUT (iter->data);
      shortcut_volume = thunar_shortcut_get_volume (shortcut);

      if (shortcut_volume == volume)
        gtk_container_remove (GTK_CONTAINER (group->shortcuts),
                              GTK_WIDGET (shortcut));
    }

  g_list_free (children);
}



void
thunar_shortcut_group_remove_mount_shortcut (ThunarShortcutGroup *group,
                                             GMount              *mount)
{
  ThunarShortcut *shortcut;
  GMount        *shortcut_mount;
  GList          *children;
  GList          *iter;

  _thunar_return_if_fail (THUNAR_IS_SHORTCUT_GROUP (group));
  _thunar_return_if_fail (G_IS_MOUNT (mount));

  children = gtk_container_get_children (GTK_CONTAINER (group->shortcuts));

  for (iter = children; iter != NULL; iter = iter->next)
    {
      shortcut = THUNAR_SHORTCUT (iter->data);

      if (!thunar_shortcut_get_persistent (shortcut))
        {
          shortcut_mount = thunar_shortcut_get_mount (shortcut);

          if (shortcut_mount == mount)
            gtk_container_remove (GTK_CONTAINER (group->shortcuts),
                                  GTK_WIDGET (shortcut));
        }
    }

  g_list_free (children);
}



gboolean
thunar_shortcut_group_find_shortcut_by_file (ThunarShortcutGroup *group,
                                             ThunarFile          *file,
                                             ThunarShortcut     **result)
{
  ThunarShortcut *shortcut;
  gboolean         has_shortcut = FALSE;
  GList           *children;
  GList           *iter;

  _thunar_return_val_if_fail (THUNAR_IS_SHORTCUT_GROUP (group), FALSE);
  _thunar_return_val_if_fail (THUNAR_IS_FILE (file), FALSE);

  children = gtk_container_get_children (GTK_CONTAINER (group->shortcuts));

  if (result != NULL)
    *result = NULL;

  for (iter = children; !has_shortcut && iter != NULL; iter = iter->next)
    {
      shortcut = THUNAR_SHORTCUT (iter->data);

      if (thunar_shortcut_matches_file (shortcut, file))
        {
          if (result != NULL)
            *result = shortcut;

          has_shortcut = TRUE;
        }
    }

  g_list_free (children);

  return has_shortcut;
}
