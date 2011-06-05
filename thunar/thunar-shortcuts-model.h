/* vi:set et ai sw=2 sts=2 ts=2: */
/*-
 * Copyright (c) 2005-2006 Benedikt Meurer <benny@xfce.org>
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

#ifndef __THUNAR_SHORTCUTS_MODEL_H__
#define __THUNAR_SHORTCUTS_MODEL_H__

#include <thunar/thunar-file.h>

G_BEGIN_DECLS

typedef struct _ThunarShortcutsModelClass ThunarShortcutsModelClass;
typedef struct _ThunarShortcutsModel      ThunarShortcutsModel;

#define THUNAR_TYPE_SHORTCUTS_MODEL            (thunar_shortcuts_model_get_type ())
#define THUNAR_SHORTCUTS_MODEL(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), THUNAR_TYPE_SHORTCUTS_MODEL, ThunarShortcutsModel))
#define THUNAR_SHORTCUTS_MODEL_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), THUNAR_TYPE_SHORTCUTS_MODEL, ThunarShortcutsModelClass))
#define THUNAR_IS_SHORTCUTS_MODEL(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), THUNAR_TYPE_SHORTCUTS_MODEL))
#define THUNAR_IS_SHORTCUTS_MODEL_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), THUNAR_TYPE_SHORTCUTS_MODEL))
#define THUNAR_SHORTCUTS_MODEL_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), THUNAR_TYPE_MODEL_SHORTCUTS_MODEL, ThunarShortcutsModelClass))

/**
 * ThunarShortcutsModelColumn:
 * @THUNAR_SHORTCUTS_MODEL_COLUMN_ICON       : file or volume icon.
 * @THUNAR_SHORTCUTS_MODEL_COLUMN_NAME       : file or volume display name.
 * @THUNAR_SHORTCUTS_MODEL_COLUMN_FILE       : the corresponding #GFile object.
 * @THUNAR_SHORTCUTS_MODEL_COLUMN_VOLUME     : the corresponding #GVolume object.
 * @THUNAR_SHORTCUTS_MODEL_COLUMN_MUTABLE    : tells whether a row is mutable.
 * @THUNAR_SHORTCUTS_MODEL_COLUMN_EJECT_ICON : stock icon name for eject symbol.
 * @THUNAR_SHORTCUTS_MODEL_COLUMN_CATEGORY   : tells whether the row is a category.
 * @THUNAR_SHORTCUTS_MODEL_COLUMN_PERSISTENT : tells whether the row is persistent.
 *
 * Columns exported by #ThunarShortcutsModel using the
 * #GtkTreeModel interface.
 **/
typedef enum
{
  THUNAR_SHORTCUTS_MODEL_COLUMN_ICON,
  THUNAR_SHORTCUTS_MODEL_COLUMN_NAME,
  THUNAR_SHORTCUTS_MODEL_COLUMN_FILE,
  THUNAR_SHORTCUTS_MODEL_COLUMN_VOLUME,
  THUNAR_SHORTCUTS_MODEL_COLUMN_MUTABLE,
  THUNAR_SHORTCUTS_MODEL_COLUMN_EJECT_ICON,
  THUNAR_SHORTCUTS_MODEL_COLUMN_CATEGORY,
  THUNAR_SHORTCUTS_MODEL_COLUMN_PERSISTENT,
  THUNAR_SHORTCUTS_MODEL_N_COLUMNS,
} ThunarShortcutsModelColumn;

GType                  thunar_shortcuts_model_get_type      (void) G_GNUC_CONST;

ThunarShortcutsModel *thunar_shortcuts_model_get_default    (void);

G_END_DECLS

#endif /* !__THUNAR_SHORTCUTS_MODEL_H__ */
