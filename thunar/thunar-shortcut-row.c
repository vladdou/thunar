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

#include <gdk/gdkkeysyms.h>

#include <exo/exo.h>

#include <thunar/thunar-browser.h>
#include <thunar/thunar-dialogs.h>
#include <thunar/thunar-enum-types.h>
#include <thunar/thunar-file.h>
#include <thunar/thunar-gio-extensions.h>
#include <thunar/thunar-marshal.h>
#include <thunar/thunar-preferences.h>
#include <thunar/thunar-private.h>
#include <thunar/thunar-shortcut-row.h>



#define THUNAR_SHORTCUT_ROW_MIN_HEIGHT 20



/* property identifiers */
enum
{
  PROP_0,
  PROP_ICON,
  PROP_LABEL,
  PROP_EJECT_ICON,
  PROP_LOCATION,
  PROP_FILE,
  PROP_VOLUME,
  PROP_MOUNT,
  PROP_SHORTCUT_TYPE,
  PROP_ICON_SIZE,
};



/* signal identifiers */
enum
{
  SIGNAL_ACTIVATED,
  SIGNAL_CONTEXT_MENU,
  LAST_SIGNAL,
};



/* row states */
typedef enum
{
  THUNAR_SHORTCUT_ROW_NORMAL,
  THUNAR_SHORTCUT_ROW_RESOLVING,
  THUNAR_SHORTCUT_ROW_EJECTING,
} ThunarShortcutRowState;




static void     thunar_shortcut_row_constructed           (GObject               *object);
static void     thunar_shortcut_row_dispose               (GObject               *object);
static void     thunar_shortcut_row_finalize              (GObject               *object);
static void     thunar_shortcut_row_get_property          (GObject               *object,
                                                           guint                  prop_id,
                                                           GValue                *value,
                                                           GParamSpec            *pspec);
static void     thunar_shortcut_row_set_property          (GObject               *object,
                                                           guint                  prop_id,
                                                           const GValue          *value,
                                                           GParamSpec            *pspec);
static gboolean thunar_shortcut_row_button_press_event    (GtkWidget             *widget,
                                                           GdkEventButton        *event);
static gboolean thunar_shortcut_row_button_release_event  (GtkWidget             *widget,
                                                           GdkEventButton        *event);
static gboolean thunar_shortcut_row_key_press_event       (GtkWidget             *widget,
                                                           GdkEventKey           *event);
static gboolean thunar_shortcut_row_enter_notify_event    (GtkWidget             *widget,
                                                           GdkEventCrossing      *event);
static gboolean thunar_shortcut_row_leave_notify_event    (GtkWidget             *widget,
                                                           GdkEventCrossing      *event);
static gboolean thunar_shortcut_row_expose_event          (GtkWidget             *widget,
                                                           GdkEventExpose        *event);
static gboolean thunar_shortcut_row_focus                 (GtkWidget             *widget,
                                                           GtkDirectionType       direction);
static gboolean thunar_shortcut_row_focus_in_event        (GtkWidget             *widget,
                                                           GdkEventFocus         *event);
static void     thunar_shortcut_row_size_request          (GtkWidget             *widget,
                                                           GtkRequisition        *requisition);
static void     thunar_shortcut_row_button_state_changed  (ThunarShortcutRow     *row,
                                                           GtkStateType           previous_state,
                                                           GtkWidget             *button);
static void     thunar_shortcut_row_button_clicked        (ThunarShortcutRow     *row,
                                                           GtkButton             *button);
static void     thunar_shortcut_row_mount_unmount_finish  (GObject               *object,
                                                           GAsyncResult          *result,
                                                           gpointer               user_data);
static void     thunar_shortcut_row_mount_eject_finish    (GObject               *object,
                                                           GAsyncResult          *result,
                                                           gpointer               user_data);
static void     thunar_shortcut_row_volume_eject_finish   (GObject               *object,
                                                           GAsyncResult          *result,
                                                           gpointer               user_data);
static void     thunar_shortcut_row_poke_volume_finish    (ThunarBrowser         *browser,
                                                           GVolume               *volume,
                                                           ThunarFile            *file,
                                                           GError                *error,
                                                           gpointer               user_data);
static void     thunar_shortcut_row_poke_file_finish      (ThunarBrowser         *browser,
                                                           ThunarFile            *file,
                                                           ThunarFile            *target_file,
                                                           GError                *error,
                                                           gpointer               user_data);
static void     thunar_shortcut_row_poke_location_finish  (ThunarBrowser         *browser,
                                                           GFile                 *location,
                                                           ThunarFile            *file,
                                                           ThunarFile            *target_file,
                                                           GError                *error,
                                                           gpointer               user_data);
static void     thunar_shortcut_row_icon_changed          (ThunarShortcutRow     *row);
static void     thunar_shortcut_row_label_changed         (ThunarShortcutRow     *row);
static void     thunar_shortcut_row_location_changed      (ThunarShortcutRow     *row);
static void     thunar_shortcut_row_file_changed          (ThunarShortcutRow     *row);
static void     thunar_shortcut_row_eject_icon_changed    (ThunarShortcutRow     *row);
static void     thunar_shortcut_row_volume_changed        (ThunarShortcutRow     *row);
static void     thunar_shortcut_row_mount_changed         (ThunarShortcutRow     *row);
static void     thunar_shortcut_row_shortcut_type_changed (ThunarShortcutRow     *row);
static void     thunar_shortcut_row_icon_size_changed     (ThunarShortcutRow     *row);
static void     thunar_shortcut_row_set_spinning          (ThunarShortcutRow     *row,
                                                           gboolean               spinning,
                                                           ThunarShortcutRowState new_state);



struct _ThunarShortcutRowClass
{
  GtkEventBoxClass __parent__;
};

struct _ThunarShortcutRow
{
  GtkEventBox            __parent__;

  ThunarPreferences     *preferences;

  gchar                 *label;

  GIcon                 *icon;
  GIcon                 *eject_icon;

  GFile                 *location;
  ThunarFile            *file;
  GVolume               *volume;
  GMount                *mount;

  ThunarShortcutType     shortcut_type;

  GtkWidget             *label_widget;
  GtkWidget             *icon_image;
  GtkWidget             *action_button;
  GtkWidget             *action_image;
  GtkWidget             *spinner;

  ThunarIconSize         icon_size;

  GCancellable          *cancellable;

  ThunarShortcutRowState state;
};



G_DEFINE_TYPE_WITH_CODE (ThunarShortcutRow, thunar_shortcut_row, GTK_TYPE_EVENT_BOX,
                         G_IMPLEMENT_INTERFACE (THUNAR_TYPE_BROWSER, NULL))



static guint row_signals[LAST_SIGNAL];



static void
thunar_shortcut_row_class_init (ThunarShortcutRowClass *klass)
{
  GtkWidgetClass *gtkwidget_class;
  GObjectClass   *gobject_class;

  /* determine the parent type class */
  thunar_shortcut_row_parent_class = g_type_class_peek_parent (klass);

  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->constructed = thunar_shortcut_row_constructed;
  gobject_class->dispose = thunar_shortcut_row_dispose;
  gobject_class->finalize = thunar_shortcut_row_finalize;
  gobject_class->get_property = thunar_shortcut_row_get_property;
  gobject_class->set_property = thunar_shortcut_row_set_property;

  gtkwidget_class = GTK_WIDGET_CLASS (klass);
  gtkwidget_class->button_press_event = thunar_shortcut_row_button_press_event;
  gtkwidget_class->button_release_event = thunar_shortcut_row_button_release_event;
  gtkwidget_class->key_press_event = thunar_shortcut_row_key_press_event;
  gtkwidget_class->enter_notify_event = thunar_shortcut_row_enter_notify_event;
  gtkwidget_class->leave_notify_event = thunar_shortcut_row_leave_notify_event;
  gtkwidget_class->expose_event = thunar_shortcut_row_expose_event;
  gtkwidget_class->focus = thunar_shortcut_row_focus;
  gtkwidget_class->focus_in_event = thunar_shortcut_row_focus_in_event;
  gtkwidget_class->size_request = thunar_shortcut_row_size_request;

  g_object_class_install_property (gobject_class,
                                   PROP_ICON,
                                   g_param_spec_object ("icon",
                                                        "icon",
                                                        "icon",
                                                        G_TYPE_ICON,
                                                        EXO_PARAM_READWRITE));

  g_object_class_install_property (gobject_class,
                                   PROP_EJECT_ICON,
                                   g_param_spec_object ("eject-icon",
                                                        "eject-icon",
                                                        "eject-icon",
                                                        G_TYPE_ICON,
                                                        EXO_PARAM_READWRITE));

  g_object_class_install_property (gobject_class,
                                   PROP_LABEL,
                                   g_param_spec_string ("label",
                                                        "label",
                                                        "label",
                                                        NULL,
                                                        EXO_PARAM_READWRITE));

  g_object_class_install_property (gobject_class,
                                   PROP_LOCATION,
                                   g_param_spec_object ("location",
                                                        "location",
                                                        "location",
                                                        G_TYPE_FILE,
                                                        EXO_PARAM_READWRITE));

  g_object_class_install_property (gobject_class,
                                   PROP_FILE,
                                   g_param_spec_object ("file",
                                                        "file",
                                                        "file",
                                                        THUNAR_TYPE_FILE,
                                                        EXO_PARAM_READWRITE));

  g_object_class_install_property (gobject_class,
                                   PROP_VOLUME,
                                   g_param_spec_object ("volume",
                                                        "volume",
                                                        "volume",
                                                        G_TYPE_VOLUME,
                                                        EXO_PARAM_READWRITE));

  g_object_class_install_property (gobject_class,
                                   PROP_MOUNT,
                                   g_param_spec_object ("mount",
                                                        "mount",
                                                        "mount",
                                                        G_TYPE_MOUNT,
                                                        EXO_PARAM_READWRITE));

  g_object_class_install_property (gobject_class,
                                   PROP_SHORTCUT_TYPE,
                                   g_param_spec_enum ("shortcut-type",
                                                      "shortcut-type",
                                                      "shortcut-type",
                                                      THUNAR_TYPE_SHORTCUT_TYPE,
                                                      THUNAR_SHORTCUT_REGULAR_FILE,
                                                      EXO_PARAM_READWRITE));

  g_object_class_install_property (gobject_class,
                                   PROP_ICON_SIZE,
                                   g_param_spec_enum ("icon-size",
                                                      "icon-size",
                                                      "icon-size",
                                                      THUNAR_TYPE_ICON_SIZE,
                                                      THUNAR_ICON_SIZE_SMALLER,
                                                      EXO_PARAM_READWRITE));

  row_signals[SIGNAL_ACTIVATED] = 
    g_signal_new (I_("activated"),
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_LAST,
                  0, NULL, NULL,
                  _thunar_marshal_VOID__OBJECT_BOOLEAN,
                  G_TYPE_NONE, 2, 
                  THUNAR_TYPE_FILE,
                  G_TYPE_BOOLEAN);

  row_signals[SIGNAL_CONTEXT_MENU] =
    g_signal_new (I_("context-menu"),
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_LAST,
                  0, NULL, NULL,
                  g_cclosure_marshal_VOID__VOID,
                  G_TYPE_NONE, 0);

}



static void
thunar_shortcut_row_init (ThunarShortcutRow *row)
{
  GtkWidget *alignment;
  GtkWidget *box;

  /* create a cancellable for aborting mount/unmount operations */
  row->cancellable = g_cancellable_new ();

  /* set the row state to normal */
  row->state = THUNAR_SHORTCUT_ROW_NORMAL;

  /* configure general widget behavior */
  gtk_widget_set_can_focus (GTK_WIDGET (row), TRUE);
  gtk_widget_set_sensitive (GTK_WIDGET (row), TRUE);

  /* create the alignment for left and right padding */
  alignment = gtk_alignment_new (0.0f, 0.0f, 1.0f, 1.0f);
  /* TODO use expander arrow width instead of 16 here */
  gtk_alignment_set_padding (GTK_ALIGNMENT (alignment), 0, 0, 16, 4);
  gtk_container_add (GTK_CONTAINER (row), alignment);
  gtk_widget_show (alignment);

  /* create a box for the different sub-widgets */
  box = gtk_hbox_new (FALSE, 4);
  gtk_container_add (GTK_CONTAINER (alignment), box);
  gtk_widget_show (box);

  /* create the icon widget */
  row->icon_image = gtk_image_new ();
  gtk_box_pack_start (GTK_BOX (box), row->icon_image, FALSE, TRUE, 0);
  gtk_widget_hide (row->icon_image);

  /* create the label widget */
  row->label_widget = gtk_label_new (NULL);
  gtk_label_set_ellipsize (GTK_LABEL (row->label_widget), PANGO_ELLIPSIZE_END);
  gtk_misc_set_alignment (GTK_MISC (row->label_widget), 0.0f, 0.5f);
  gtk_container_add (GTK_CONTAINER (box), row->label_widget);
  gtk_widget_hide (row->label_widget);

  /* create the action button */
  row->action_button = gtk_button_new ();
  gtk_button_set_relief (GTK_BUTTON (row->action_button), GTK_RELIEF_NONE);
  gtk_box_pack_start (GTK_BOX (box), row->action_button, FALSE, TRUE, 0);
  gtk_widget_hide (row->action_button);

  /* adjust the state transitions of the button */
  g_signal_connect_swapped (row->action_button, "state-changed",
                            G_CALLBACK (thunar_shortcut_row_button_state_changed), row);

  /* react on button click events */
  g_signal_connect_swapped (row->action_button, "clicked",
                            G_CALLBACK (thunar_shortcut_row_button_clicked), row);

  /* create the action button image */
  row->action_image = gtk_image_new ();
  gtk_button_set_image (GTK_BUTTON (row->action_button), row->action_image);
  gtk_widget_show (row->action_image);

  /* take a reference because we need to be able to remove it from the
   * button temporarily */
  g_object_ref (row->action_image);

  /* default to "media-eject", we only set this for the button size
  * to be computed so that all rows have equal heights */
  gtk_image_set_from_icon_name (GTK_IMAGE (row->action_image), "media-eject",
                                GTK_ICON_SIZE_MENU);

  /* create the spinner icon */
  row->spinner = gtk_spinner_new ();
  gtk_spinner_stop (GTK_SPINNER (row->spinner));

  /* take a reference because we need to be able to remove it from the
   * button temporarily */
  g_object_ref (row->spinner);

  /* update the icon size whenever necessary */
  row->preferences = thunar_preferences_get ();
  exo_binding_new (G_OBJECT (row->preferences), "shortcuts-icon-size",
                   G_OBJECT (row), "icon-size");
}



static void
thunar_shortcut_row_constructed (GObject *object)
{
}



static void
thunar_shortcut_row_dispose (GObject *object)
{
  (*G_OBJECT_CLASS (thunar_shortcut_row_parent_class)->dispose) (object);
}



static void
thunar_shortcut_row_finalize (GObject *object)
{
  ThunarShortcutRow *row = THUNAR_SHORTCUT_ROW (object);

  g_free (row->label);

  /* release the spinner and action image */
  g_object_unref (row->spinner);
  g_object_unref (row->action_image);

  if (row->icon != NULL)
    g_object_unref (row->icon);

  if (row->eject_icon != NULL)
    g_object_unref (row->eject_icon);

  if (row->location != NULL)
    g_object_unref (row->location);

  if (row->file != NULL)
    g_object_unref (row->file);

  if (row->volume != NULL)
    g_object_unref (row->volume);

  if (row->mount != NULL)
    g_object_unref (row->mount);

  /* release the cancellable */
  g_cancellable_cancel (row->cancellable);
  g_object_unref (row->cancellable);

  /* release the preferences */
  g_object_unref (row->preferences);

  (*G_OBJECT_CLASS (thunar_shortcut_row_parent_class)->finalize) (object);
}



static void
thunar_shortcut_row_get_property (GObject    *object,
                                  guint       prop_id,
                                  GValue     *value,
                                  GParamSpec *pspec)
{
  ThunarShortcutRow *row = THUNAR_SHORTCUT_ROW (object);

  switch (prop_id)
    {
    case PROP_ICON:
      g_value_set_object (value, row->icon);
      break;

    case PROP_EJECT_ICON:
      g_value_set_object (value, row->eject_icon);
      break;

    case PROP_LABEL:
      g_value_set_string (value, row->label);
      break;

    case PROP_LOCATION:
      g_value_set_object (value, row->location);
      break;

    case PROP_FILE:
      g_value_set_object (value, row->file);
      break;

    case PROP_VOLUME:
      g_value_set_object (value, row->volume);
      break;

    case PROP_MOUNT:
      g_value_set_object (value, row->mount);
      break;

    case PROP_SHORTCUT_TYPE:
      g_value_set_enum (value, row->shortcut_type);
      break;

    case PROP_ICON_SIZE:
      g_value_set_enum (value, row->icon_size);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}



static void
thunar_shortcut_row_set_property (GObject      *object,
                                  guint         prop_id,
                                  const GValue *value,
                                  GParamSpec   *pspec)
{
  ThunarShortcutRow *row = THUNAR_SHORTCUT_ROW (object);

  switch (prop_id)
    {
    case PROP_ICON:
      thunar_shortcut_row_set_icon (row, g_value_get_object (value));
      break;

    case PROP_EJECT_ICON:
      thunar_shortcut_row_set_eject_icon (row, g_value_get_object (value));
      break;

    case PROP_LABEL:
      thunar_shortcut_row_set_label (row, g_value_get_string (value));
      break;

    case PROP_LOCATION:
      thunar_shortcut_row_set_location (row, g_value_get_object (value));
      break;

    case PROP_FILE:
      thunar_shortcut_row_set_file (row, g_value_get_object (value));
      break;

    case PROP_VOLUME:
      thunar_shortcut_row_set_volume (row, g_value_get_object (value));
      break;

    case PROP_MOUNT:
      thunar_shortcut_row_set_mount (row, g_value_get_object (value));
      break;

    case PROP_SHORTCUT_TYPE:
      thunar_shortcut_row_set_shortcut_type (row, g_value_get_enum (value));
      break;

    case PROP_ICON_SIZE:
      thunar_shortcut_row_set_icon_size (row, g_value_get_enum (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}



static gboolean
thunar_shortcut_row_button_press_event (GtkWidget      *widget,
                                        GdkEventButton *event)
{
  GtkStateType state;

  _thunar_return_val_if_fail (THUNAR_IS_SHORTCUT_ROW (widget), FALSE);

  /* determine the widget's state */
  state = gtk_widget_get_state (widget);

  if (state == GTK_STATE_SELECTED)
    {
      if ((event->state & GDK_CONTROL_MASK) != 0)
        gtk_widget_set_state (widget, GTK_STATE_NORMAL);
    }
  else
    {
      gtk_widget_set_state (widget, GTK_STATE_SELECTED);
      gtk_widget_grab_focus (widget);
    }

  /* distinguish between left, right and middle click */
  if (event->button == 1)
    {
      /* resolve (e.g. mount) the shortcut and activate it */
      if (gtk_widget_get_state (widget) == GTK_STATE_SELECTED)
        thunar_shortcut_row_resolve_and_activate (THUNAR_SHORTCUT_ROW (widget), FALSE);
    }
  else if (event->button == 3)
    {
      /* TODO start a context menu timeout */
      g_debug ("right button press");
    }
  else if (event->button == 2)
    {
      /* TODO we don't handle middle-click events yet */
      g_debug ("middle button press");
    }

  return TRUE;
}



static gboolean
thunar_shortcut_row_button_release_event (GtkWidget      *widget,
                                          GdkEventButton *event)
{
  _thunar_return_val_if_fail (THUNAR_IS_SHORTCUT_ROW (widget), FALSE);

  /* distinguish between left, right and middle-click */
  if (event->button == 3)
    {
      /* TODO abort the menu popup timeout created in reaction to 
       * the right button press event */

      /* emit the popup-menu signal */
      g_signal_emit_by_name (widget, "context-menu");

      return TRUE;
    }
  else
    {
      return FALSE;
    }
}



static gboolean
thunar_shortcut_row_key_press_event (GtkWidget   *widget,
                                     GdkEventKey *event)
{
  gboolean new_window;

  _thunar_return_val_if_fail (THUNAR_IS_SHORTCUT_ROW (widget), FALSE);

  if (event->keyval == GDK_KEY_Return
      || event->keyval == GDK_KEY_KP_Enter
      || event->keyval == GDK_KEY_space
      || event->keyval== GDK_KEY_KP_Space)
    {
      new_window = (event->state & GDK_CONTROL_MASK) != 0;

      thunar_shortcut_row_resolve_and_activate (THUNAR_SHORTCUT_ROW (widget), 
                                                new_window);

      return TRUE;
    }

  return FALSE;
}



static gboolean
thunar_shortcut_row_enter_notify_event (GtkWidget        *widget,
                                        GdkEventCrossing *event)
{
  _thunar_return_val_if_fail (THUNAR_IS_SHORTCUT_ROW (widget), FALSE);

  if (gtk_widget_get_state (widget) != GTK_STATE_SELECTED)
    gtk_widget_set_state (widget, GTK_STATE_PRELIGHT);

  return TRUE;
}



static gboolean
thunar_shortcut_row_leave_notify_event (GtkWidget        *widget,
                                        GdkEventCrossing *event)
{
  ThunarShortcutRow *row = THUNAR_SHORTCUT_ROW (widget);
  gint               x;
  gint               y;
  gint               width;
  gint               height;

  _thunar_return_val_if_fail (THUNAR_IS_SHORTCUT_ROW (widget), FALSE);

  /* check whether the row is hovered but not selected */
  if (gtk_widget_get_state (widget) == GTK_STATE_PRELIGHT)
    {
      /* determine the location of the pointer relative to the action button */
      gtk_widget_get_pointer (row->action_button, &x, &y);

      /* determine the geometry of the button window */
      gdk_window_get_geometry (gtk_widget_get_window (row->action_button),
                               NULL, NULL, &width, &height, NULL);

      /* the pointer has left the row widget itself but we only
       * reset the prelight state if the mouse is not hovering
       * the button either */
      if (x < 0 || y < 0 || x >= width || y >= height)
        gtk_widget_set_state (widget, GTK_STATE_NORMAL);
    }

  return TRUE;
}



static gboolean
thunar_shortcut_row_expose_event (GtkWidget      *widget,
                                  GdkEventExpose *event)
{
  GtkStateType state;
  GList       *children;
  GList       *lp;

  _thunar_return_val_if_fail (THUNAR_IS_SHORTCUT_ROW (widget), FALSE);

  /* determine the widget state */
  state = gtk_widget_get_state (widget);

  /* paint a flat box that gives the row the same look as a
   * tree view row */
  gtk_paint_flat_box (gtk_widget_get_style (widget),
                      event->window,
                      state,
                      GTK_SHADOW_NONE,
                      &event->area,
                      widget,
                      "cell_even_middle",
                      event->area.x,
                      event->area.y,
                      event->area.width,
                      event->area.height);

  /* propagate the expose event to all children */
  children = gtk_container_get_children (GTK_CONTAINER (widget));
  for (lp = children; lp != NULL; lp = lp->next)
    gtk_container_propagate_expose (GTK_CONTAINER (widget), lp->data, event);
  g_list_free (children);

  return FALSE;
}



static gboolean
thunar_shortcut_row_focus (GtkWidget       *widget,
                           GtkDirectionType direction)
{
  ThunarShortcutRow *row = THUNAR_SHORTCUT_ROW (widget);

  _thunar_return_val_if_fail (THUNAR_IS_SHORTCUT_ROW (widget), FALSE);

  switch (direction)
    {
    case GTK_DIR_TAB_FORWARD:
    case GTK_DIR_TAB_BACKWARD:
      return FALSE;

    case GTK_DIR_UP:
      if (gtk_widget_is_focus (widget) || gtk_widget_is_focus (row->action_button))
        {
          return FALSE;
        }
      else
        {
          gtk_widget_grab_focus (widget);
          return TRUE;
        }

    case GTK_DIR_DOWN:
      if (gtk_widget_is_focus (widget) || gtk_widget_is_focus (row->action_button))
        {
          return FALSE;
        }
      else
        {
          gtk_widget_grab_focus (widget);
          return TRUE;
        }

    case GTK_DIR_LEFT:
      gtk_widget_grab_focus (widget);
      return TRUE;

    case GTK_DIR_RIGHT:
      if (gtk_widget_get_visible (row->action_button))
        {
          gtk_widget_grab_focus (row->action_button);
          return TRUE;
        }
      else
        {
          return FALSE;
        }

    default:
      return FALSE;
    }
}



static gboolean
thunar_shortcut_row_focus_in_event (GtkWidget     *widget,
                                    GdkEventFocus *event)
{
  _thunar_return_val_if_fail (THUNAR_IS_SHORTCUT_ROW (widget), FALSE);

  gtk_widget_set_state (widget, GTK_STATE_SELECTED);
  return TRUE;
}



static void
thunar_shortcut_row_size_request (GtkWidget      *widget,
                                  GtkRequisition *requisition)
{
  ThunarShortcutRow *row = THUNAR_SHORTCUT_ROW (widget);
  GtkRequisition     button_requisition;

  _thunar_return_if_fail (THUNAR_IS_SHORTCUT_ROW (widget));

  /* let the event box class compute the size we need for its children */
  (*GTK_WIDGET_CLASS (thunar_shortcut_row_parent_class)->size_request) (widget, requisition);

  /* compute the button size */
  gtk_widget_size_request (row->action_button, &button_requisition);

  /* use the maximum of the computed requisition height, the button height,
   * the icon size + 4, and the minimum allowed height for rows */
  requisition->height = MAX (requisition->height, button_requisition.height);
  requisition->height = MAX (requisition->height, (gint) row->icon_size + 4);
  requisition->height = MAX (requisition->height, THUNAR_SHORTCUT_ROW_MIN_HEIGHT);
}



static void
thunar_shortcut_row_button_state_changed (ThunarShortcutRow *row,
                                          GtkStateType       previous_state,
                                          GtkWidget         *button)
{
  gint x;
  gint y;
  gint width;
  gint height;

  _thunar_return_if_fail (THUNAR_IS_SHORTCUT_ROW (row));

  if (gtk_widget_get_state (row->action_button) == GTK_STATE_PRELIGHT
      && gtk_widget_get_state (GTK_WIDGET (row)) == GTK_STATE_PRELIGHT)
    {
      gtk_widget_get_pointer (row->action_button, &x, &y);

      gdk_window_get_geometry (gtk_widget_get_window (row->action_button),
                               NULL, NULL, &width, &height, NULL);

      if (x < 0 || y < 0 || x >= width || y >= height)
        gtk_widget_set_state (row->action_button, GTK_STATE_NORMAL);
    }
}



static void
thunar_shortcut_row_button_clicked (ThunarShortcutRow *row,
                                    GtkButton         *button)
{
  _thunar_return_if_fail (THUNAR_IS_SHORTCUT_ROW (row));

  /* check if we are currently mounting/ejecting something */
  if (row->state != THUNAR_SHORTCUT_ROW_NORMAL)
    {
      /* abort the mount/eject process */
      g_cancellable_cancel (row->cancellable);

      /* we're done, no further processing please */
      return;
    }

  /* disconnect the shortcut */
  thunar_shortcut_row_disconnect (row);
}



static void
thunar_shortcut_row_mount_unmount_finish (GObject      *object,
                                          GAsyncResult *result,
                                          gpointer      user_data)
{
  ThunarShortcutRow *row = THUNAR_SHORTCUT_ROW (user_data);
  GMount            *mount = G_MOUNT (object);
  GError            *error = NULL;

  _thunar_return_if_fail (THUNAR_IS_SHORTCUT_ROW (row));
  _thunar_return_if_fail (G_IS_MOUNT (mount));
  _thunar_return_if_fail (G_IS_ASYNC_RESULT (result));

  /* stop spinning */
  thunar_shortcut_row_set_spinning (row, FALSE, THUNAR_SHORTCUT_ROW_NORMAL);

  if (!g_mount_unmount_with_operation_finish (mount, result, &error))
    {
      thunar_dialogs_show_error (GTK_WIDGET (row), error,
                                 _("Failed to eject \"%s\""),
                                 row->label);
      g_error_free (error);
    }
}



static void
thunar_shortcut_row_mount_eject_finish (GObject      *object,
                                        GAsyncResult *result,
                                        gpointer      user_data)
{
  ThunarShortcutRow *row = THUNAR_SHORTCUT_ROW (user_data);
  GMount            *mount = G_MOUNT (object);
  GError            *error = NULL;

  /* stop spinning */
  thunar_shortcut_row_set_spinning (row, FALSE, THUNAR_SHORTCUT_ROW_NORMAL);

  _thunar_return_if_fail (THUNAR_IS_SHORTCUT_ROW (row));
  _thunar_return_if_fail (G_IS_MOUNT (mount));
  _thunar_return_if_fail (G_IS_ASYNC_RESULT (result));

  if (!g_mount_eject_with_operation_finish (mount, result, &error))
    {
      thunar_dialogs_show_error (GTK_WIDGET (row), error,
                                 _("Failed to eject \"%s\""),
                                 row->label);
      g_error_free (error);
    }
}



static void
thunar_shortcut_row_volume_eject_finish (GObject      *object,
                                         GAsyncResult *result,
                                         gpointer      user_data)
{
  ThunarShortcutRow *row = THUNAR_SHORTCUT_ROW (user_data);
  GVolume           *volume = G_VOLUME (object);
  GError            *error = NULL;

  _thunar_return_if_fail (THUNAR_IS_SHORTCUT_ROW (row));
  _thunar_return_if_fail (G_IS_VOLUME (volume));
  _thunar_return_if_fail (G_IS_ASYNC_RESULT (result));

  /* stop spinning */
  thunar_shortcut_row_set_spinning (row, FALSE, THUNAR_SHORTCUT_ROW_NORMAL);

  if (!g_volume_eject_with_operation_finish (volume, result, &error))
    {
      thunar_dialogs_show_error (GTK_WIDGET (row), error,
                                 _("Failed to eject \"%s\""),
                                 row->label);
      g_error_free (error);
    }
}



static void
thunar_shortcut_row_poke_volume_finish (ThunarBrowser *browser,
                                        GVolume       *volume,
                                        ThunarFile    *file,
                                        GError        *error,
                                        gpointer       user_data)
{
  ThunarShortcutRow *row = THUNAR_SHORTCUT_ROW (browser);
  gboolean           open_in_new_window = GPOINTER_TO_UINT (user_data);

  _thunar_return_if_fail (THUNAR_IS_SHORTCUT_ROW (browser));
  _thunar_return_if_fail (G_IS_VOLUME (volume));
  _thunar_return_if_fail (file == NULL || THUNAR_IS_FILE (file));

  /* deactivate the spinner */
  thunar_shortcut_row_set_spinning (row, FALSE, THUNAR_SHORTCUT_ROW_NORMAL);

  if (error == NULL)
    {
      g_signal_emit (row, row_signals[SIGNAL_ACTIVATED], 0, file, open_in_new_window);
    }
  else
    {
      thunar_dialogs_show_error (GTK_WIDGET (row), error,
                                 _("Failed to open \"%s\""),
                                 row->label);
    }
}



static void
thunar_shortcut_row_poke_file_finish (ThunarBrowser *browser,
                                        ThunarFile    *file,
                                        ThunarFile    *target_file,
                                        GError        *error,
                                        gpointer       user_data)
{
  ThunarShortcutRow *row = THUNAR_SHORTCUT_ROW (browser);
  gboolean           open_in_new_window = GPOINTER_TO_UINT (user_data);

  _thunar_return_if_fail (THUNAR_IS_SHORTCUT_ROW (browser));
  _thunar_return_if_fail (THUNAR_IS_FILE (file));
  _thunar_return_if_fail (target_file == NULL || THUNAR_IS_FILE (target_file));

  /* deactivate the spinner */
  thunar_shortcut_row_set_spinning (row, FALSE, THUNAR_SHORTCUT_ROW_NORMAL);

  if (error == NULL)
    {
      g_signal_emit (row, row_signals[SIGNAL_ACTIVATED], 0, target_file, 
                     open_in_new_window);
    }
  else
    {
      thunar_dialogs_show_error (GTK_WIDGET (row), error,
                                 _("Failed to open \"%s\""),
                                 row->label);
    }
}



static void
thunar_shortcut_row_poke_location_finish (ThunarBrowser *browser,
                                          GFile         *location,
                                          ThunarFile    *file,
                                          ThunarFile    *target_file,
                                          GError        *error,
                                          gpointer       user_data)
{
  ThunarShortcutRow *row = THUNAR_SHORTCUT_ROW (browser);
  gboolean           open_in_new_window = GPOINTER_TO_UINT (user_data);

  _thunar_return_if_fail (THUNAR_IS_SHORTCUT_ROW (browser));
  _thunar_return_if_fail (G_IS_FILE (location));
  _thunar_return_if_fail (file == NULL || THUNAR_IS_FILE (file));
  _thunar_return_if_fail (target_file == NULL || THUNAR_IS_FILE (target_file));

  /* deactivate the spinner */
  thunar_shortcut_row_set_spinning (row, FALSE, THUNAR_SHORTCUT_ROW_NORMAL);

  if (error == NULL)
    {
      g_signal_emit (row, row_signals[SIGNAL_ACTIVATED], 0, target_file, 
                     open_in_new_window);
    }
  else
    {
      thunar_dialogs_show_error (GTK_WIDGET (row), error,
                                 _("Failed to open \"%s\""),
                                 row->label);
    }
}



void
thunar_shortcut_row_resolve_and_activate (ThunarShortcutRow *row,
                                          gboolean           open_in_new_window)
{
  _thunar_return_if_fail (THUNAR_IS_SHORTCUT_ROW (row));

  /* check if we are currently mounting/ejecting something */
  if (row->state != THUNAR_SHORTCUT_ROW_NORMAL)
    {
      /* abort the current mount/eject process */
      g_cancellable_cancel (row->cancellable);
      g_cancellable_reset (row->cancellable);
    }

  if (row->volume != NULL)
    {
      /* activate the spinner */
      thunar_shortcut_row_set_spinning (row, TRUE, THUNAR_SHORTCUT_ROW_RESOLVING);

      thunar_browser_poke_volume (THUNAR_BROWSER (row), row->volume, row,
                                  thunar_shortcut_row_poke_volume_finish,
                                  GUINT_TO_POINTER (open_in_new_window));
    }
  else if (row->file != NULL)
    {
      /* activate the spinner */
      thunar_shortcut_row_set_spinning (row, TRUE, THUNAR_SHORTCUT_ROW_RESOLVING);

      thunar_browser_poke_file (THUNAR_BROWSER (row), row->file, row,
                                thunar_shortcut_row_poke_file_finish,
                                GUINT_TO_POINTER (open_in_new_window));
    }
  else if (row->location != NULL)
    {
      /* activate the spinner */
      thunar_shortcut_row_set_spinning (row, TRUE, THUNAR_SHORTCUT_ROW_RESOLVING);

      thunar_browser_poke_location (THUNAR_BROWSER (row), row->location, row,
                                    thunar_shortcut_row_poke_location_finish,
                                    GUINT_TO_POINTER (open_in_new_window));
    }
  else
    {
      _thunar_assert_not_reached ();
    }
}



static void
thunar_shortcut_row_icon_changed (ThunarShortcutRow *row)
{
  _thunar_return_if_fail (THUNAR_IS_SHORTCUT_ROW (row));

  /* update the icon image */
  gtk_image_set_from_gicon (GTK_IMAGE (row->icon_image), row->icon, row->icon_size);
  gtk_widget_set_visible (row->icon_image, row->icon != NULL);
}



static void
thunar_shortcut_row_label_changed (ThunarShortcutRow *row)
{
  _thunar_return_if_fail (THUNAR_IS_SHORTCUT_ROW (row));

  /* update the label widget */
  gtk_label_set_markup (GTK_LABEL (row->label_widget), row->label);
  gtk_widget_set_visible (row->label_widget, row->label != NULL && *row->label != '\0');
}



static void
thunar_shortcut_row_eject_icon_changed (ThunarShortcutRow *row)
{
  _thunar_return_if_fail (THUNAR_IS_SHORTCUT_ROW (row));

  if (row->eject_icon != NULL)
    {
      /* update the action button image */
      gtk_image_set_from_gicon (GTK_IMAGE (row->action_image), row->eject_icon,
                                GTK_ICON_SIZE_MENU);
    }
  else
    {
      /* default to "media-eject", we only set this for the button size
       * to be computed so that all rows have equal heights */
      gtk_image_set_from_icon_name (GTK_IMAGE (row->action_image), "media-eject",
                                    GTK_ICON_SIZE_MENU);
    }
}




static void
thunar_shortcut_row_location_changed (ThunarShortcutRow *row)
{
  _thunar_return_if_fail (THUNAR_IS_SHORTCUT_ROW (row));

  /* don't update the row based on the location if it's not
   * a regular or network file */
  if (row->shortcut_type != THUNAR_SHORTCUT_REGULAR_FILE 
      && row->shortcut_type != THUNAR_SHORTCUT_NETWORK_FILE)
    {
      return;
    }

  if (row->location != NULL)
    {
      /* do nothing (yet) */
    }
  else
    {
      /* if this is reached, there is something wrong with either the
       * classification in ThunarShortcutsModel or how the data is
       * passed over from the model to ThunarShortcutsView and to
       * this row */
      _thunar_assert_not_reached ();
    }
}



static void
thunar_shortcut_row_file_changed (ThunarShortcutRow *row)
{
  _thunar_return_if_fail (THUNAR_IS_SHORTCUT_ROW (row));

  /* don't update the row based on the file if it's not
   * a regular or network file */
  if (row->shortcut_type != THUNAR_SHORTCUT_REGULAR_FILE 
      && row->shortcut_type != THUNAR_SHORTCUT_NETWORK_FILE)
    {
      return;
    }

  if (row->file != NULL)
    {
      /* update the visibility of the eject button */
      if (thunar_file_is_mountable (row->file) && thunar_file_is_mounted (row->file))
        gtk_widget_set_visible (row->action_button, TRUE);
      else
        gtk_widget_set_visible (row->action_button, FALSE);
    }
  else
    {
      /* update the row based on the location alone */
      thunar_shortcut_row_location_changed (row);
    }
}



static void
thunar_shortcut_row_volume_changed (ThunarShortcutRow *row)
{
  _thunar_return_if_fail (THUNAR_IS_SHORTCUT_ROW (row));

  /* don't update the information based on the volume unless
   * it refers to a regular or ejectable volume */
  if (row->shortcut_type != THUNAR_SHORTCUT_REGULAR_VOLUME
      && row->shortcut_type != THUNAR_SHORTCUT_EJECTABLE_VOLUME)
    {
      return;
    }

  if (row->volume != NULL)
    {
      /* update the action button */
      if (thunar_g_volume_is_removable (row->volume)
          && thunar_g_volume_is_mounted (row->volume))
        {
          gtk_widget_set_visible (row->action_button, TRUE);
        }
      else
        {
          gtk_widget_set_visible (row->action_button, FALSE);
        }
    }
  else
    {
      /* if this is reached, there is something wrong with either the
       * classification in ThunarShortcutsModel or how the data is
       * passed over from the model to ThunarShortcutsView and to
       * this row */
      _thunar_assert_not_reached ();
    }
}



static void
thunar_shortcut_row_mount_changed (ThunarShortcutRow *row)
{
  _thunar_return_if_fail (THUNAR_IS_SHORTCUT_ROW (row));

  /* don't update the row based on the mount unless we have
   * a standalone mount that is not associated with a volume */
  if (row->shortcut_type != THUNAR_SHORTCUT_REGULAR_MOUNT)
    return;

  if (row->mount != NULL)
    {
      /* update the action button */
      if (g_mount_can_unmount (row->mount) || g_mount_can_eject (row->mount))
        gtk_widget_set_visible (row->action_button, TRUE);
      else
        gtk_widget_set_visible (row->action_button, FALSE);
    }
  else
    {
      /* if this is reached, there is something wrong with either the
       * classification in ThunarShortcutsModel or how the data is
       * passed over from the model to ThunarShortcutsView and to
       * this row */
      _thunar_assert_not_reached ();
    }
}



static void
thunar_shortcut_row_shortcut_type_changed (ThunarShortcutRow *row)
{
  _thunar_return_if_fail (THUNAR_IS_SHORTCUT_ROW (row));

  /* update the row depending on the type */
  switch (row->shortcut_type)
    {
    case THUNAR_SHORTCUT_REGULAR_FILE:
    case THUNAR_SHORTCUT_NETWORK_FILE:
      thunar_shortcut_row_location_changed (row);
      break;

    case THUNAR_SHORTCUT_REGULAR_MOUNT:
      thunar_shortcut_row_mount_changed (row);
      break;

    case THUNAR_SHORTCUT_EJECTABLE_VOLUME:
    case THUNAR_SHORTCUT_REGULAR_VOLUME:
      thunar_shortcut_row_volume_changed (row);
      break;

    default:
      _thunar_assert_not_reached ();
      break;
    }
}



static void
thunar_shortcut_row_icon_size_changed (ThunarShortcutRow *row)
{
  _thunar_return_if_fail (THUNAR_IS_SHORTCUT_ROW (row));

  gtk_image_set_pixel_size (GTK_IMAGE (row->icon_image), row->icon_size);
  gtk_image_set_pixel_size (GTK_IMAGE (row->action_image), row->icon_size);

  gtk_widget_set_size_request (row->spinner, row->icon_size, row->icon_size);
}



static void
thunar_shortcut_row_set_spinning (ThunarShortcutRow     *row,
                                  gboolean               spinning,
                                  ThunarShortcutRowState new_state)
{
  _thunar_return_if_fail (THUNAR_IS_SHORTCUT_ROW (row));

  /* apply the new state */
  row->state = new_state;

  /* reset the cancelable so that we can use it to cancel the process */
  g_cancellable_reset (row->cancellable);

  if (spinning)
    {
      gtk_button_set_image (GTK_BUTTON (row->action_button), row->spinner);
      gtk_spinner_start (GTK_SPINNER (row->spinner));
      gtk_widget_show (row->spinner);
      gtk_widget_show (row->action_button);
    }
  else
    {
      gtk_button_set_image (GTK_BUTTON (row->action_button), row->action_image);
      gtk_spinner_stop (GTK_SPINNER (row->spinner));
      gtk_widget_hide (row->spinner);
      gtk_widget_hide (row->action_button);

      /* assume the mount and volume have changed which will make 
       * the action button visible again if the volume or mount is 
       * mounted and can be ejected */
      thunar_shortcut_row_mount_changed (row);
      thunar_shortcut_row_volume_changed (row);
    }
}



void
thunar_shortcut_row_set_icon (ThunarShortcutRow *row,
                              GIcon             *icon)
{
  _thunar_return_if_fail (THUNAR_IS_SHORTCUT_ROW (row));
  _thunar_return_if_fail (icon == NULL || G_IS_ICON (icon));

  if (row->icon == icon)
    return;

  if (row->icon != NULL)
    g_object_unref (row->icon);

  if (icon != NULL)
    row->icon = g_object_ref (icon);
  else
    row->icon = NULL;

  thunar_shortcut_row_icon_changed (row);

  g_object_notify (G_OBJECT (row), "icon");
}



void
thunar_shortcut_row_set_eject_icon (ThunarShortcutRow *row,
                                    GIcon             *eject_icon)
{
  _thunar_return_if_fail (THUNAR_IS_SHORTCUT_ROW (row));
  _thunar_return_if_fail (eject_icon == NULL || G_IS_ICON (eject_icon));

  if (row->eject_icon == eject_icon)
    return;

  if (row->eject_icon != NULL)
    g_object_unref (row->eject_icon);

  if (eject_icon != NULL)
    row->eject_icon = g_object_ref (eject_icon);
  else
    row->eject_icon = NULL;

  thunar_shortcut_row_eject_icon_changed (row);

  g_object_notify (G_OBJECT (row), "eject-icon");
}



void
thunar_shortcut_row_set_label (ThunarShortcutRow *row,
                               const gchar       *label)
{
  _thunar_return_if_fail (THUNAR_IS_SHORTCUT_ROW (row));

  if (g_strcmp0 (row->label, label) == 0)
    return;

  g_free (row->label);

  row->label = g_strdup (label);

  thunar_shortcut_row_label_changed (row);

  g_object_notify (G_OBJECT (row), "label");
}



void
thunar_shortcut_row_set_location (ThunarShortcutRow *row,
                                  GFile             *location)
{
  _thunar_return_if_fail (THUNAR_IS_SHORTCUT_ROW (row));
  _thunar_return_if_fail (location == NULL || G_IS_FILE (location));

  if (row->location == location)
    return;

  if (row->location != NULL)
    g_object_unref (row->location);

  if (location != NULL)
    row->location = g_object_ref (location);
  else
    row->location = NULL;

  thunar_shortcut_row_location_changed (row);

  g_object_notify (G_OBJECT (row), "location");
}



GFile *
thunar_shortcut_row_get_location (ThunarShortcutRow *row)
{
  _thunar_return_val_if_fail (THUNAR_IS_SHORTCUT_ROW (row), NULL);
  return row->location;
}



void
thunar_shortcut_row_set_file (ThunarShortcutRow *row,
                              ThunarFile        *file)
{
  _thunar_return_if_fail (THUNAR_IS_SHORTCUT_ROW (row));
  _thunar_return_if_fail (file == NULL || THUNAR_IS_FILE (file));

  if (row->file == file)
    return;

  if (row->file != NULL)
    g_object_unref (row->file);

  if (file != NULL)
    row->file = g_object_ref (file);
  else
    row->file = NULL;

  thunar_shortcut_row_file_changed (row);

  g_object_notify (G_OBJECT (row), "file");
}



ThunarFile *
thunar_shortcut_row_get_file (ThunarShortcutRow *row)
{
  _thunar_return_val_if_fail (THUNAR_IS_SHORTCUT_ROW (row), NULL);
  return row->file;
}



void
thunar_shortcut_row_set_volume (ThunarShortcutRow *row,
                                GVolume           *volume)
{
  _thunar_return_if_fail (THUNAR_IS_SHORTCUT_ROW (row));
  _thunar_return_if_fail (volume == NULL || G_IS_VOLUME (volume));

  if (row->volume != NULL)
    g_object_unref (row->volume);

  if (volume != NULL)
    row->volume = g_object_ref (volume);
  else
    row->volume = NULL;

  thunar_shortcut_row_volume_changed (row);

  g_object_notify (G_OBJECT (row), "volume");
}



GVolume *
thunar_shortcut_row_get_volume (ThunarShortcutRow *row)
{
  _thunar_return_val_if_fail (THUNAR_IS_SHORTCUT_ROW (row), NULL);
  return row->volume;
}



void
thunar_shortcut_row_set_mount (ThunarShortcutRow *row,
                               GMount            *mount)
{
  _thunar_return_if_fail (THUNAR_IS_SHORTCUT_ROW (row));
  _thunar_return_if_fail (mount == NULL || G_IS_MOUNT (mount));

  if (row->mount != NULL)
    g_object_unref (row->mount);

  if (mount != NULL)
    row->mount = g_object_ref (mount);
  else
    row->mount = NULL;

  thunar_shortcut_row_mount_changed (row);

  g_object_notify (G_OBJECT (row), "mount");
}



GMount *
thunar_shortcut_row_get_mount (ThunarShortcutRow *row)
{
  _thunar_return_val_if_fail (THUNAR_IS_SHORTCUT_ROW (row), NULL);
  return row->mount;
}



void
thunar_shortcut_row_set_shortcut_type (ThunarShortcutRow *row,
                                       ThunarShortcutType shortcut_type)
{
  _thunar_return_if_fail (THUNAR_IS_SHORTCUT_ROW (row));

  if (row->shortcut_type == shortcut_type)
    return;

  row->shortcut_type = shortcut_type;

  thunar_shortcut_row_shortcut_type_changed (row);

  g_object_notify (G_OBJECT (row), "shortcut-type");
}



ThunarShortcutType
thunar_shortcut_row_get_shortcut_type (ThunarShortcutRow *row)
{
  _thunar_return_val_if_fail (THUNAR_IS_SHORTCUT_ROW (row), 0);
  return row->shortcut_type;
}



void
thunar_shortcut_row_set_icon_size (ThunarShortcutRow *row,
                                   ThunarIconSize     icon_size)
{
  _thunar_return_if_fail (THUNAR_IS_SHORTCUT_ROW (row));
  _thunar_return_if_fail (icon_size >= THUNAR_ICON_SIZE_SMALLEST && icon_size <= THUNAR_ICON_SIZE_LARGEST);

  if (row->icon_size == icon_size)
    return;

  row->icon_size = icon_size;

  thunar_shortcut_row_icon_size_changed (row);

  g_object_notify (G_OBJECT (row), "icon-size");
}



void
thunar_shortcut_row_mount (ThunarShortcutRow *row)
{
  _thunar_return_if_fail (THUNAR_IS_SHORTCUT_ROW (row));

  thunar_shortcut_row_resolve_and_activate (row, FALSE);
}



void
thunar_shortcut_row_unmount (ThunarShortcutRow *row)
{
  GMountOperation *mount_operation;
  GtkWidget       *toplevel;
  GMount          *mount;

  _thunar_return_if_fail (THUNAR_IS_SHORTCUT_ROW (row));

  g_debug ("row unmount");

  /* check if we are currently mounting/ejecting something */
  if (row->state != THUNAR_SHORTCUT_ROW_NORMAL)
    {
      /* abort the current mount/eject process */
      g_cancellable_cancel (row->cancellable);
      g_cancellable_reset (row->cancellable);
    }

  /* create a mount operation */
  toplevel = gtk_widget_get_toplevel (GTK_WIDGET (row));
  mount_operation = gtk_mount_operation_new (GTK_WINDOW (toplevel));
  gtk_mount_operation_set_screen (GTK_MOUNT_OPERATION (mount_operation),
                                  gtk_widget_get_screen (GTK_WIDGET (row)));

  if (row->mount != NULL)
    {
      g_debug ("  have mount");

      /* only handle mounts that can be unmounted here */
      if (g_mount_can_unmount (row->mount))
        {
          g_debug ("  mount can unmount");

          /* start spinning */
          thunar_shortcut_row_set_spinning (row, TRUE, THUNAR_SHORTCUT_ROW_EJECTING);

          g_debug ("  unmount now");

          /* try unmounting the mount */
          g_mount_unmount_with_operation (row->mount,
                                          G_MOUNT_UNMOUNT_NONE,
                                          mount_operation,
                                          row->cancellable,
                                          thunar_shortcut_row_mount_unmount_finish,
                                          row);
        }
    }
  else if (row->volume != NULL)
    {
      g_debug ("  have volume");
      mount = g_volume_get_mount (row->volume);
      if (mount != NULL)
        {
          g_debug ("  have mount");

          /* only handle mounts that can be unmounted here */
          if (g_mount_can_unmount(mount))
            {
              g_debug ("  can unmount");

              /* start spinning */
              thunar_shortcut_row_set_spinning (row, TRUE, 
                                                THUNAR_SHORTCUT_ROW_EJECTING);

              g_debug ("  unmount now");

              /* try unmounting the mount */
              g_mount_unmount_with_operation (mount,
                                              G_MOUNT_UNMOUNT_NONE,
                                              mount_operation,
                                              row->cancellable,
                                              thunar_shortcut_row_mount_unmount_finish,
                                              row);
            }

          /* release the mount */
          g_object_unref (mount);
        }
      
    }
  else
    {
      /* this method was called despite no mount or volume being available
       * for this shortcut... that should not happen */
    }

  /* release the mount operation */
  g_object_unref (mount_operation);
}



void
thunar_shortcut_row_disconnect (ThunarShortcutRow *row)
{
  GMountOperation *mount_operation;
  GtkWidget       *toplevel;
  GMount          *mount;

  _thunar_return_if_fail (THUNAR_IS_SHORTCUT_ROW (row));

  g_debug ("disconnect row");

  /* check if we are currently mounting/ejecting something */
  if (row->state != THUNAR_SHORTCUT_ROW_NORMAL)
    {
      /* abort the current mount/eject process */
      g_cancellable_cancel (row->cancellable);
      g_cancellable_reset (row->cancellable);
    }

  /* create a mount operation */
  toplevel = gtk_widget_get_toplevel (GTK_WIDGET (row));
  mount_operation = gtk_mount_operation_new (GTK_WINDOW (toplevel));
  gtk_mount_operation_set_screen (GTK_MOUNT_OPERATION (mount_operation),
                                  gtk_widget_get_screen (GTK_WIDGET (row)));

  if (row->mount != NULL)
    {
      g_debug ("  have mount");

      /* distinguish between ejectable and unmountable mounts */
      if (g_mount_can_eject (row->mount))
        {
          g_debug ("  can eject");

          /* start spinning */
          thunar_shortcut_row_set_spinning (row, TRUE, THUNAR_SHORTCUT_ROW_EJECTING);

          g_debug ("  eject now");

          /* try ejecting the mount */
          g_mount_eject_with_operation (row->mount,
                                        G_MOUNT_UNMOUNT_NONE,
                                        mount_operation,
                                        row->cancellable,
                                        thunar_shortcut_row_mount_eject_finish,
                                        row);
        }
      else if (g_mount_can_unmount (row->mount))
        {
          /* start spinning */
          thunar_shortcut_row_set_spinning (row, TRUE, 
                                            THUNAR_SHORTCUT_ROW_EJECTING);

          /* try unmounting the mount */
          g_mount_unmount_with_operation (row->mount,
                                          G_MOUNT_UNMOUNT_NONE,
                                          mount_operation,
                                          row->cancellable,
                                          thunar_shortcut_row_mount_unmount_finish,
                                          row);
        }
    }
  else if (row->volume != NULL)
    {
      if (g_volume_can_eject (row->volume))
        {
          /* start spinning */
          thunar_shortcut_row_set_spinning (row, TRUE, THUNAR_SHORTCUT_ROW_EJECTING);

          /* try ejecting the volume */
          g_volume_eject_with_operation (row->volume,
                                         G_MOUNT_UNMOUNT_NONE,
                                         mount_operation,
                                         row->cancellable,
                                         thunar_shortcut_row_volume_eject_finish,
                                         row);
        }
      else
        {
          mount = g_volume_get_mount (row->volume);
          if (mount != NULL)
            {
              /* distinguish between ejectable and unmountable mounts */
              if (g_mount_can_eject (mount))
                {
                  /* start spinning */
                  thunar_shortcut_row_set_spinning (row, TRUE, 
                                                    THUNAR_SHORTCUT_ROW_EJECTING);

                  /* try unmounting the mount */
                  g_mount_unmount_with_operation (mount,
                                                  G_MOUNT_UNMOUNT_NONE,
                                                  mount_operation,
                                                  row->cancellable,
                                                  thunar_shortcut_row_mount_unmount_finish,
                                                  row);
                }
              else if (g_mount_can_unmount (mount))
                {
                  /* start spinning */
                  thunar_shortcut_row_set_spinning (row, TRUE, 
                                                    THUNAR_SHORTCUT_ROW_EJECTING);

                  /* try unmounting the mount */
                  g_mount_unmount_with_operation (mount,
                                                  G_MOUNT_UNMOUNT_NONE,
                                                  mount_operation,
                                                  row->cancellable,
                                                  thunar_shortcut_row_mount_unmount_finish,
                                                  row);
                }

              /* release the mount */
              g_object_unref (mount);
            }
        }
    }
  else
    {
      /* this method was called despite no mount or volume being available
       * for this shortcut... that should not happen */
    }

  /* release the mount operation */
  g_object_unref (mount_operation);
}
