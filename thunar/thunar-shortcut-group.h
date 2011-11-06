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

#ifndef __THUNAR_SHORTCUT_GROUP_H__
#define __THUNAR_SHORTCUT_GROUP_H__

#include <thunar/thunar-file.h>
#include <thunar/thunar-shortcut.h>

G_BEGIN_DECLS

#define THUNAR_TYPE_SHORTCUT_GROUP            (thunar_shortcut_group_get_type ())
#define THUNAR_SHORTCUT_GROUP(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), THUNAR_TYPE_SHORTCUT_GROUP, ThunarShortcutGroup))
#define THUNAR_SHORTCUT_GROUP_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), THUNAR_TYPE_SHORTCUT_GROUP, ThunarShortcutGroupClass))
#define THUNAR_IS_SHORTCUT_GROUP(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), THUNAR_TYPE_SHORTCUT_GROUP))
#define THUNAR_IS_SHORTCUT_GROUP_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), THUNAR_TYPE_SHORTCUT_GROUP)
#define THUNAR_SHORTCUT_GROUP_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), THUNAR_TYPE_SHORTCUT_GROUP, ThunarShortcutGroupClass))

typedef struct _ThunarShortcutGroupPrivate ThunarShortcutGroupPrivate;
typedef struct _ThunarShortcutGroupClass   ThunarShortcutGroupClass;
typedef struct _ThunarShortcutGroup        ThunarShortcutGroup;

GType      thunar_shortcut_group_get_type               (void) G_GNUC_CONST;

GtkWidget *thunar_shortcut_group_new                    (const gchar         *label,
                                                         ThunarShortcutType   accepted_types);
gboolean   thunar_shortcut_group_try_add_shortcut       (ThunarShortcutGroup *group,
                                                         ThunarShortcut      *shortcut);
void       thunar_shortcut_group_unselect_shortcuts     (ThunarShortcutGroup *group,
                                                         ThunarShortcut      *exception);
void       thunar_shortcut_group_unprelight_shortcuts   (ThunarShortcutGroup *group,
                                                         ThunarShortcut      *exception);
void       thunar_shortcut_group_update_selection       (ThunarShortcutGroup *group,
                                                         ThunarFile          *file);
void       thunar_shortcut_group_remove_volume_shortcut (ThunarShortcutGroup *group,
                                                         GVolume             *volume);
void       thunar_shortcut_group_remove_mount_shortcut  (ThunarShortcutGroup *group,
                                                         GMount              *mount);

G_END_DECLS

#endif /* !__THUNAR_SHORTCUT_GROUP_H__ */
