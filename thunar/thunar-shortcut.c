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

#include <gio/gio.h>

#include <gdk/gdkkeysyms.h>

#include <gtk/gtk.h>

#include <thunar/thunar-browser.h>
#include <thunar/thunar-enum-types.h>
#include <thunar/thunar-dialogs.h>
#include <thunar/thunar-file.h>
#include <thunar/thunar-marshal.h>
#include <thunar/thunar-preferences.h>
#include <thunar/thunar-private.h>
#include <thunar/thunar-shortcut.h>



#define THUNAR_SHORTCUT_MIN_HEIGHT 20



/* property identifiers */
enum
{
  PROP_0,
  PROP_LOCATION,
  PROP_FILE,
  PROP_VOLUME,
  PROP_MOUNT,
  PROP_ICON,
  PROP_CUSTOM_ICON,
  PROP_EJECT_ICON,
  PROP_NAME,
  PROP_CUSTOM_NAME,
  PROP_SHORTCUT_TYPE,
  PROP_ICON_SIZE,
  PROP_HIDDEN,
  PROP_MUTABLE,
  PROP_PERSISTENT,
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
  THUNAR_SHORTCUT_STATE_NORMAL,
  THUNAR_SHORTCUT_STATE_RESOLVING,
  THUNAR_SHORTCUT_STATE_EJECTING,
} ThunarShortcutState;



static void         thunar_shortcut_constructed             (GObject            *object);
static void         thunar_shortcut_finalize                (GObject            *object);
static void         thunar_shortcut_get_property            (GObject            *object,
                                                             guint               prop_id,
                                                             GValue             *value,
                                                             GParamSpec         *pspec);
static void         thunar_shortcut_set_property            (GObject            *object,
                                                             guint               prop_id,
                                                             const GValue       *value,
                                                             GParamSpec         *pspec);
static gboolean     thunar_shortcut_button_press_event      (GtkWidget          *widget,
                                                             GdkEventButton     *event);
static gboolean     thunar_shortcut_button_release_event    (GtkWidget          *widget,
                                                             GdkEventButton     *event);
static gboolean     thunar_shortcut_key_press_event         (GtkWidget          *widget,
                                                             GdkEventKey        *event);
static gboolean     thunar_shortcut_enter_notify_event      (GtkWidget          *widget,
                                                             GdkEventCrossing   *event);
static gboolean     thunar_shortcut_leave_notify_event      (GtkWidget          *widget,
                                                             GdkEventCrossing   *event);
static gboolean     thunar_shortcut_expose_event            (GtkWidget          *widget,
                                                             GdkEventExpose     *event);
static gboolean     thunar_shortcut_focus                   (GtkWidget          *widget,
                                                             GtkDirectionType    direction);
static gboolean     thunar_shortcut_focus_in_event          (GtkWidget          *widget,
                                                             GdkEventFocus      *event);
static gboolean     thunar_shortcut_focus_out_event         (GtkWidget          *widget,
                                                             GdkEventFocus      *event);
static void         thunar_shortcut_size_request            (GtkWidget          *widget,
                                                             GtkRequisition     *requisition);
static void         thunar_shortcut_button_state_changed    (ThunarShortcut     *shortcut,
                                                             GtkStateType        previous_state,
                                                             GtkWidget          *button);
static void         thunar_shortcut_button_clicked          (ThunarShortcut     *shortcut,
                                                             GtkButton          *button);
static gboolean     thunar_shortcut_matches_types           (ThunarShortcut     *shortcut,
                                                             ThunarShortcutType  types);
static void         thunar_shortcut_location_changed        (ThunarShortcut     *shortcut);
static void         thunar_shortcut_file_changed            (ThunarShortcut     *shortcut);
static void         thunar_shortcut_shortcut_type_changed   (ThunarShortcut     *shortcut);
static void         thunar_shortcut_icon_changed            (ThunarShortcut     *shortcut);
static void         thunar_shortcut_resolve_location_finish (ThunarBrowser      *browser,
                                                             GFile              *location,
                                                             ThunarFile         *file,
                                                             ThunarFile         *target_file,
                                                             GError             *error,
                                                             gpointer            user_data);
static void         thunar_shortcut_mount_unmount_finish    (GObject            *object,
                                                             GAsyncResult       *result,
                                                             gpointer            user_data);
static void         thunar_shortcut_mount_eject_finish      (GObject            *object,
                                                             GAsyncResult       *result,
                                                             gpointer            user_data);
static void         thunar_shortcut_volume_eject_finish     (GObject            *object,
                                                             GAsyncResult       *result,
                                                             gpointer            user_data);
static void         thunar_shortcut_poke_volume_finish      (ThunarBrowser      *browser,
                                                             GVolume            *volume,
                                                             ThunarFile         *file,
                                                             GError             *error,
                                                             gpointer            user_data);
static void         thunar_shortcut_poke_file_finish        (ThunarBrowser      *browser,
                                                             ThunarFile         *file,
                                                             ThunarFile         *target_file,
                                                             GError             *error,
                                                             gpointer            user_data);
static void         thunar_shortcut_poke_location_finish    (ThunarBrowser      *browser,
                                                             GFile              *location,
                                                             ThunarFile         *file,
                                                             ThunarFile         *target_file,
                                                             GError             *error,
                                                             gpointer            user_data);
static void         thunar_shortcut_set_spinning            (ThunarShortcut     *shortcut,
                                                             gboolean            spinning,
                                                             ThunarShortcutState new_state);
static const gchar *thunar_shortcut_get_display_name        (ThunarShortcut     *shortcut);
static GIcon       *thunar_shortcut_get_display_icon        (ThunarShortcut     *shortcut);



struct _ThunarShortcutClass
{
  GtkEventBoxClass __parent__;
};

struct _ThunarShortcut
{
  GtkEventBox         __parent__;

  GFile              *location;
  GVolume            *volume;
  GMount             *mount;
  
  ThunarFile         *file;

  GIcon              *icon;
  GIcon              *custom_icon;
  GIcon              *eject_icon;
  
  gchar              *name;
  gchar              *custom_name;
  
  ThunarShortcutType  type;

  guint               hidden : 1;
  guint               mutable : 1;
  guint               persistent : 1;
  guint               constructed : 1;

  GtkWidget          *label_widget;
  GtkWidget          *icon_image;
  GtkWidget          *action_button;
  GtkWidget          *action_image;
  GtkWidget          *spinner;

  ThunarPreferences  *preferences;
  ThunarIconSize      icon_size;

  GCancellable       *cancellable;

  ThunarShortcutState state;
};



G_DEFINE_TYPE_WITH_CODE (ThunarShortcut, thunar_shortcut, GTK_TYPE_EVENT_BOX,
                         G_IMPLEMENT_INTERFACE (THUNAR_TYPE_BROWSER, NULL))



static guint shortcut_signals[LAST_SIGNAL];



static void
thunar_shortcut_class_init (ThunarShortcutClass *klass)
{
  GtkWidgetClass *gtkwidget_class;
  GObjectClass   *gobject_class;

  /* Determine the parent type class */
  thunar_shortcut_parent_class = g_type_class_peek_parent (klass);

  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->constructed = thunar_shortcut_constructed; 
  gobject_class->finalize = thunar_shortcut_finalize; 
  gobject_class->get_property = thunar_shortcut_get_property;
  gobject_class->set_property = thunar_shortcut_set_property;

  gtkwidget_class = GTK_WIDGET_CLASS (klass);
  gtkwidget_class->button_press_event = thunar_shortcut_button_press_event;
  gtkwidget_class->button_release_event = thunar_shortcut_button_release_event;
  gtkwidget_class->key_press_event = thunar_shortcut_key_press_event;
  gtkwidget_class->enter_notify_event = thunar_shortcut_enter_notify_event;
  gtkwidget_class->leave_notify_event = thunar_shortcut_leave_notify_event;
  gtkwidget_class->expose_event = thunar_shortcut_expose_event;
  gtkwidget_class->focus = thunar_shortcut_focus;
  gtkwidget_class->focus_in_event = thunar_shortcut_focus_in_event;
  gtkwidget_class->focus_out_event = thunar_shortcut_focus_out_event;
  gtkwidget_class->size_request = thunar_shortcut_size_request;

  g_object_class_install_property (gobject_class,
                                   PROP_LOCATION,
                                   g_param_spec_object ("location",
                                                        "location",
                                                        "location",
                                                        G_TYPE_FILE,
                                                        EXO_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT));

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
                                                        EXO_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT));

  g_object_class_install_property (gobject_class,
                                   PROP_MOUNT,
                                   g_param_spec_object ("mount",
                                                        "mount",
                                                        "mount",
                                                        G_TYPE_MOUNT,
                                                        EXO_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT));

  g_object_class_install_property (gobject_class,
                                   PROP_ICON,
                                   g_param_spec_object ("icon",
                                                        "icon",
                                                        "icon",
                                                        G_TYPE_ICON,
                                                        EXO_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT));

  g_object_class_install_property (gobject_class,
                                   PROP_CUSTOM_ICON,
                                   g_param_spec_object ("custom-icon",
                                                        "custom-icon",
                                                        "custom-icon",
                                                        G_TYPE_ICON,
                                                        EXO_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT));

  g_object_class_install_property (gobject_class,
                                   PROP_EJECT_ICON,
                                   g_param_spec_object ("eject-icon",
                                                        "eject-icon",
                                                        "eject-icon",
                                                        G_TYPE_ICON,
                                                        EXO_PARAM_READWRITE));

  g_object_class_install_property (gobject_class,
                                   PROP_NAME,
                                   g_param_spec_string ("name",
                                                        "name",
                                                        "name",
                                                        NULL,
                                                        EXO_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT));

  g_object_class_install_property (gobject_class,
                                   PROP_CUSTOM_NAME,
                                   g_param_spec_string ("custom-name",
                                                        "custom-name",
                                                        "custom-name",
                                                        NULL,
                                                        EXO_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT));

  g_object_class_install_property (gobject_class,
                                   PROP_SHORTCUT_TYPE,
                                   g_param_spec_flags ("shortcut-type",
                                                       "shortcut-type",
                                                       "shortcut-type",
                                                       THUNAR_TYPE_SHORTCUT_TYPE,
                                                       THUNAR_SHORTCUT_REGULAR_FILE,
                                                       EXO_PARAM_READWRITE |
                                                       G_PARAM_CONSTRUCT));

  g_object_class_install_property (gobject_class,
                                   PROP_ICON_SIZE,
                                   g_param_spec_enum ("icon-size",
                                                      "icon-size",
                                                      "icon-size",
                                                      THUNAR_TYPE_ICON_SIZE,
                                                      THUNAR_ICON_SIZE_SMALLER,
                                                      EXO_PARAM_READWRITE));

  g_object_class_install_property (gobject_class,
                                   PROP_HIDDEN,
                                   g_param_spec_boolean ("hidden",
                                                         "hidden",
                                                         "hidden",
                                                         FALSE,
                                                         EXO_PARAM_READWRITE |
                                                         G_PARAM_CONSTRUCT));

  g_object_class_install_property (gobject_class,
                                   PROP_MUTABLE,
                                   g_param_spec_boolean ("mutable",
                                                         "mutable",
                                                         "mutable",
                                                         FALSE,
                                                         EXO_PARAM_READWRITE |
                                                         G_PARAM_CONSTRUCT));

  g_object_class_install_property (gobject_class,
                                   PROP_PERSISTENT,
                                   g_param_spec_boolean ("persistent",
                                                         "persistent",
                                                         "persistent",
                                                         FALSE,
                                                         EXO_PARAM_READWRITE |
                                                         G_PARAM_CONSTRUCT));

  shortcut_signals[SIGNAL_ACTIVATED] = 
    g_signal_new (I_("activated"),
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_LAST,
                  0, NULL, NULL,
                  _thunar_marshal_VOID__OBJECT_BOOLEAN,
                  G_TYPE_NONE, 2, 
                  THUNAR_TYPE_FILE,
                  G_TYPE_BOOLEAN);

  shortcut_signals[SIGNAL_CONTEXT_MENU] =
    g_signal_new (I_("context-menu"),
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_LAST,
                  0, NULL, NULL,
                  g_cclosure_marshal_VOID__VOID,
                  G_TYPE_NONE, 0);
}



static void
thunar_shortcut_init (ThunarShortcut *shortcut)
{
  GtkWidget *alignment;
  GtkWidget *box;

  /* create a cancellable for aborting mount/unmount operations */
  shortcut->cancellable = g_cancellable_new ();

  /* set the shortcut state to normal */
  shortcut->state = THUNAR_SHORTCUT_STATE_NORMAL;

  /* configure general widget behavior */
  gtk_widget_set_can_focus (GTK_WIDGET (shortcut), TRUE);
  gtk_widget_set_sensitive (GTK_WIDGET (shortcut), TRUE);

  /* create the alignment for left and right padding */
  alignment = gtk_alignment_new (0.0f, 0.0f, 1.0f, 1.0f);
  /* TODO use expander arrow width instead of 16 here */
  gtk_alignment_set_padding (GTK_ALIGNMENT (alignment), 0, 0, 16, 4);
  gtk_container_add (GTK_CONTAINER (shortcut), alignment);
  gtk_widget_show (alignment);

  /* create a box for the different sub-widgets */
  box = gtk_hbox_new (FALSE, 4);
  gtk_container_add (GTK_CONTAINER (alignment), box);
  gtk_widget_show (box);

  /* create the icon widget */
  shortcut->icon_image = gtk_image_new ();
  gtk_box_pack_start (GTK_BOX (box), shortcut->icon_image, FALSE, TRUE, 0);
  gtk_widget_hide (shortcut->icon_image);

  /* create the label widget */
  shortcut->label_widget = gtk_label_new (NULL);
  gtk_label_set_ellipsize (GTK_LABEL (shortcut->label_widget), PANGO_ELLIPSIZE_END);
  gtk_misc_set_alignment (GTK_MISC (shortcut->label_widget), 0.0f, 0.5f);
  gtk_container_add (GTK_CONTAINER (box), shortcut->label_widget);
  gtk_widget_hide (shortcut->label_widget);

  /* create the action button */
  shortcut->action_button = gtk_button_new ();
  gtk_button_set_relief (GTK_BUTTON (shortcut->action_button), GTK_RELIEF_NONE);
  gtk_box_pack_start (GTK_BOX (box), shortcut->action_button, FALSE, TRUE, 0);
  gtk_widget_hide (shortcut->action_button);

  /* adjust the state transitions of the button */
  g_signal_connect_swapped (shortcut->action_button, "state-changed",
                            G_CALLBACK (thunar_shortcut_button_state_changed), shortcut);

  /* react on button click events */
  g_signal_connect_swapped (shortcut->action_button, "clicked",
                            G_CALLBACK (thunar_shortcut_button_clicked), shortcut);

  /* create the action button image */
  shortcut->action_image = gtk_image_new ();
  gtk_button_set_image (GTK_BUTTON (shortcut->action_button), shortcut->action_image);
  gtk_widget_show (shortcut->action_image);

  /* take a reference because we need to be able to remove it from the
   * button temporarily */
  g_object_ref (shortcut->action_image);

  /* default to "media-eject", we only set this for the button size
  * to be computed so that all shortcuts have equal heights */
  gtk_image_set_from_icon_name (GTK_IMAGE (shortcut->action_image), "media-eject",
                                GTK_ICON_SIZE_MENU);

  /* create the spinner icon */
  shortcut->spinner = gtk_spinner_new ();
  gtk_spinner_stop (GTK_SPINNER (shortcut->spinner));

  /* take a reference because we need to be able to remove it from the
   * button temporarily */
  g_object_ref (shortcut->spinner);

  /* update the icon size whenever necessary */
  shortcut->preferences = thunar_preferences_get ();
  exo_binding_new (G_OBJECT (shortcut->preferences), "shortcuts-icon-size",
                   G_OBJECT (shortcut), "icon-size");
}



static void
thunar_shortcut_constructed (GObject *object)
{
  ThunarShortcut *shortcut = THUNAR_SHORTCUT (object);

  /* mark as constructed so now all properties can be changed as usual */
  shortcut->constructed = TRUE;
}



static void
thunar_shortcut_finalize (GObject *object)
{
  ThunarShortcut *shortcut = THUNAR_SHORTCUT (object);

  /* release the spinner and action image */
  g_object_unref (shortcut->spinner);
  g_object_unref (shortcut->action_image);

  /* release the cancellable */
  g_cancellable_cancel (shortcut->cancellable);
  g_object_unref (shortcut->cancellable);

  /* release the preferences */
  g_object_unref (shortcut->preferences);

  if (shortcut->location != NULL)
    g_object_unref (shortcut->location);

  if (shortcut->file != NULL)
    g_object_unref (shortcut->file);

  if (shortcut->volume != NULL)
    g_object_unref (shortcut->volume);

  if (shortcut->mount != NULL)
    g_object_unref (shortcut->mount);

  if (shortcut->icon != NULL)
    g_object_unref (shortcut->icon);

  if (shortcut->custom_icon != NULL)
    g_object_unref (shortcut->custom_icon);

  if (shortcut->eject_icon != NULL)
    g_object_unref (shortcut->eject_icon);

  g_free (shortcut->name);
  g_free (shortcut->custom_name);

  (*G_OBJECT_CLASS (thunar_shortcut_parent_class)->finalize) (object);
}



static void
thunar_shortcut_get_property (GObject    *object,
                              guint       prop_id,
                              GValue     *value,
                              GParamSpec *pspec)
{
  ThunarShortcut *shortcut = THUNAR_SHORTCUT (object);

  switch (prop_id)
    {
    case PROP_LOCATION:
      g_value_set_object (value, thunar_shortcut_get_location (shortcut));
      break;

    case PROP_FILE:
      g_value_set_object (value, thunar_shortcut_get_file (shortcut));
      break;

    case PROP_VOLUME:
      g_value_set_object (value, thunar_shortcut_get_volume (shortcut));
      break;

    case PROP_MOUNT:
      g_value_set_object (value, thunar_shortcut_get_mount (shortcut));
      break;

    case PROP_ICON:
      g_value_set_object (value, thunar_shortcut_get_icon (shortcut));
      break;

    case PROP_CUSTOM_ICON:
      g_value_set_object (value, thunar_shortcut_get_custom_icon (shortcut));
      break;

    case PROP_EJECT_ICON:
      g_value_set_object (value, thunar_shortcut_get_eject_icon (shortcut));
      break;

    case PROP_NAME:
      g_value_set_string (value, thunar_shortcut_get_name (shortcut));
      break;

    case PROP_CUSTOM_NAME:
      g_value_set_string (value, thunar_shortcut_get_custom_name (shortcut));
      break;

    case PROP_SHORTCUT_TYPE:
      g_value_set_flags (value, thunar_shortcut_get_shortcut_type (shortcut));
      break;

    case PROP_ICON_SIZE:
      g_value_set_enum (value, thunar_shortcut_get_icon_size (shortcut));
      break;

    case PROP_HIDDEN:
      g_value_set_boolean (value, thunar_shortcut_get_hidden (shortcut));
      break;

    case PROP_MUTABLE:
      g_value_set_boolean (value, thunar_shortcut_get_mutable (shortcut));
      break;

    case PROP_PERSISTENT:
      g_value_set_boolean (value, thunar_shortcut_get_persistent (shortcut));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}



static void
thunar_shortcut_set_property (GObject      *object,
                              guint         prop_id,
                              const GValue *value,
                              GParamSpec   *pspec)
{
  ThunarShortcut *shortcut = THUNAR_SHORTCUT (object);

  switch (prop_id)
    {
    case PROP_LOCATION:
      thunar_shortcut_set_location (shortcut, g_value_get_object (value));
      break;

    case PROP_FILE:
      thunar_shortcut_set_file (shortcut, g_value_get_object (value));
      break;

    case PROP_VOLUME:
      thunar_shortcut_set_volume (shortcut, g_value_get_object (value));
      break;

    case PROP_MOUNT:
      thunar_shortcut_set_mount (shortcut, g_value_get_object (value));
      break;

    case PROP_ICON:
      thunar_shortcut_set_icon (shortcut, g_value_get_object (value));
      break;

    case PROP_CUSTOM_ICON:
      thunar_shortcut_set_custom_icon (shortcut, g_value_get_object (value));
      break;

    case PROP_EJECT_ICON:
      thunar_shortcut_set_eject_icon (shortcut, g_value_get_object (value));
      break;

    case PROP_NAME:
      thunar_shortcut_set_name (shortcut, g_value_get_string (value));
      break;

    case PROP_CUSTOM_NAME:
      thunar_shortcut_set_custom_name (shortcut, g_value_get_string (value));
      break;

    case PROP_SHORTCUT_TYPE:
      thunar_shortcut_set_shortcut_type (shortcut, g_value_get_flags (value));
      break;

    case PROP_ICON_SIZE:
      thunar_shortcut_set_icon_size (shortcut, g_value_get_enum (value));
      break;

    case PROP_HIDDEN:
      thunar_shortcut_set_hidden (shortcut, g_value_get_boolean (value));
      break;

    case PROP_MUTABLE:
      thunar_shortcut_set_mutable (shortcut, g_value_get_boolean (value));
      break;

    case PROP_PERSISTENT:
      thunar_shortcut_set_persistent (shortcut, g_value_get_boolean (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}



static gboolean
thunar_shortcut_button_press_event (GtkWidget      *widget,
                                    GdkEventButton *event)
{
  GtkStateType state;

  _thunar_return_val_if_fail (THUNAR_IS_SHORTCUT (widget), FALSE);

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
        thunar_shortcut_resolve_and_activate (THUNAR_SHORTCUT (widget), FALSE);
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
thunar_shortcut_button_release_event (GtkWidget      *widget,
                                      GdkEventButton *event)
{
  _thunar_return_val_if_fail (THUNAR_IS_SHORTCUT (widget), FALSE);

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
thunar_shortcut_key_press_event (GtkWidget   *widget,
                                 GdkEventKey *event)
{
  gboolean new_window;

  _thunar_return_val_if_fail (THUNAR_IS_SHORTCUT (widget), FALSE);

  if (event->keyval == GDK_KEY_Return
      || event->keyval == GDK_KEY_KP_Enter
      || event->keyval == GDK_KEY_space
      || event->keyval== GDK_KEY_KP_Space)
    {
      new_window = (event->state & GDK_CONTROL_MASK) != 0;
      thunar_shortcut_resolve_and_activate (THUNAR_SHORTCUT (widget), new_window);
      return TRUE;
    }

  return FALSE;
}



static gboolean
thunar_shortcut_enter_notify_event (GtkWidget        *widget,
                                    GdkEventCrossing *event)
{
  _thunar_return_val_if_fail (THUNAR_IS_SHORTCUT (widget), FALSE);

  if (gtk_widget_get_state (widget) != GTK_STATE_SELECTED)
    gtk_widget_set_state (widget, GTK_STATE_PRELIGHT);

  return TRUE;
}



static gboolean
thunar_shortcut_leave_notify_event (GtkWidget        *widget,
                                    GdkEventCrossing *event)
{
  ThunarShortcut *shortcut = THUNAR_SHORTCUT (widget);
  gint            x;
  gint            y;
  gint            width;
  gint            height;

  _thunar_return_val_if_fail (THUNAR_IS_SHORTCUT (widget), FALSE);

  /* check whether the shortcut is hovered but not selected */
  if (gtk_widget_get_state (widget) == GTK_STATE_PRELIGHT)
    {
      /* determine the location of the pointer relative to the action button */
      gtk_widget_get_pointer (shortcut->action_button, &x, &y);

      /* determine the geometry of the button window */
      gdk_window_get_geometry (gtk_widget_get_window (shortcut->action_button),
                               NULL, NULL, &width, &height, NULL);

      /* the pointer has left the shortcut widget itself but we only
       * reset the prelight state if the mouse is not hovering
       * the button either */
      if (x <= 0 || y <= 0 || x >= width || y >= height)
        gtk_widget_set_state (widget, GTK_STATE_NORMAL);
    }

  return TRUE;
}



static gboolean
thunar_shortcut_expose_event (GtkWidget      *widget,
                              GdkEventExpose *event)
{
  GtkStateType state;
  GList       *children;
  GList       *lp;

  _thunar_return_val_if_fail (THUNAR_IS_SHORTCUT (widget), FALSE);

  /* determine the widget state */
  state = gtk_widget_get_state (widget);

  /* paint a flat box that gives the shortcut the same look as a
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
thunar_shortcut_focus (GtkWidget       *widget,
                       GtkDirectionType direction)
{
  ThunarShortcut *shortcut = THUNAR_SHORTCUT (widget);

  _thunar_return_val_if_fail (THUNAR_IS_SHORTCUT (widget), FALSE);

  switch (direction)
    {
    case GTK_DIR_TAB_FORWARD:
    case GTK_DIR_TAB_BACKWARD:
      return FALSE;

    case GTK_DIR_UP:
      if (gtk_widget_is_focus (widget) || gtk_widget_is_focus (shortcut->action_button))
        {
          return FALSE;
        }
      else
        {
          gtk_widget_grab_focus (widget);
          return TRUE;
        }

    case GTK_DIR_DOWN:
      if (gtk_widget_is_focus (widget) || gtk_widget_is_focus (shortcut->action_button))
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
      if (gtk_widget_get_visible (shortcut->action_button))
        {
          gtk_widget_grab_focus (shortcut->action_button);
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
thunar_shortcut_focus_in_event (GtkWidget     *widget,
                                GdkEventFocus *event)
{
  _thunar_return_val_if_fail (THUNAR_IS_SHORTCUT (widget), FALSE);

  gtk_widget_set_state (widget, GTK_STATE_SELECTED);
  return TRUE;
}



static gboolean
thunar_shortcut_focus_out_event (GtkWidget     *widget,
                                 GdkEventFocus *event)
{
  _thunar_return_val_if_fail (THUNAR_IS_SHORTCUT (widget), FALSE);

  gtk_widget_set_state (widget, GTK_STATE_NORMAL);
  return TRUE;
}



static void
thunar_shortcut_size_request (GtkWidget      *widget,
                              GtkRequisition *requisition)
{
  ThunarShortcut *shortcut = THUNAR_SHORTCUT (widget);
  GtkRequisition  button_requisition;

  _thunar_return_if_fail (THUNAR_IS_SHORTCUT (shortcut));

  /* let the event box class compute the size we need for its children */
  (*GTK_WIDGET_CLASS (thunar_shortcut_parent_class)->size_request) (widget, requisition);

  /* compute the button size */
  gtk_widget_size_request (shortcut->action_button, &button_requisition);

  /* use the maximum of the computed requisition height, the button height,
   * the icon size + 4, and the minimum allowed height for shortcuts */
  requisition->height = MAX (requisition->height, button_requisition.height);
  requisition->height = MAX (requisition->height, (gint) shortcut->icon_size + 4);
  requisition->height = MAX (requisition->height, THUNAR_SHORTCUT_MIN_HEIGHT);
}



static void
thunar_shortcut_button_state_changed (ThunarShortcut *shortcut,
                                      GtkStateType    previous_state,
                                      GtkWidget      *button)
{
  gint x;
  gint y;
  gint width;
  gint height;

  _thunar_return_if_fail (THUNAR_IS_SHORTCUT (shortcut));

  if (gtk_widget_get_state (shortcut->action_button) == GTK_STATE_PRELIGHT
      && gtk_widget_get_state (GTK_WIDGET (shortcut)) == GTK_STATE_PRELIGHT)
    {
      gtk_widget_get_pointer (shortcut->action_button, &x, &y);

      gdk_window_get_geometry (gtk_widget_get_window (shortcut->action_button),
                               NULL, NULL, &width, &height, NULL);

      if (x <= 0 || y <= 0 || x >= width || y >= height)
        gtk_widget_set_state (shortcut->action_button, GTK_STATE_NORMAL);
    }
}



static void
thunar_shortcut_button_clicked (ThunarShortcut *shortcut,
                                GtkButton      *button)
{
  _thunar_return_if_fail (THUNAR_IS_SHORTCUT (shortcut));

  /* check if we are currently mounting/ejecting something */
  if (shortcut->state != THUNAR_SHORTCUT_STATE_NORMAL)
    {
      /* abort the mount/eject process */
      g_cancellable_cancel (shortcut->cancellable);

      /* we're done, no further processing please */
      return;
    }

  /* disconnect the shortcut */
  thunar_shortcut_disconnect (shortcut);
}



static gboolean
thunar_shortcut_matches_types (ThunarShortcut    *shortcut,
                               ThunarShortcutType types)
{
  _thunar_return_val_if_fail (THUNAR_IS_SHORTCUT (shortcut), FALSE);
  return (shortcut->type & types) != 0;
}



static void
thunar_shortcut_location_changed (ThunarShortcut *shortcut)
{
  GIcon *icon;
  gchar *uri;
  gchar *base_name;

  _thunar_return_if_fail (THUNAR_IS_SHORTCUT (shortcut));

  /* only update the shortcut if this is a file shortcut */
  if (!thunar_shortcut_matches_types (shortcut, 
                                      THUNAR_SHORTCUT_REGULAR_FILE
                                      | THUNAR_SHORTCUT_NETWORK_FILE
                                      | THUNAR_SHORTCUT_TRASH_FILE))
    {
      return;
    }
  
  /* check whether we have a location */
  if (shortcut->location != NULL)
    {
      /* use the location base name as the default name */
      uri = g_file_get_uri (shortcut->location);
      base_name = g_filename_display_basename (uri);
      thunar_shortcut_set_name (shortcut, base_name);
      g_free (base_name);
      g_free (uri);

      /* use the folder icon as the default icon */
      if (shortcut->icon == NULL)
        {
          icon = g_themed_icon_new ("folder");
          thunar_shortcut_set_icon (shortcut, icon);
          g_object_unref (icon);
        }

      /* load additional file information asynchronously */
      thunar_browser_poke_location (THUNAR_BROWSER (shortcut),
                                    shortcut->location,
                                    shortcut,
                                    thunar_shortcut_resolve_location_finish,
                                    NULL);
    }
  else
    {
      /* if this is reached, there is something wrong with the
       * shortcut classification */
      _thunar_assert (!shortcut->constructed);
    }
}



static void
thunar_shortcut_file_changed (ThunarShortcut *shortcut)
{
  const gchar *display_name;
  GIcon       *icon;

  _thunar_return_if_fail (THUNAR_IS_SHORTCUT (shortcut));

  /* only update the information if this is a file shortcut */
  if (!thunar_shortcut_matches_types (shortcut, 
                                      THUNAR_SHORTCUT_REGULAR_FILE
                                      | THUNAR_SHORTCUT_NETWORK_FILE))
    {
      return;
    }

  /* update only if we have a file */
  if (shortcut->file != NULL)
    {
      /* update the name of the shortcut */
      display_name = thunar_file_get_display_name (shortcut->file);
      thunar_shortcut_set_name (shortcut, display_name);

      /* update the icon of the shortcut */
      icon = thunar_file_get_icon (shortcut->file);
      thunar_shortcut_set_icon (shortcut, icon);
    }
}



static void
thunar_shortcut_name_changed (ThunarShortcut *shortcut)
{
  const gchar *name;

  _thunar_return_if_fail (THUNAR_IS_SHORTCUT (shortcut));

  /* get the name (custom name overrides the regular name) */
  name = thunar_shortcut_get_display_name (shortcut);

  /* update the label widget */
  gtk_label_set_text (GTK_LABEL (shortcut->label_widget), name);
  gtk_widget_set_visible (shortcut->label_widget, name != NULL && *name != '\0');
}



static void
thunar_shortcut_volume_changed (ThunarShortcut *shortcut)
{
  GIcon *icon;
  gchar *name;

  _thunar_return_if_fail (THUNAR_IS_SHORTCUT (shortcut));

  /* don't update if this is not a volume */
  if (!thunar_shortcut_matches_types (shortcut,
                                      THUNAR_SHORTCUT_REGULAR_VOLUME
                                      | THUNAR_SHORTCUT_EJECTABLE_VOLUME))
    {
      return;
    }

  /* only update if we have a volume */
  if (shortcut->volume != NULL)
    {
      /* update the name of the shortcut */
      name = g_volume_get_name (shortcut->volume);
      thunar_shortcut_set_name (shortcut, name);
      g_free (name);

      /* update the icon of the shortcut */
      icon = g_volume_get_icon (shortcut->volume);
      thunar_shortcut_set_icon (shortcut, icon);
      g_object_unref (icon);
    }
  else
    {
      /* if this is reached, there is something wrong the 
       * shortcut classification */
      _thunar_assert(!shortcut->constructed);
    }
}



static void
thunar_shortcut_mount_changed (ThunarShortcut *shortcut)
{
  GIcon *icon;
  gchar *name;

  _thunar_return_if_fail (THUNAR_IS_SHORTCUT (shortcut));

  /* don't update the information if this is not a mount shortcut */
  if (!thunar_shortcut_matches_types (shortcut,
                                      THUNAR_SHORTCUT_REGULAR_MOUNT 
                                      | THUNAR_SHORTCUT_ARCHIVE_MOUNT
                                      | THUNAR_SHORTCUT_NETWORK_MOUNT))
    {
      return;
    }

  /* only update if we have a mount */
  if (shortcut->mount != NULL)
    {
      /* update the name of the shortcut */
      name = g_mount_get_name (shortcut->mount);
      thunar_shortcut_set_name (shortcut, name);
      g_free (name);

      /* update the icon of the shortcut  */
      icon = g_mount_get_icon (shortcut->mount);
      thunar_shortcut_set_icon (shortcut, icon);
      g_object_unref (icon);

      /* update the action button */
      if (g_mount_can_unmount (shortcut->mount) || g_mount_can_eject (shortcut->mount))
        gtk_widget_set_visible (shortcut->action_button, TRUE);
      else
        gtk_widget_set_visible (shortcut->action_button, FALSE);
    }
  else
    {
      /* if this is reached, there is something wrong the 
       * shortcut classification */
      _thunar_assert (!shortcut->constructed);
    }
}



static void
thunar_shortcut_shortcut_type_changed (ThunarShortcut *shortcut)
{
  _thunar_return_if_fail (THUNAR_IS_SHORTCUT (shortcut));

  /* update the shortcut depending on the type */
  switch (shortcut->type)
    {
    case THUNAR_SHORTCUT_REGULAR_FILE:
    case THUNAR_SHORTCUT_NETWORK_FILE:
    case THUNAR_SHORTCUT_TRASH_FILE:
      thunar_shortcut_location_changed (shortcut);
      break;

    case THUNAR_SHORTCUT_REGULAR_MOUNT:
    case THUNAR_SHORTCUT_ARCHIVE_MOUNT:
    case THUNAR_SHORTCUT_NETWORK_MOUNT:
      thunar_shortcut_mount_changed (shortcut);
      break;

    case THUNAR_SHORTCUT_EJECTABLE_VOLUME:
    case THUNAR_SHORTCUT_REGULAR_VOLUME:
      thunar_shortcut_volume_changed (shortcut);
      break;

    default:
      _thunar_assert_not_reached ();
      break;
    }
}



static void
thunar_shortcut_icon_changed (ThunarShortcut *shortcut)
{
  GIcon *icon;

  _thunar_return_if_fail (THUNAR_IS_SHORTCUT (shortcut));

  /* get the icon (custom icon overrides the regular icon) */
  icon = thunar_shortcut_get_display_icon (shortcut);

  /* update the icon image */
  gtk_image_set_from_gicon (GTK_IMAGE (shortcut->icon_image), icon, shortcut->icon_size);

  /* update the icon's visibility */
  gtk_widget_set_visible (shortcut->icon_image, icon != NULL);
}



static void
thunar_shortcut_icon_size_changed (ThunarShortcut *shortcut)
{
  _thunar_return_if_fail (THUNAR_IS_SHORTCUT (shortcut));

  gtk_image_set_pixel_size (GTK_IMAGE (shortcut->icon_image), shortcut->icon_size);
  gtk_image_set_pixel_size (GTK_IMAGE (shortcut->action_image), shortcut->icon_size);

  gtk_widget_set_size_request (shortcut->spinner,
                               shortcut->icon_size,
                               shortcut->icon_size);
}



static void
thunar_shortcut_hidden_changed (ThunarShortcut *shortcut)
{
  _thunar_return_if_fail (THUNAR_IS_SHORTCUT (shortcut));

  gtk_widget_set_visible (GTK_WIDGET (shortcut), !!shortcut->hidden);
}



static void
thunar_shortcut_resolve_location_finish (ThunarBrowser *browser,
                                         GFile         *location,
                                         ThunarFile    *file,
                                         ThunarFile    *target_file,
                                         GError        *error,
                                         gpointer       user_data)
{
  ThunarShortcut *shortcut = THUNAR_SHORTCUT (browser);

  _thunar_return_if_fail (THUNAR_IS_SHORTCUT (shortcut));
  _thunar_return_if_fail (G_IS_FILE (location));
  _thunar_return_if_fail (file == NULL || THUNAR_IS_FILE (file));
  _thunar_return_if_fail (target_file == NULL || THUNAR_IS_FILE (target_file));

  /* assign the file to the shortcut */
  thunar_shortcut_set_file (shortcut, target_file);
}



static void
thunar_shortcut_mount_unmount_finish (GObject      *object,
                                      GAsyncResult *result,
                                      gpointer      user_data)
{
  ThunarShortcut *shortcut = THUNAR_SHORTCUT (user_data);
  const gchar    *name;
  GMount         *mount = G_MOUNT (object);
  GError         *error = NULL;

  _thunar_return_if_fail (THUNAR_IS_SHORTCUT (shortcut));
  _thunar_return_if_fail (G_IS_MOUNT (mount));
  _thunar_return_if_fail (G_IS_ASYNC_RESULT (result));

  /* stop spinning */
  thunar_shortcut_set_spinning (shortcut, FALSE, THUNAR_SHORTCUT_STATE_NORMAL);

  if (!g_mount_unmount_with_operation_finish (mount, result, &error))
    {
      name = thunar_shortcut_get_display_name (shortcut);
      thunar_dialogs_show_error (GTK_WIDGET (shortcut), error,
                                 _("Failed to eject \"%s\""), name);
      g_error_free (error);
    }
}



static void
thunar_shortcut_mount_eject_finish (GObject      *object,
                                    GAsyncResult *result,
                                    gpointer      user_data)
{
  ThunarShortcut *shortcut = THUNAR_SHORTCUT (user_data);
  const gchar    *name;
  GMount         *mount = G_MOUNT (object);
  GError         *error = NULL;

  /* stop spinning */
  thunar_shortcut_set_spinning (shortcut, FALSE, THUNAR_SHORTCUT_STATE_NORMAL);

  _thunar_return_if_fail (THUNAR_IS_SHORTCUT (shortcut));
  _thunar_return_if_fail (G_IS_MOUNT (mount));
  _thunar_return_if_fail (G_IS_ASYNC_RESULT (result));

  if (!g_mount_eject_with_operation_finish (mount, result, &error))
    {
      name = thunar_shortcut_get_display_name (shortcut);
      thunar_dialogs_show_error (GTK_WIDGET (shortcut), error,
                                 _("Failed to eject \"%s\""), name);
      g_error_free (error);
    }
}



static void
thunar_shortcut_volume_eject_finish (GObject      *object,
                                     GAsyncResult *result,
                                     gpointer      user_data)
{
  ThunarShortcut *shortcut = THUNAR_SHORTCUT (user_data);
  const gchar    *name;
  GVolume        *volume = G_VOLUME (object);
  GError         *error = NULL;

  _thunar_return_if_fail (THUNAR_IS_SHORTCUT (shortcut));
  _thunar_return_if_fail (G_IS_VOLUME (volume));
  _thunar_return_if_fail (G_IS_ASYNC_RESULT (result));

  /* stop spinning */
  thunar_shortcut_set_spinning (shortcut, FALSE, THUNAR_SHORTCUT_STATE_NORMAL);

  if (!g_volume_eject_with_operation_finish (volume, result, &error))
    {
      name = thunar_shortcut_get_display_name (shortcut);
      thunar_dialogs_show_error (GTK_WIDGET (shortcut), error,
                                 _("Failed to eject \"%s\""), name);
      g_error_free (error);
    }
}



static void
thunar_shortcut_poke_volume_finish (ThunarBrowser *browser,
                                    GVolume       *volume,
                                    ThunarFile    *file,
                                    GError        *error,
                                    gpointer       user_data)
{
  ThunarShortcut *shortcut = THUNAR_SHORTCUT (browser);
  const gchar    *name;
  gboolean        open_in_new_window = GPOINTER_TO_UINT (user_data);

  _thunar_return_if_fail (THUNAR_IS_SHORTCUT (browser));
  _thunar_return_if_fail (G_IS_VOLUME (volume));
  _thunar_return_if_fail (file == NULL || THUNAR_IS_FILE (file));

  /* deactivate the spinner */
  thunar_shortcut_set_spinning (shortcut, FALSE, THUNAR_SHORTCUT_STATE_NORMAL);

  if (error == NULL)
    {
      g_signal_emit (shortcut, shortcut_signals[SIGNAL_ACTIVATED], 0, file,
                     open_in_new_window);
    }
  else
    {
      name = thunar_shortcut_get_display_name (shortcut);
      thunar_dialogs_show_error (GTK_WIDGET (shortcut), error,
                                 _("Failed to open \"%s\""), name);
    }
}



static void
thunar_shortcut_poke_file_finish (ThunarBrowser *browser,
                                  ThunarFile    *file,
                                  ThunarFile    *target_file,
                                  GError        *error,
                                  gpointer       user_data)
{
  ThunarShortcut *shortcut = THUNAR_SHORTCUT (browser);
  const gchar    *name;
  gboolean        open_in_new_window = GPOINTER_TO_UINT (user_data);

  _thunar_return_if_fail (THUNAR_IS_SHORTCUT (browser));
  _thunar_return_if_fail (THUNAR_IS_FILE (file));
  _thunar_return_if_fail (target_file == NULL || THUNAR_IS_FILE (target_file));

  /* deactivate the spinner */
  thunar_shortcut_set_spinning (shortcut, FALSE, THUNAR_SHORTCUT_STATE_NORMAL);

  if (error == NULL)
    {
      g_signal_emit (shortcut, shortcut_signals[SIGNAL_ACTIVATED], 0, target_file, 
                     open_in_new_window);
    }
  else
    {
      name = thunar_shortcut_get_display_name (shortcut);
      thunar_dialogs_show_error (GTK_WIDGET (shortcut), error,
                                 _("Failed to open \"%s\""), name);
    }
}



static void
thunar_shortcut_poke_location_finish (ThunarBrowser *browser,
                                      GFile         *location,
                                      ThunarFile    *file,
                                      ThunarFile    *target_file,
                                      GError        *error,
                                      gpointer       user_data)
{
  ThunarShortcut *shortcut = THUNAR_SHORTCUT (browser);
  const gchar    *name;
  gboolean        open_in_new_window = GPOINTER_TO_UINT (user_data);

  _thunar_return_if_fail (THUNAR_IS_SHORTCUT (browser));
  _thunar_return_if_fail (G_IS_FILE (location));
  _thunar_return_if_fail (file == NULL || THUNAR_IS_FILE (file));
  _thunar_return_if_fail (target_file == NULL || THUNAR_IS_FILE (target_file));

  /* deactivate the spinner */
  thunar_shortcut_set_spinning (shortcut, FALSE, THUNAR_SHORTCUT_STATE_NORMAL);

  if (error == NULL)
    {
      g_signal_emit (shortcut, shortcut_signals[SIGNAL_ACTIVATED], 0, target_file, 
                     open_in_new_window);
    }
  else
    {
      name = thunar_shortcut_get_display_name (shortcut);
      thunar_dialogs_show_error (GTK_WIDGET (shortcut), error,
                                 _("Failed to open \"%s\""), name);
    }
}



static void
thunar_shortcut_set_spinning (ThunarShortcut     *shortcut,
                              gboolean            spinning,
                              ThunarShortcutState new_state)
{
  _thunar_return_if_fail (THUNAR_IS_SHORTCUT (shortcut));

  /* apply the new state */
  shortcut->state = new_state;

  /* reset the cancelable so that we can use it to cancel the process */
  g_cancellable_reset (shortcut->cancellable);

  if (spinning)
    {
      gtk_button_set_image (GTK_BUTTON (shortcut->action_button), shortcut->spinner);
      gtk_spinner_start (GTK_SPINNER (shortcut->spinner));
      gtk_widget_show (shortcut->spinner);
      gtk_widget_show (shortcut->action_button);
    }
  else
    {
      gtk_button_set_image (GTK_BUTTON (shortcut->action_button),
                            shortcut->action_image);
      gtk_spinner_stop (GTK_SPINNER (shortcut->spinner));
      gtk_widget_hide (shortcut->spinner);
      gtk_widget_hide (shortcut->action_button);

      /* assume the mount and volume have changed which will make 
       * the action button visible again if the volume or mount is 
       * mounted and can be ejected */
      thunar_shortcut_mount_changed (shortcut);
      thunar_shortcut_volume_changed (shortcut);
    }
}



static const gchar *
thunar_shortcut_get_display_name (ThunarShortcut *shortcut)
{
  const gchar *label;

  _thunar_return_val_if_fail (THUNAR_IS_SHORTCUT (shortcut), NULL);
  
  label = thunar_shortcut_get_custom_name (shortcut);
  if (label == NULL)
    label = thunar_shortcut_get_name (shortcut);

  return label;
}



static GIcon *
thunar_shortcut_get_display_icon (ThunarShortcut *shortcut)
{
  GIcon *icon;

  _thunar_return_val_if_fail (THUNAR_IS_SHORTCUT (shortcut), NULL);

  icon = thunar_shortcut_get_custom_icon (shortcut);
  if (icon == NULL)
    icon = thunar_shortcut_get_icon (shortcut);

  return icon;
}



GFile *
thunar_shortcut_get_location (ThunarShortcut *shortcut)
{
  _thunar_return_val_if_fail (THUNAR_IS_SHORTCUT (shortcut), NULL);
  return shortcut->location;
}



void
thunar_shortcut_set_location (ThunarShortcut *shortcut,
                              GFile          *location)
{
  _thunar_return_if_fail (THUNAR_IS_SHORTCUT (shortcut));
  _thunar_return_if_fail (location == NULL || G_IS_FILE (location));

  if (location == NULL && !shortcut->constructed)
    return;

  if (shortcut->location == location)
    return;

  if (shortcut->location != NULL)
    g_object_unref (shortcut->location);

  if (location != NULL)
    shortcut->location = g_object_ref (location);
  else
    shortcut->location = NULL;

  thunar_shortcut_location_changed (shortcut);

  g_object_notify (G_OBJECT (shortcut), "location");
}



ThunarFile *
thunar_shortcut_get_file (ThunarShortcut *shortcut)
{
  _thunar_return_val_if_fail (THUNAR_IS_SHORTCUT (shortcut), NULL);
  return shortcut->file;
}



void
thunar_shortcut_set_file (ThunarShortcut *shortcut,
                          ThunarFile     *file)
{
  _thunar_return_if_fail (THUNAR_IS_SHORTCUT (shortcut));
  _thunar_return_if_fail (file == NULL || THUNAR_IS_FILE (file));

  if (file == NULL && !shortcut->constructed)
    return;

  if (shortcut->file == file)
    return;

  if (shortcut->file != NULL)
    g_object_unref (shortcut->file);

  if (file != NULL)
    shortcut->file = g_object_ref (file);
  else
    shortcut->file = NULL;

  thunar_shortcut_file_changed (shortcut);

  g_object_notify (G_OBJECT (shortcut), "file");
}



GVolume *
thunar_shortcut_get_volume (ThunarShortcut *shortcut)
{
  _thunar_return_val_if_fail (THUNAR_IS_SHORTCUT (shortcut), NULL);
  return shortcut->volume;
}



void
thunar_shortcut_set_volume (ThunarShortcut *shortcut,
                            GVolume        *volume)
{
  _thunar_return_if_fail (THUNAR_IS_SHORTCUT (shortcut));
  _thunar_return_if_fail (volume == NULL || G_IS_VOLUME (volume));

  if (volume == NULL && !shortcut->constructed)
    return;

  if (shortcut->volume == volume)
    return;

  if (shortcut->volume != NULL)
    g_object_unref (shortcut->volume);

  if (volume != NULL)
    shortcut->volume = g_object_ref (volume);
  else
    shortcut->volume = NULL;

  thunar_shortcut_volume_changed (shortcut);

  g_object_notify (G_OBJECT (shortcut), "volume");
}



GMount *
thunar_shortcut_get_mount (ThunarShortcut *shortcut)
{
  _thunar_return_val_if_fail (THUNAR_IS_SHORTCUT (shortcut), NULL);
  return shortcut->mount;
}



void
thunar_shortcut_set_mount (ThunarShortcut *shortcut,
                           GMount         *mount)
{
  _thunar_return_if_fail (THUNAR_IS_SHORTCUT (shortcut));
  _thunar_return_if_fail (mount == NULL || G_IS_MOUNT (mount));

  if (mount == NULL && !shortcut->constructed)
    return;

  if (shortcut->mount == mount)
    return;

  if (shortcut->mount != NULL)
    g_object_unref (shortcut->mount);

  if (mount != NULL)
    shortcut->mount = g_object_ref (mount);
  else
    shortcut->mount = NULL;

  thunar_shortcut_mount_changed (shortcut);

  g_object_notify (G_OBJECT (shortcut), "mount");
}



GIcon *
thunar_shortcut_get_icon (ThunarShortcut *shortcut)
{
  _thunar_return_val_if_fail (THUNAR_IS_SHORTCUT (shortcut), NULL);
  return shortcut->icon;
}



void
thunar_shortcut_set_icon (ThunarShortcut *shortcut,
                          GIcon          *icon)
{
  _thunar_return_if_fail (THUNAR_IS_SHORTCUT (shortcut));
  _thunar_return_if_fail (icon == NULL || G_IS_ICON (icon));

  if (icon == NULL && !shortcut->constructed)
    return;

  if (shortcut->icon == icon)
    return;

  if (shortcut->icon != NULL)
    g_object_unref (shortcut->icon);

  if (icon != NULL)
    shortcut->icon = g_object_ref (icon);
  else
    shortcut->icon = NULL;

  thunar_shortcut_icon_changed (shortcut);

  g_object_notify (G_OBJECT (shortcut), "icon");
}



GIcon *
thunar_shortcut_get_custom_icon (ThunarShortcut *shortcut)
{
  _thunar_return_val_if_fail (THUNAR_IS_SHORTCUT (shortcut), NULL);
  return shortcut->custom_icon;
}



void
thunar_shortcut_set_custom_icon (ThunarShortcut *shortcut,
                                 GIcon          *custom_icon)
{
  _thunar_return_if_fail (THUNAR_IS_SHORTCUT (shortcut));
  _thunar_return_if_fail (custom_icon == NULL || G_IS_ICON (custom_icon));

  if (custom_icon == NULL && !shortcut->constructed)
    return;

  if (shortcut->custom_icon == custom_icon)
    return;

  if (shortcut->custom_icon != NULL)
    g_object_unref (shortcut->custom_icon);

  if (custom_icon != NULL)
    shortcut->custom_icon = g_object_ref (custom_icon);
  else
    shortcut->custom_icon = NULL;

  thunar_shortcut_icon_changed (shortcut);

  g_object_notify (G_OBJECT (shortcut), "custom-icon");
}



GIcon *
thunar_shortcut_get_eject_icon (ThunarShortcut *shortcut)
{
  _thunar_return_val_if_fail (THUNAR_IS_SHORTCUT (shortcut), NULL);
  return shortcut->eject_icon;
}



void
thunar_shortcut_set_eject_icon (ThunarShortcut *shortcut,
                                GIcon          *eject_icon)
{
  _thunar_return_if_fail (THUNAR_IS_SHORTCUT (shortcut));
  _thunar_return_if_fail (eject_icon == NULL || G_IS_ICON (eject_icon));

  if (eject_icon == NULL && !shortcut->constructed)
    return;

  if (shortcut->eject_icon == eject_icon)
    return;

  if (shortcut->eject_icon != NULL)
    g_object_unref (shortcut->eject_icon);

  if (eject_icon != NULL)
    shortcut->eject_icon = g_object_ref (eject_icon);
  else
    shortcut->eject_icon = NULL;

  g_object_notify (G_OBJECT (shortcut), "eject-icon");
}



const gchar *
thunar_shortcut_get_name (ThunarShortcut *shortcut)
{
  _thunar_return_val_if_fail (THUNAR_IS_SHORTCUT (shortcut), NULL);
  return shortcut->name;
}



void
thunar_shortcut_set_name (ThunarShortcut *shortcut,
                          const gchar    *name)
{
  _thunar_return_if_fail (THUNAR_IS_SHORTCUT (shortcut));

  if (name == NULL && !shortcut->constructed)
    return;

  if (g_strcmp0 (shortcut->name, name) == 0)
    return;

  g_free (shortcut->name);

  shortcut->name = g_strdup (name);

  thunar_shortcut_name_changed (shortcut);

  g_object_notify (G_OBJECT (shortcut), "name");
}



const gchar *
thunar_shortcut_get_custom_name (ThunarShortcut *shortcut)
{
  _thunar_return_val_if_fail (THUNAR_IS_SHORTCUT (shortcut), NULL);
  return shortcut->custom_name;
}



void
thunar_shortcut_set_custom_name (ThunarShortcut *shortcut,
                                 const gchar    *custom_name)
{
  _thunar_return_if_fail (THUNAR_IS_SHORTCUT (shortcut));

  if (custom_name == NULL && !shortcut->constructed)
    return;

  if (g_strcmp0 (shortcut->custom_name, custom_name) == 0)
    return;

  g_free (shortcut->custom_name);

  shortcut->custom_name = g_strdup (custom_name);

  thunar_shortcut_name_changed (shortcut);

  g_object_notify (G_OBJECT (shortcut), "custom-name");
}



ThunarShortcutType
thunar_shortcut_get_shortcut_type (ThunarShortcut *shortcut)
{
  _thunar_return_val_if_fail (THUNAR_IS_SHORTCUT (shortcut), 0);
  return shortcut->type;
}



void
thunar_shortcut_set_shortcut_type (ThunarShortcut    *shortcut,
                                   ThunarShortcutType shortcut_type)
{
  _thunar_return_if_fail (THUNAR_IS_SHORTCUT (shortcut));

  if (shortcut->type == shortcut_type)
    return;

  shortcut->type = shortcut_type;

  thunar_shortcut_shortcut_type_changed (shortcut);

  g_object_notify (G_OBJECT (shortcut), "shortcut-type");
}



ThunarIconSize
thunar_shortcut_get_icon_size (ThunarShortcut *shortcut)
{
  _thunar_return_val_if_fail (THUNAR_IS_SHORTCUT (shortcut), 0);
  return shortcut->icon_size;
}



void
thunar_shortcut_set_icon_size (ThunarShortcut    *shortcut,
                               ThunarIconSize icon_size)
{
  _thunar_return_if_fail (THUNAR_IS_SHORTCUT (shortcut));

  if (shortcut->icon_size == icon_size)
    return;

  shortcut->icon_size = icon_size;

  thunar_shortcut_icon_size_changed (shortcut);

  g_object_notify (G_OBJECT (shortcut), "icon-size");
}



gboolean
thunar_shortcut_get_hidden (ThunarShortcut *shortcut)
{
  _thunar_return_val_if_fail (THUNAR_IS_SHORTCUT (shortcut), FALSE);
  return shortcut->hidden;
}



void
thunar_shortcut_set_hidden (ThunarShortcut *shortcut,
                            gboolean        hidden)
{
  _thunar_return_if_fail (THUNAR_IS_SHORTCUT (shortcut));

  if (shortcut->hidden == hidden)
    return;

  shortcut->hidden = hidden;

  thunar_shortcut_hidden_changed (shortcut);

  g_object_notify (G_OBJECT (shortcut), "hidden");
}



gboolean
thunar_shortcut_get_mutable (ThunarShortcut *shortcut)
{
  _thunar_return_val_if_fail (THUNAR_IS_SHORTCUT (shortcut), FALSE);
  return shortcut->mutable;
}



void
thunar_shortcut_set_mutable (ThunarShortcut *shortcut,
                             gboolean        mutable)
{
  _thunar_return_if_fail (THUNAR_IS_SHORTCUT (shortcut));

  if (shortcut->mutable == mutable)
    return;

  shortcut->mutable = mutable;

  g_object_notify (G_OBJECT (shortcut), "mutable");
}



gboolean
thunar_shortcut_get_persistent (ThunarShortcut *shortcut)
{
  _thunar_return_val_if_fail (THUNAR_IS_SHORTCUT (shortcut), FALSE);
  return shortcut->persistent;
}



void
thunar_shortcut_set_persistent (ThunarShortcut *shortcut,
                                gboolean        persistent)
{
  _thunar_return_if_fail (THUNAR_IS_SHORTCUT (shortcut));

  if (shortcut->persistent == persistent)
    return;

  shortcut->persistent = persistent;

  g_object_notify (G_OBJECT (shortcut), "persistent");
}



void
thunar_shortcut_resolve_and_activate (ThunarShortcut *shortcut,
                                      gboolean        open_in_new_window)
{
  _thunar_return_if_fail (THUNAR_IS_SHORTCUT (shortcut));

  /* check if we are currently mounting/ejecting something */
  if (shortcut->state != THUNAR_SHORTCUT_STATE_NORMAL)
    {
      /* abort the current mount/eject process */
      g_cancellable_cancel (shortcut->cancellable);
      g_cancellable_reset (shortcut->cancellable);
    }

  if (shortcut->volume != NULL)
    {
      /* activate the spinner */
      thunar_shortcut_set_spinning (shortcut, TRUE, THUNAR_SHORTCUT_STATE_RESOLVING);

      thunar_browser_poke_volume (THUNAR_BROWSER (shortcut), shortcut->volume,
                                  shortcut, thunar_shortcut_poke_volume_finish,
                                  GUINT_TO_POINTER (open_in_new_window));
    }
  else if (shortcut->file != NULL)
    {
      /* activate the spinner */
      thunar_shortcut_set_spinning (shortcut, TRUE, THUNAR_SHORTCUT_STATE_RESOLVING);

      thunar_browser_poke_file (THUNAR_BROWSER (shortcut), shortcut->file,
                                shortcut, thunar_shortcut_poke_file_finish,
                                GUINT_TO_POINTER (open_in_new_window));
    }
  else if (shortcut->location != NULL)
    {
      /* activate the spinner */
      thunar_shortcut_set_spinning (shortcut, TRUE, THUNAR_SHORTCUT_STATE_RESOLVING);

      thunar_browser_poke_location (THUNAR_BROWSER (shortcut), shortcut->location,
                                    shortcut, thunar_shortcut_poke_location_finish,
                                    GUINT_TO_POINTER (open_in_new_window));
    }
  else
    {
      _thunar_assert_not_reached ();
    }
}



void
thunar_shortcut_disconnect (ThunarShortcut *shortcut)
{
  GMountOperation *mount_operation;
  GtkWidget       *toplevel;
  GMount          *mount;

  _thunar_return_if_fail (THUNAR_IS_SHORTCUT (shortcut));

  g_debug ("disconnect shortcut");

  /* check if we are currently mounting/ejecting something */
  if (shortcut->state != THUNAR_SHORTCUT_STATE_NORMAL)
    {
      /* abort the current mount/eject process */
      g_cancellable_cancel (shortcut->cancellable);
      g_cancellable_reset (shortcut->cancellable);
    }

  /* create a mount operation */
  toplevel = gtk_widget_get_toplevel (GTK_WIDGET (shortcut));
  mount_operation = gtk_mount_operation_new (GTK_WINDOW (toplevel));
  gtk_mount_operation_set_screen (GTK_MOUNT_OPERATION (mount_operation),
                                  gtk_widget_get_screen (GTK_WIDGET (shortcut)));

  if (shortcut->mount != NULL)
    {
      g_debug ("  have mount");

      /* distinguish between ejectable and unmountable mounts */
      if (g_mount_can_eject (shortcut->mount))
        {
          g_debug ("  can eject");

          /* start spinning */
          thunar_shortcut_set_spinning (shortcut, TRUE, THUNAR_SHORTCUT_STATE_EJECTING);

          g_debug ("  eject now");

          /* try ejecting the mount */
          g_mount_eject_with_operation (shortcut->mount,
                                        G_MOUNT_UNMOUNT_NONE,
                                        mount_operation,
                                        shortcut->cancellable,
                                        thunar_shortcut_mount_eject_finish,
                                        shortcut);
        }
      else if (g_mount_can_unmount (shortcut->mount))
        {
          /* start spinning */
          thunar_shortcut_set_spinning (shortcut, TRUE, THUNAR_SHORTCUT_STATE_EJECTING);

          /* try unmounting the mount */
          g_mount_unmount_with_operation (shortcut->mount,
                                          G_MOUNT_UNMOUNT_NONE,
                                          mount_operation,
                                          shortcut->cancellable,
                                          thunar_shortcut_mount_unmount_finish,
                                          shortcut);
        }
    }
  else if (shortcut->volume != NULL)
    {
      if (g_volume_can_eject (shortcut->volume))
        {
          /* start spinning */
          thunar_shortcut_set_spinning (shortcut, TRUE, THUNAR_SHORTCUT_STATE_EJECTING);

          /* try ejecting the volume */
          g_volume_eject_with_operation (shortcut->volume,
                                         G_MOUNT_UNMOUNT_NONE,
                                         mount_operation,
                                         shortcut->cancellable,
                                         thunar_shortcut_volume_eject_finish,
                                         shortcut);
        }
      else
        {
          mount = g_volume_get_mount (shortcut->volume);
          if (mount != NULL)
            {
              /* distinguish between ejectable and unmountable mounts */
              if (g_mount_can_eject (mount))
                {
                  /* start spinning */
                  thunar_shortcut_set_spinning (shortcut, TRUE, 
                                                THUNAR_SHORTCUT_STATE_EJECTING);

                  /* try unmounting the mount */
                  g_mount_unmount_with_operation (mount,
                                                  G_MOUNT_UNMOUNT_NONE,
                                                  mount_operation,
                                                  shortcut->cancellable,
                                                  thunar_shortcut_mount_unmount_finish,
                                                  shortcut);
                }
              else if (g_mount_can_unmount (mount))
                {
                  /* start spinning */
                  thunar_shortcut_set_spinning (shortcut, TRUE, 
                                                THUNAR_SHORTCUT_STATE_EJECTING);

                  /* try unmounting the mount */
                  g_mount_unmount_with_operation (mount,
                                                  G_MOUNT_UNMOUNT_NONE,
                                                  mount_operation,
                                                  shortcut->cancellable,
                                                  thunar_shortcut_mount_unmount_finish,
                                                  shortcut);
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
      _thunar_assert_not_reached ();
    }

  /* release the mount operation */
  g_object_unref (mount_operation);
}
