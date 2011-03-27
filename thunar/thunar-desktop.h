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

#ifndef __THUNAR_DESKTOP_H__
#define __THUNAR_DESKTOP_H__

#include <glib-object.h>

G_BEGIN_DECLS

#define THUNAR_TYPE_DESKTOP            (thunar_desktop_get_type ())
#define THUNAR_DESKTOP(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), THUNAR_TYPE_DESKTOP, ThunarDesktop))
#define THUNAR_DESKTOP_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), THUNAR_TYPE_DESKTOP, ThunarDesktopClass))
#define THUNAR_IS_DESKTOP(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), THUNAR_TYPE_DESKTOP))
#define THUNAR_IS_DESKTOP_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), THUNAR_TYPE_DESKTOP)
#define THUNAR_DESKTOP_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), THUNAR_TYPE_DESKTOP, ThunarDesktopClass))

typedef struct _ThunarDesktopClass   ThunarDesktopClass;
typedef struct _ThunarDesktop        ThunarDesktop;

GType thunar_desktop_get_type (void) G_GNUC_CONST;

G_END_DECLS

#endif /* !__THUNAR_DESKTOP_H__ */
