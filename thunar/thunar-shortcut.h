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

#ifndef __THUNAR_SHORTCUT_H__
#define __THUNAR_SHORTCUT_H__

#include <gio/gio.h>

#include <thunar/thunar-file.h>

G_BEGIN_DECLS

#define THUNAR_TYPE_SHORTCUT            (thunar_shortcut_get_type ())
#define THUNAR_SHORTCUT(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), THUNAR_TYPE_SHORTCUT, ThunarShortcut))
#define THUNAR_SHORTCUT_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), THUNAR_TYPE_SHORTCUT, ThunarShortcutClass))
#define THUNAR_IS_SHORTCUT(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), THUNAR_TYPE_SHORTCUT))
#define THUNAR_IS_SHORTCUT_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), THUNAR_TYPE_SHORTCUT)
#define THUNAR_SHORTCUT_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), THUNAR_TYPE_SHORTCUT, ThunarShortcutClass))

typedef struct _ThunarShortcutClass ThunarShortcutClass;
typedef struct _ThunarShortcut      ThunarShortcut;

GType              thunar_shortcut_get_type             (void) G_GNUC_CONST;

GFile             *thunar_shortcut_get_location         (ThunarShortcut    *shortcut);
void               thunar_shortcut_set_location         (ThunarShortcut    *shortcut,
                                                         GFile             *location);
ThunarFile        *thunar_shortcut_get_file             (ThunarShortcut    *shortcut);
void               thunar_shortcut_set_file             (ThunarShortcut    *shortcut,
                                                         ThunarFile        *file);
GVolume           *thunar_shortcut_get_volume           (ThunarShortcut    *shortcut);
void               thunar_shortcut_set_volume           (ThunarShortcut    *shortcut,
                                                         GVolume           *volume);
GMount            *thunar_shortcut_get_mount            (ThunarShortcut    *shortcut);
void               thunar_shortcut_set_mount            (ThunarShortcut    *shortcut,
                                                         GMount            *mount);
GIcon             *thunar_shortcut_get_icon             (ThunarShortcut    *shortcut);
void               thunar_shortcut_set_icon             (ThunarShortcut    *shortcut,
                                                         GIcon             *icon);
GIcon             *thunar_shortcut_get_custom_icon      (ThunarShortcut    *shortcut);
void               thunar_shortcut_set_custom_icon      (ThunarShortcut    *shortcut,
                                                         GIcon             *custom_icon);
GIcon             *thunar_shortcut_get_eject_icon       (ThunarShortcut    *shortcut);
void               thunar_shortcut_set_eject_icon       (ThunarShortcut    *shortcut,
                                                         GIcon             *eject_icon);
const gchar       *thunar_shortcut_get_name             (ThunarShortcut    *shortcut);
void               thunar_shortcut_set_name             (ThunarShortcut    *shortcut,
                                                         const gchar       *name);
const gchar       *thunar_shortcut_get_custom_name      (ThunarShortcut    *shortcut);
void               thunar_shortcut_set_custom_name      (ThunarShortcut    *shortcut,
                                                         const gchar       *custom_name);
ThunarShortcutType thunar_shortcut_get_shortcut_type    (ThunarShortcut    *shortcut);
void               thunar_shortcut_set_shortcut_type    (ThunarShortcut    *shortcut,
                                                         ThunarShortcutType shortcut_type);
ThunarIconSize     thunar_shortcut_get_icon_size        (ThunarShortcut    *shortcut);
void               thunar_shortcut_set_icon_size        (ThunarShortcut    *shortcut,
                                                         ThunarIconSize     icon_size);
gboolean           thunar_shortcut_get_hidden           (ThunarShortcut    *shortcut);
void               thunar_shortcut_set_hidden           (ThunarShortcut    *shortcut,
                                                         gboolean           hidden);
gboolean           thunar_shortcut_get_mutable          (ThunarShortcut    *shortcut);
void               thunar_shortcut_set_mutable          (ThunarShortcut    *shortcut,
                                                         gboolean           mutable);
gboolean           thunar_shortcut_get_persistent       (ThunarShortcut    *shortcut);
void               thunar_shortcut_set_persistent       (ThunarShortcut    *shortcut,
                                                         gboolean           persistent);
void               thunar_shortcut_resolve_and_activate (ThunarShortcut    *shortcut,
                                                         gboolean           open_in_new_window);
void               thunar_shortcut_disconnect           (ThunarShortcut    *shortcut);
gboolean           thunar_shortcut_matches_file         (ThunarShortcut    *shortcut,
                                                         ThunarFile        *file);

G_END_DECLS

#endif /* !__THUNAR_SHORTCUT_H__ */
