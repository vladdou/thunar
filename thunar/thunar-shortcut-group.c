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



static void     thunar_shortcut_group_constructed          (GObject                *object);
static void     thunar_shortcut_group_dispose              (GObject                *object);
static void     thunar_shortcut_group_finalize             (GObject                *object);
static void     thunar_shortcut_group_get_property         (GObject                *object,
                                                            guint                   prop_id,
                                                            GValue                 *value,
                                                            GParamSpec             *pspec);
static void     thunar_shortcut_group_set_property         (GObject                *object,
                                                            guint                   prop_id,
                                                            const GValue           *value,
                                                            GParamSpec             *pspec);
static void     thunar_shortcut_group_drag_data_received   (GtkWidget              *widget,
                                                            GdkDragContext         *context,
                                                            gint                    x,
                                                            gint                    y,
                                                            GtkSelectionData       *data,
                                                            guint                   info,
                                                            guint                   timestamp);
static gboolean thunar_shortcut_group_drag_drop            (GtkWidget              *widget,
                                                            GdkDragContext         *context,
                                                            gint                    x,
                                                            gint                    y,
                                                            guint                   timestamp);
static void     thunar_shortcut_group_drag_leave           (GtkWidget              *widget,
                                                            GdkDragContext         *context,
                                                            guint                   timestamp);
static gboolean thunar_shortcut_group_drag_motion          (GtkWidget              *widget,
                                                            GdkDragContext         *context,
                                                            gint                    x,
                                                            gint                    y,
                                                            guint                   timestamp);
static void     thunar_shortcut_group_attract_attention    (ThunarShortcutGroup    *group);
static gboolean thunar_shortcut_group_flash_expander       (gpointer                user_data);
static void     thunar_shortcut_group_expand_style_set     (ThunarShortcutGroup    *group,
                                                            GtkStyle               *style,
                                                            GtkWidget              *expander);
static void     thunar_shortcut_group_apply_indentation    (ThunarShortcutGroup    *group,
                                                            ThunarShortcut         *shortcut);
static gboolean thunar_shortcut_group_find_child_by_coords (ThunarShortcutGroup    *group,
                                                            gint                    x,
                                                            gint                    y,
                                                            GtkWidget             **child,
                                                            gint                   *child_index);



struct _ThunarShortcutGroupClass
{
  GtkExpanderClass __parent__;
};

struct _ThunarShortcutGroup
{
  GtkExpander        __parent__;

  ThunarShortcutType accepted_types;

  GtkWidget         *shortcuts;
  GtkSizeGroup      *size_group;
  GtkWidget         *empty_spot;

  guint              flash_idle_id;
  guint              flash_count;

  guint              drop_data_ready : 1;
  guint              drop_occurred : 1;
  ThunarShortcut    *drop_shortcut;
};



G_DEFINE_TYPE (ThunarShortcutGroup, thunar_shortcut_group, GTK_TYPE_EXPANDER)



static const GtkTargetEntry drop_targets[] =
{
  { "text/uri-list", 0, THUNAR_DND_TARGET_TEXT_URI_LIST },
  { "THUNAR_DND_TARGET_SHORTCUT", GTK_TARGET_SAME_APP, THUNAR_DND_TARGET_SHORTCUT },
};



static void
thunar_shortcut_group_class_init (ThunarShortcutGroupClass *klass)
{
  GtkWidgetClass *gtkwidget_class;
  GObjectClass   *gobject_class;

  /* Determine the parent type class */
  thunar_shortcut_group_parent_class = g_type_class_peek_parent (klass);

  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->constructed = thunar_shortcut_group_constructed; 
  gobject_class->dispose = thunar_shortcut_group_dispose; 
  gobject_class->finalize = thunar_shortcut_group_finalize; 
  gobject_class->get_property = thunar_shortcut_group_get_property;
  gobject_class->set_property = thunar_shortcut_group_set_property;

  gtkwidget_class = GTK_WIDGET_CLASS (klass);
  gtkwidget_class->drag_data_received = thunar_shortcut_group_drag_data_received;
  gtkwidget_class->drag_drop = thunar_shortcut_group_drag_drop;
  gtkwidget_class->drag_leave = thunar_shortcut_group_drag_leave;
  gtkwidget_class->drag_motion = thunar_shortcut_group_drag_motion;

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
  g_signal_connect_swapped (group, "style-set",
                            G_CALLBACK (thunar_shortcut_group_expand_style_set),
                            group);

  /* add a box for the individual shortcuts */
  group->shortcuts = gtk_vbox_new (TRUE, 0);
  gtk_container_add (GTK_CONTAINER (group), group->shortcuts);
  gtk_widget_show (group->shortcuts);

  /* set the group up as a drop site to allow shortcuts to be re-ordered by DND */
  gtk_drag_dest_set (GTK_WIDGET (group), 0,
                     drop_targets, G_N_ELEMENTS (drop_targets),
                     GDK_ACTION_MOVE);
}



static void
thunar_shortcut_group_constructed (GObject *object)
{
  ThunarShortcutGroup *group = THUNAR_SHORTCUT_GROUP (object);

  /* configure the look and feel of the expander */
  gtk_expander_set_use_markup (GTK_EXPANDER (group), TRUE);
  /* TODO make this depend on the last remembered state: */
  gtk_expander_set_expanded (GTK_EXPANDER (group), TRUE);
  gtk_container_set_border_width (GTK_CONTAINER (group), 0);
  gtk_expander_set_spacing (GTK_EXPANDER (group), 0);

  /* create a size group to enforce the same size for all shortcuts */
  group->size_group = gtk_size_group_new (GTK_SIZE_GROUP_BOTH);

  /* create an empty label to indicate an empty spot when re-ordering 
   * shortcuts via DND */
  group->empty_spot = gtk_image_new ();
  g_object_ref (group->empty_spot);
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

  /* release the empty spot label */
  g_object_unref (group->empty_spot);

  /* release the size group */
  g_object_unref (group->size_group);

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
      label = gtk_expander_get_label (GTK_EXPANDER (group));
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
      gtk_expander_set_label (GTK_EXPANDER (group), label_markup);
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
thunar_shortcut_group_drag_data_received (GtkWidget        *widget,
                                          GdkDragContext   *context,
                                          gint              x,
                                          gint              y,
                                          GtkSelectionData *data,
                                          guint             info,
                                          guint             timestamp)
{
  ThunarShortcutGroup *group = THUNAR_SHORTCUT_GROUP (widget);
  GtkWidget           *shortcut;
  GList               *children;
  GList               *iter;
  gint                 empty_spot_index;

  _thunar_return_if_fail (THUNAR_IS_SHORTCUT_GROUP (widget));
  _thunar_return_if_fail (GDK_IS_DRAG_CONTEXT (context));

  if (info == THUNAR_DND_TARGET_SHORTCUT)
    {
      /* only read data if we don't have it yet */
      if (!group->drop_data_ready)
        {
          group->drop_shortcut = THUNAR_SHORTCUT (gtk_drag_get_source_widget (context));
          group->drop_data_ready = TRUE;
        }

      if (group->drop_occurred)
        {
          group->drop_occurred = FALSE;

          children = gtk_container_get_children (GTK_CONTAINER (group->shortcuts));
          iter = g_list_find (children, group->empty_spot);
          empty_spot_index = g_list_position (children, iter);
          g_list_free (children);

          shortcut = gtk_drag_get_source_widget (context);
          gtk_box_reorder_child (GTK_BOX (group->shortcuts), shortcut, empty_spot_index);

          thunar_shortcut_group_drag_leave (widget, context, timestamp);

          gtk_drag_finish (context, TRUE, FALSE, timestamp);
        }
    }
  else if (info == THUNAR_DND_TARGET_TEXT_URI_LIST)
    {
      /* only read data if we don't have it yet */
      if (!group->drop_data_ready)
        {
        }

      if (group->drop_occurred)
        {
          group->drop_occurred = FALSE;
        }
    }
}



static gboolean
thunar_shortcut_group_drag_drop (GtkWidget      *widget,
                                 GdkDragContext *context,
                                 gint            x,
                                 gint            y,
                                 guint           timestamp)
{
  ThunarShortcutGroup *group = THUNAR_SHORTCUT_GROUP (widget);
  GdkAtom              target;

  _thunar_return_val_if_fail (THUNAR_IS_SHORTCUT_GROUP (widget), FALSE);
  _thunar_return_val_if_fail (GDK_IS_DRAG_CONTEXT (context), FALSE);

  target = gtk_drag_dest_find_target (widget, context, NULL);
  if (target == gdk_atom_intern_static_string ("THUNAR_DND_TARGET_SHORTCUT"))
    {
      /* set the state so that drag_data_received knows that it needs to drop now */
      group->drop_occurred = TRUE;

      /* request the drag data from the source and perform the drop */
      gtk_drag_get_data (widget, context, target, timestamp);

      return TRUE;
    }
  else
    {
      /* we cannot handle the drop */
      return FALSE;
    }
}



static void
thunar_shortcut_group_drag_leave (GtkWidget      *widget,
                                  GdkDragContext *context,
                                  guint           timestamp)
{
  ThunarShortcutGroup *group = THUNAR_SHORTCUT_GROUP (widget);

  _thunar_return_if_fail (THUNAR_IS_SHORTCUT_GROUP (widget));
  _thunar_return_if_fail (GDK_IS_DRAG_CONTEXT (context));

  /* reset DND state information */
  if (group->drop_data_ready)
    {
      group->drop_data_ready = FALSE;
      group->drop_occurred = FALSE;
      group->drop_shortcut = NULL;
    }

  /* hide the empty spot indicator */
  gtk_widget_hide (group->empty_spot);

  /* call the parent's drag_leave() handler */
  if (GTK_WIDGET_CLASS (thunar_shortcut_group_parent_class)->drag_leave != NULL)
    (*GTK_WIDGET_CLASS (thunar_shortcut_group_parent_class)->drag_leave) (widget, context, timestamp);
}



static gboolean
thunar_shortcut_group_drag_motion (GtkWidget      *widget,
                                   GdkDragContext *context,
                                   gint            x,
                                   gint            y,
                                   guint           timestamp)
{
  ThunarShortcutGroup *group = THUNAR_SHORTCUT_GROUP (widget);
  GtkAllocation        allocation;
  GdkDragAction        actions = 0;
  GdkPixmap           *pixmap;
  GtkWidget           *child = NULL;
  GdkAtom              target;
  gint                 child_index;

  _thunar_return_val_if_fail (THUNAR_IS_SHORTCUT_GROUP (widget), FALSE);
  _thunar_return_val_if_fail (GDK_IS_DRAG_CONTEXT (context), FALSE);

  /* call the parent's drag_motion() handler */
  if (GTK_WIDGET_CLASS (thunar_shortcut_group_parent_class)->drag_motion != NULL)
    (*GTK_WIDGET_CLASS (thunar_shortcut_group_parent_class)->drag_motion) (widget, context, x, y, timestamp);

  /* determine the drag target */
  target = gtk_drag_dest_find_target (widget, context, NULL);

  if (target == gdk_atom_intern_static_string ("THUNAR_DND_TARGET_SHORTCUT"))
    {
      /* request the drop data on demand */
      if (!group->drop_data_ready)
        {
          /* request the drop data from the source */
          gtk_drag_get_data (widget, context, target, timestamp);
          if (!gtk_widget_is_ancestor (GTK_WIDGET (group->drop_shortcut), widget))
            return FALSE;
          return TRUE;
        }
      else
        {
          if (!gtk_widget_is_ancestor (GTK_WIDGET (group->drop_shortcut), widget))
            return FALSE;

          /* translate the y coordinate so that it is relative to the parent */
          gtk_widget_get_allocation (widget, &allocation);
          y = y + allocation.y;

          /* find the shortcut we are currently hovering */
          if (!thunar_shortcut_group_find_child_by_coords (group, x, y,
                                                           &child,
                                                           &child_index))
            {
              gdk_drag_status (context, 0, timestamp);
              return TRUE;
            }

          /* if the hovered shortcut is mutable, show an empty spot indicator */
          if (THUNAR_IS_SHORTCUT (child))
            {
              if (thunar_shortcut_get_mutable (THUNAR_SHORTCUT (child)))
                {
                  /* add the empty spot indicator to the shortcuts container */
                  if (gtk_widget_get_parent (group->empty_spot) == NULL)
                    {
                      gtk_box_pack_start (GTK_BOX (group->shortcuts),
                                          group->empty_spot,
                                          FALSE, TRUE, 0);
                    }

                  /* show the empty spot indicator before/after the hovered shortcut,
                   * depending on where exactly the drag motion happened */
                  gtk_widget_get_allocation (child, &allocation);
                  if (y > allocation.y + allocation.height / 2)
                    child_index += 1;
                  gtk_box_reorder_child (GTK_BOX (group->shortcuts),
                                         group->empty_spot,
                                         child_index);

                  /* display a snapshot of the dragged shortcut in the empty spot */
                  pixmap = thunar_shortcut_get_pre_drag_snapshot (group->drop_shortcut);
                  gtk_image_set_from_pixmap (GTK_IMAGE (group->empty_spot),
                                             pixmap,
                                             NULL);

                  /* hide the dragged shortcut as we are now dragging it */
                  gtk_widget_hide (GTK_WIDGET (group->drop_shortcut));

                  /* make sure to show the empty spot indicator */
                  gtk_widget_show (group->empty_spot);
                }
            }

          /* allow reordering */
          actions = GDK_ACTION_MOVE;
        }
    }
  else if (target == gdk_atom_intern_static_string ("text/uri-list"))
    {
      /* request the drop data on demand */
      if (!group->drop_data_ready)
        {
          /* request the drop data from the source */
          gtk_drag_get_data (widget, context, target, timestamp);
        }
      else
        {
        }
    }
  else
    {
      /* unsupported target */
      return FALSE;
    }

  /* tell Gdk whether we can drop here */
  gdk_drag_status (context, actions, timestamp);

  return TRUE;
}



static void
thunar_shortcut_group_attract_attention (ThunarShortcutGroup *group)
{
  _thunar_return_if_fail (THUNAR_IS_SHORTCUT_GROUP (group));

  /* check if we are in collapsed mode */
  if (!gtk_expander_get_expanded (GTK_EXPANDER (group)))
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
  _thunar_return_if_fail (expander == GTK_WIDGET (group));
  
  children = gtk_container_get_children (GTK_CONTAINER (group->shortcuts));

  for (iter = children; iter != NULL; iter = iter->next)
    {
      if (THUNAR_IS_SHORTCUT (iter->data))
        thunar_shortcut_group_apply_indentation (group, THUNAR_SHORTCUT (iter->data));
    }

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
  gtk_widget_style_get (GTK_WIDGET (group),
                        "expander-size", &expander_size,
                        "expander-spacing", &expander_spacing,
                        NULL);

  /* apply the indentation to the shortcut alignment */
  alignment = thunar_shortcut_get_alignment (shortcut);
  g_object_set (alignment,
                "left-padding", expander_size + expander_spacing * 2 + 2,
                NULL);
}



static gboolean
thunar_shortcut_group_find_child_by_coords (ThunarShortcutGroup *group,
                                            gint                 x,
                                            gint                 y,
                                            GtkWidget          **child,
                                            gint                *child_index)
{
  GtkAllocation allocation = { 0, 0, 0, 0 };
  gboolean      child_found = FALSE;
  GList        *children;
  GList        *iter;
  gint          idx = 0;

  _thunar_return_val_if_fail (THUNAR_IS_SHORTCUT_GROUP (group), FALSE);

  if (child != NULL)
    *child = NULL;

  if (child_index != NULL)
    *child_index = -1;

  children = gtk_container_get_children (GTK_CONTAINER (group->shortcuts));

  for (iter = children; !child_found && iter != NULL; iter = iter->next)
    {
      if (gtk_widget_get_visible (iter->data))
        {
          gtk_widget_get_allocation (iter->data, &allocation);

          if (y >= allocation.y && y <= allocation.y + allocation.height)
            {
              if (child != NULL)
                *child = iter->data;

              if (child_index != NULL)
                *child_index = idx;

              child_found = TRUE;
            }
        }

      idx++;
    }

  g_list_free (children);

  return child_found;
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

  gtk_size_group_add_widget (group->size_group, GTK_WIDGET (shortcut));

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
      if (!THUNAR_IS_SHORTCUT (iter->data))
        continue;

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
      if (!THUNAR_IS_SHORTCUT (iter->data))
        continue;

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
      if (!THUNAR_IS_SHORTCUT (iter->data))
        continue;

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
      if (!THUNAR_IS_SHORTCUT (iter->data))
        continue;

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
      if (!THUNAR_IS_SHORTCUT (iter->data))
        continue;

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
      if (!THUNAR_IS_SHORTCUT (iter->data))
        continue;

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
  _thunar_return_val_if_fail (THUNAR_IS_SHORTCUT_GROUP (group), FALSE);
  _thunar_return_val_if_fail (THUNAR_IS_FILE (file), FALSE);

  return thunar_shortcut_group_find_shortcut_by_location (group, file->gfile, result);
}



gboolean
thunar_shortcut_group_find_shortcut_by_location (ThunarShortcutGroup *group,
                                                 GFile               *location,
                                                 ThunarShortcut     **result)
{
  ThunarShortcut *shortcut;
  gboolean        has_shortcut = FALSE;
  GList          *children;
  GList          *iter;

  _thunar_return_val_if_fail (THUNAR_IS_SHORTCUT_GROUP (group), FALSE);
  _thunar_return_val_if_fail (G_IS_FILE (location), FALSE);

  children = gtk_container_get_children (GTK_CONTAINER (group->shortcuts));

  if (result != NULL)
    *result = NULL;

  for (iter = children; !has_shortcut && iter != NULL; iter = iter->next)
    {
      if (!THUNAR_IS_SHORTCUT (iter->data))
        continue;

      shortcut = THUNAR_SHORTCUT (iter->data);

      if (thunar_shortcut_matches_location (shortcut, location))
        {
          if (result != NULL)
            *result = shortcut;

          has_shortcut = TRUE;
        }
    }

  g_list_free (children);

  return has_shortcut;
}
