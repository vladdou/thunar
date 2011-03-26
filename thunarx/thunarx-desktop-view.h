/* vi:set et ai sw=2 sts=2 ts=2: */
/*-
 * Copyright (c) 2011 Jannis Pohlmann <jannis@xfce.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the 
 * GNU Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General 
 * Public License along with this library; if not, write to the 
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#if !defined(THUNARX_INSIDE_THUNARX_H) && !defined(THUNARX_COMPILATION)
#error "Only <thunarx/thunarx.h> can be included directly, this file may disappear or change contents"
#endif

#ifndef __THUNARX_DESKTOP_VIEW_H__
#define __THUNARX_DESKTOP_VIEW_H__

#include <gtk/gtk.h>

G_BEGIN_DECLS

typedef struct _ThunarxDesktopViewPrivate ThunarxDesktopViewPrivate;
typedef struct _ThunarxDesktopViewClass   ThunarxDesktopViewClass;
typedef struct _ThunarxDesktopView        ThunarxDesktopView;

#define THUNARX_TYPE_DESKTOP_VIEW            (thunarx_desktop_view_get_type ())
#define THUNARX_DESKTOP_VIEW(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), THUNARX_TYPE_DESKTOP_VIEW, ThunarxDesktopView))
#define THUNARX_DESKTOP_VIEW_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), THUNARX_TYPE_DESKTOP_VIEW, ThunarxDesktopViewClass))
#define THUNARX_IS_DESKTOP_VIEW(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), THUNARX_TYPE_DESKTOP_VIEW))
#define THUNARX_IS_DESKTOP_VIEW_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), THUNARX_TYPE_DESKTOP_VIEW))
#define THUNARX_DESKTOP_VIEW_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), THUNARX_TYPE_DESKTOP_VIEW, ThunarxDesktopViewClass))

struct _ThunarxDesktopViewClass
{
  /*< private >*/
  GtkWindowClass __parent__;

  /*< public >*/

  /* virtual methods */

  /*< private >*/
  void (*reserved0) (void);
  void (*reserved1) (void);
  void (*reserved2) (void);
  void (*reserved3) (void);
  void (*reserved4) (void);

  /*< public >*/

  /* signals */

  /*< private >*/
  void (*reserved6) (void);
  void (*reserved7) (void);
  void (*reserved8) (void);
  void (*reserved9) (void);
};

struct _ThunarxDesktopView
{
  GtkWindow __parent__;

  /*< private >*/
  ThunarxDesktopViewPrivate *priv;
};

GType        thunarx_desktop_view_get_type (void) G_GNUC_CONST;

G_END_DECLS

#endif /* !__THUNARX_DESKTOP_VIEW_H__ */
