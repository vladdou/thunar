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

#ifndef __THUNARX_DESKTOP_VIEW_PROVIDER_H__
#define __THUNARX_DESKTOP_VIEW_PROVIDER_H__

#include <thunarx/thunarx-desktop-view.h>

G_BEGIN_DECLS

typedef struct _ThunarxDesktopViewProviderIface   ThunarxDesktopViewProviderIface;
typedef struct _ThunarxDesktopViewProvider        ThunarxDesktopViewProvider;

#define THUNARX_TYPE_DESKTOP_VIEW_PROVIDER           (thunarx_desktop_view_provider_get_type ())
#define THUNARX_DESKTOP_VIEW_PROVIDER(obj)           (G_TYPE_CHECK_INSTANCE_CAST ((obj), THUNARX_TYPE_DESKTOP_VIEW_PROVIDER, ThunarxDesktopViewProvider))
#define THUNARX_IS_DESKTOP_VIEW_PROVIDER(obj)        (G_TYPE_CHECK_INSTANCE_TYPE ((obj), THUNARX_TYPE_DESKTOP_VIEW_PROVIDER))
#define THUNARX_DESKTOP_VIEW_PROVIDER_GET_IFACE(obj) (G_TYPE_INSTANCE_GET_INTERFACE ((obj), THUNARX_TYPE_DESKTOP_VIEW_PROVIDER, ThunarxDesktopViewProviderIface))

struct _ThunarxDesktopViewProviderIface
{
  /*< private >*/
  GTypeInterface __parent__;

  /*< public >*/
  GList              *(*get_view_infos) (ThunarxDesktopViewProvider *provider);

  /* TODO add a ThunarxFolder parameter */
  ThunarxDesktopView *(*get_view)       (ThunarxDesktopViewProvider *provider,
                                         gpointer                    info,
                                         GError                    **error);

  /*< private >*/
  void (*reserved1) (void);
  void (*reserved2) (void);
  void (*reserved3) (void);
  void (*reserved4) (void);
};

GType               thunarx_desktop_view_provider_get_type       (void) G_GNUC_CONST;

GList              *thunarx_desktop_view_provider_get_view_infos (ThunarxDesktopViewProvider *provider) G_GNUC_MALLOC G_GNUC_WARN_UNUSED_RESULT;

/* TODO add a ThunarxFolder parameter */
ThunarxDesktopView *thunarx_desktop_view_provider_get_view       (ThunarxDesktopViewProvider *provider,
                                                                  gpointer                    info,
                                                                  GError                    **error) G_GNUC_MALLOC G_GNUC_WARN_UNUSED_RESULT;

G_END_DECLS

#endif /* !__THUNARX_DESKTOP_VIEW_PROVIDER_H__ */
