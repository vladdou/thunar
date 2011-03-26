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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <thunarx/thunarx-desktop-view-provider.h>
#include <thunarx/thunarx-desktop-view.h>
#include <thunarx/thunarx-private.h>



GType
thunarx_desktop_view_provider_get_type (void)
{
  static volatile gsize type__volatile = 0;
  GType                 type;
  
  if (g_once_init_enter (&type__volatile))
    {
      type = g_type_register_static_simple (G_TYPE_INTERFACE,
                                            I_("ThunarxDesktopViewProvider"),
                                            sizeof (ThunarxDesktopViewProviderIface),
                                            NULL,
                                            0,
                                            NULL,
                                            0);

      g_type_interface_add_prerequisite (type, G_TYPE_OBJECT);

      g_once_init_leave (&type__volatile, type);
    }

  return type__volatile;
}



/**
 * thunarx_desktop_view_provider_get_view_infos:
 * @provider : a #ThunarxDesktopViewProvider.
 * 
 * Returns a list of #ThunarxDesktopViewInfo<!---->s for the desktop views 
 * @provider is offering.
 *
 * The caller is responsible to free the returned list of infos
 * using something like this when no longer needed:
 * <informalexample><programlisting>
 * g_list_foreach (list, (GFunc) g_object_unref, NULL);
 * g_list_free (list);
 * </programlisting></informalexample>
 *
 * Return value: a list of #ThunarxDesktopViewInfo<!---->s for the desktop
 *               views that @provider is offering.
 **/
GList *
thunarx_desktop_view_provider_get_view_infos (ThunarxDesktopViewProvider *provider)
{
  GList *infos;

  g_return_val_if_fail (THUNARX_IS_DESKTOP_VIEW_PROVIDER (provider), NULL);
  
  if (THUNARX_DESKTOP_VIEW_PROVIDER_GET_IFACE (provider)->get_view_infos != NULL)
    {
      /* query the view infos from the implementation */
      infos = (*THUNARX_DESKTOP_VIEW_PROVIDER_GET_IFACE (provider)->get_view_infos) (provider);

      /* TODO does it make sense to take a reference on the provider here to
       * make sure the extension is kept loaded as long as we are using its
       * view infos somewhere? */
    }
  else
    {
      infos = NULL;
    }

  return infos;
}



/**
 * thunarx_desktop_view_provider_get_view:
 * @provider : a #ThunarxDesktopViewProvider.
 * @info     : a #ThunarxDesktopViewInfo.
 * @folder   : a #ThunarxFolder.
 * @error    : a return parameter to store error information in.
 * 
 * Returns a newly allocated #ThunarxDesktopView<!---->s for @info that
 * uses @folder as the desktop folder. Returns %NULL and fills @error
 * if the view cannot be created.
 *
 * The caller is responsible to free the returned view using something 
 * like this when no longer needed:
 * <informalexample><programlisting>
 * g_object_unref (view);
 * </programlisting></informalexample>
 *
 * Return value: a newly allocated #ThunarxDesktopView<!---->s for @info 
 *               that uses @folder as the desktop folder. Returns %NULL
 *               and fills @error in case the view cannot be created.
 **/
ThunarxDesktopView *
thunarx_desktop_view_provider_get_view (ThunarxDesktopViewProvider *provider,
                                        gpointer                    info,
                                        GError                    **error)
{
  ThunarxDesktopView *view;

  g_return_val_if_fail (THUNARX_IS_DESKTOP_VIEW_PROVIDER (provider), NULL);
  g_return_val_if_fail (error == NULL || *error == NULL, NULL);
  
  if (THUNARX_DESKTOP_VIEW_PROVIDER_GET_IFACE (provider)->get_view != NULL)
    {
      /* query the view infos from the implementation */
      view = (*THUNARX_DESKTOP_VIEW_PROVIDER_GET_IFACE (provider)->get_view) (provider, 
                                                                              info,
                                                                              error);

      /* TODO does it make sense to take a reference on the provider here to
       * make sure the extension is kept loaded as long as we are using its
       * views somewhere? */
    }
  else
    {
      view = NULL;

      /* TODO set the error */
    }

  return view;
}
