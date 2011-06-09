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

#ifndef __THUNAR_SHORTCUT_ROW_H__
#define __THUNAR_SHORTCUT_ROW_H__

#include <glib-object.h>

#include <thunar/thunar-enum-types.h>

G_BEGIN_DECLS

#define THUNAR_TYPE_SHORTCUT_ROW            (thunar_shortcut_row_get_type ())
#define THUNAR_SHORTCUT_ROW(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), THUNAR_TYPE_SHORTCUT_ROW, ThunarShortcutRow))
#define THUNAR_SHORTCUT_ROW_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), THUNAR_TYPE_SHORTCUT_ROW, ThunarShortcutRowClass))
#define THUNAR_IS_SHORTCUT_ROW(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), THUNAR_TYPE_SHORTCUT_ROW))
#define THUNAR_IS_SHORTCUT_ROW_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), THUNAR_TYPE_SHORTCUT_ROW)
#define THUNAR_SHORTCUT_ROW_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), THUNAR_TYPE_SHORTCUT_ROW, ThunarShortcutRowClass))

typedef struct _ThunarShortcutRowClass ThunarShortcutRowClass;
typedef struct _ThunarShortcutRow      ThunarShortcutRow;

GType              thunar_shortcut_row_get_type             (void) G_GNUC_CONST;

void               thunar_shortcut_row_set_icon             (ThunarShortcutRow *row,
                                                             GIcon             *icon);
void               thunar_shortcut_row_set_eject_icon       (ThunarShortcutRow *row,
                                                             GIcon             *eject_icon);
void               thunar_shortcut_row_set_label            (ThunarShortcutRow *row,
                                                             const gchar       *label);
GFile             *thunar_shortcut_row_get_location         (ThunarShortcutRow *row);
void               thunar_shortcut_row_set_location         (ThunarShortcutRow *row,
                                                             GFile             *location);
ThunarFile        *thunar_shortcut_row_get_file             (ThunarShortcutRow *row);
void               thunar_shortcut_row_set_file             (ThunarShortcutRow *row,
                                                             ThunarFile        *file);
GVolume           *thunar_shortcut_row_get_volume           (ThunarShortcutRow *row);
void               thunar_shortcut_row_set_volume           (ThunarShortcutRow *row,
                                                             GVolume           *volume);
GMount            *thunar_shortcut_row_get_mount            (ThunarShortcutRow *row);
void               thunar_shortcut_row_set_mount            (ThunarShortcutRow *row,
                                                             GMount            *mount);
ThunarShortcutType thunar_shortcut_row_get_shortcut_type    (ThunarShortcutRow *row);
void               thunar_shortcut_row_set_shortcut_type    (ThunarShortcutRow *row,
                                                             ThunarShortcutType shortcut_type);
void               thunar_shortcut_row_set_icon_size        (ThunarShortcutRow *row,
                                                             ThunarIconSize     icon_size);
void               thunar_shortcut_row_resolve_and_activate (ThunarShortcutRow *row,
                                                             gboolean           open_in_new_window);

G_END_DECLS

#endif /* !__THUNAR_SHORTCUT_ROW_H__ */
