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

#include <thunar/thunar-enum-types.h>
#include <thunar/thunar-file.h>
#include <thunar/thunar-private.h>
#include <thunar/thunar-shortcut.h>



/* property identifiers */
enum
{
  PROP_0,
  PROP_LOCATION,
  PROP_FILE,
  PROP_VOLUME,
  PROP_MOUNT,
  PROP_ICON,
  PROP_EJECT_ICON,
  PROP_NAME,
  PROP_SHORTCUT_TYPE,
  PROP_HIDDEN,
  PROP_MUTABLE,
  PROP_PERSISTENT,
};

/* signal identifiers */
enum
{
  SIGNAL_CHANGED,
  LAST_SIGNAL,
};



static void thunar_shortcut_constructed  (GObject        *object);
static void thunar_shortcut_finalize     (GObject        *object);
static void thunar_shortcut_get_property (GObject        *object,
                                          guint           prop_id,
                                          GValue         *value,
                                          GParamSpec     *pspec);
static void thunar_shortcut_set_property (GObject        *object,
                                          guint           prop_id,
                                          const GValue   *value,
                                          GParamSpec     *pspec);
static void thunar_shortcut_load_file    (ThunarShortcut *shortcut);



struct _ThunarShortcutClass
{
  GObjectClass __parent__;
};

struct _ThunarShortcut
{
  GObject __parent__;

  GFile             *location;

  ThunarFile        *file;
  GVolume           *volume;
  GMount            *mount;
  
  GIcon             *icon;
  GIcon             *eject_icon;
  
  gchar             *name;
  
  ThunarShortcutType type;

  guint              hidden : 1;
  guint              mutable : 1;
  guint              persistent : 1;

};



G_DEFINE_TYPE (ThunarShortcut, thunar_shortcut, G_TYPE_OBJECT)



static guint shortcut_signals[LAST_SIGNAL];



static void
thunar_shortcut_class_init (ThunarShortcutClass *klass)
{
  GObjectClass *gobject_class;

  /* Determine the parent type class */
  thunar_shortcut_parent_class = g_type_class_peek_parent (klass);

  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->constructed = thunar_shortcut_constructed; 
  gobject_class->finalize = thunar_shortcut_finalize; 
  gobject_class->get_property = thunar_shortcut_get_property;
  gobject_class->set_property = thunar_shortcut_set_property;

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
                                   PROP_SHORTCUT_TYPE,
                                   g_param_spec_enum ("shortcut-type",
                                                      "shortcut-type",
                                                      "shortcut-type",
                                                      THUNAR_TYPE_SHORTCUT_TYPE,
                                                      THUNAR_SHORTCUT_REGULAR_FILE,
                                                      EXO_PARAM_READWRITE |
                                                      G_PARAM_CONSTRUCT));

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

  shortcut_signals[SIGNAL_CHANGED] = g_signal_new ("changed",
                                                   G_TYPE_FROM_CLASS (klass),
                                                   G_SIGNAL_RUN_LAST,
                                                   0, NULL, NULL, NULL,
                                                   G_TYPE_NONE, 0);
}



static void
thunar_shortcut_init (ThunarShortcut *shortcut)
{
}



static void
thunar_shortcut_constructed (GObject *object)
{
  ThunarShortcut *shortcut = THUNAR_SHORTCUT (object);

  if (shortcut->type == THUNAR_SHORTCUT_REGULAR_FILE
      || shortcut->type == THUNAR_SHORTCUT_NETWORK_FILE)
    {
      thunar_shortcut_load_file (shortcut);
    }
}



static void
thunar_shortcut_finalize (GObject *object)
{
  ThunarShortcut *shortcut = THUNAR_SHORTCUT (object);

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

  if (shortcut->eject_icon != NULL)
    g_object_unref (shortcut->eject_icon);

  g_free (shortcut->name);

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

    case PROP_EJECT_ICON:
      g_value_set_object (value, thunar_shortcut_get_eject_icon (shortcut));
      break;

    case PROP_NAME:
      g_value_set_string (value, thunar_shortcut_get_name (shortcut));
      break;

    case PROP_SHORTCUT_TYPE:
      g_value_set_enum (value, thunar_shortcut_get_shortcut_type (shortcut));
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

    case PROP_EJECT_ICON:
      thunar_shortcut_set_eject_icon (shortcut, g_value_get_object (value));
      break;

    case PROP_NAME:
      thunar_shortcut_set_name (shortcut, g_value_get_string (value));
      break;

    case PROP_SHORTCUT_TYPE:
      thunar_shortcut_set_shortcut_type (shortcut, g_value_get_enum (value));
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



#if 0
static void
thunar_shortcut_load_file_finish (GFile      *location,
                                  ThunarFile *file,
                                  GError     *error)
{
  if (error != NULL)
    {
      g_debug ("error: %s", error->message);
    }
  else
    {
      g_debug ("file loaded: %s", thunar_file_get_display_name (file));
    }
}
#endif



static void
thunar_shortcut_load_file (ThunarShortcut *shortcut)
{
  _thunar_return_if_fail (THUNAR_IS_SHORTCUT (shortcut));
  
  if (shortcut->file != NULL)
    return;

#if 0
  /* load the ThunarFile asynchronously */
  /* TODO pass a cancellable here */
  thunar_file_get_async (shortcut->location, NULL,
                         thunar_shortcut_load_file_finish);
#endif
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

  if (shortcut->location == location)
    return;

  if (shortcut->location != NULL)
    g_object_unref (shortcut->location);

  if (location != NULL)
    shortcut->location = g_object_ref (location);
  else
    shortcut->location = NULL;

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

  if (shortcut->file == file)
    return;

  if (shortcut->file != NULL)
    g_object_unref (shortcut->file);

  if (file != NULL)
    shortcut->file = g_object_ref (file);
  else
    shortcut->file = NULL;

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

  if (shortcut->volume == volume)
    return;

  if (shortcut->volume != NULL)
    g_object_unref (shortcut->volume);

  if (volume != NULL)
    shortcut->volume = g_object_ref (volume);
  else
    shortcut->volume = NULL;

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

  if (shortcut->mount == mount)
    return;

  if (shortcut->mount != NULL)
    g_object_unref (shortcut->mount);

  if (mount != NULL)
    shortcut->mount = g_object_ref (mount);
  else
    shortcut->mount = NULL;

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

  if (shortcut->icon == icon)
    return;

  if (shortcut->icon != NULL)
    g_object_unref (shortcut->icon);

  if (icon != NULL)
    shortcut->icon = g_object_ref (icon);
  else
    shortcut->icon = NULL;

  g_object_notify (G_OBJECT (shortcut), "icon");
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

  if (g_strcmp0 (shortcut->name, name) == 0)
    return;

  g_free (shortcut->name);

  shortcut->name = g_strdup (name);

  g_object_notify (G_OBJECT (shortcut), "name");
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

  g_object_notify (G_OBJECT (shortcut), "shortcut-type");
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
