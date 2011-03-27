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

#include <thunarx/thunarx.h>

#include <thunar/thunar-desktop.h>



/* property identifiers */
enum
{
  PROP_0,
};



static void thunar_desktop_constructed  (GObject      *object);
static void thunar_desktop_dispose      (GObject      *object);
static void thunar_desktop_finalize     (GObject      *object);
static void thunar_desktop_get_property (GObject      *object,
                                         guint         prop_id,
                                         GValue       *value,
                                         GParamSpec   *pspec);
static void thunar_desktop_set_property (GObject      *object,
                                         guint         prop_id,
                                         const GValue *value,
                                         GParamSpec   *pspec);



struct _ThunarDesktopClass
{
  GObjectClass __parent__;
};

struct _ThunarDesktop
{
  GObject __parent__;
};



G_DEFINE_TYPE (ThunarDesktop, thunar_desktop, G_TYPE_OBJECT)



static void
thunar_desktop_class_init (ThunarDesktopClass *klass)
{
  GObjectClass *gobject_class;

  /* Determine the parent type class */
  thunar_desktop_parent_class = g_type_class_peek_parent (klass);

  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->constructed = thunar_desktop_constructed; 
  gobject_class->dispose = thunar_desktop_dispose; 
  gobject_class->finalize = thunar_desktop_finalize; 
  gobject_class->get_property = thunar_desktop_get_property;
  gobject_class->set_property = thunar_desktop_set_property;
}



static void
thunar_desktop_init (ThunarDesktop *desktop)
{
  ThunarxProviderFactory *provider_factory;
  GList                  *lp;
  GList                  *providers;
  GList                  *view_infos = NULL;

  /* determine the list of available views (using the desktop view provider) */
  provider_factory = thunarx_provider_factory_get_default ();
  providers = thunarx_provider_factory_list_providers (provider_factory,
                                                       THUNARX_TYPE_DESKTOP_VIEW_PROVIDER);
  for (lp = providers; lp != NULL; lp = lp->next)
    {
      /* determine the view infos for this provider */
      view_infos = 
        g_list_concat (view_infos, 
                       thunarx_desktop_view_provider_get_view_infos (lp->data));

      /* release the provider */
      g_object_unref (lp->data);
    }
  g_list_free (providers);
  g_object_unref (provider_factory);

  g_debug ("view infos:");
  for (lp = view_infos; lp != NULL; lp = lp->next)
    {
      g_debug ("  %p", lp->data);
    }
}



static void
thunar_desktop_constructed (GObject *object)
{
#if 0
  ThunarDesktop *desktop = THUNAR_DESKTOP (object);
#endif
}



static void
thunar_desktop_dispose (GObject *object)
{
#if 0
  ThunarDesktop *desktop = THUNAR_DESKTOP (object);
#endif

  (*G_OBJECT_CLASS (thunar_desktop_parent_class)->dispose) (object);
}



static void
thunar_desktop_finalize (GObject *object)
{
#if 0
  ThunarDesktop *desktop = THUNAR_DESKTOP (object);
#endif

  (*G_OBJECT_CLASS (thunar_desktop_parent_class)->finalize) (object);
}



static void
thunar_desktop_get_property (GObject    *object,
                             guint       prop_id,
                             GValue     *value,
                             GParamSpec *pspec)
{
#if 0
  ThunarDesktop *desktop = THUNAR_DESKTOP (object);
#endif

  switch (prop_id)
    {
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}



static void
thunar_desktop_set_property (GObject      *object,
                             guint         prop_id,
                             const GValue *value,
                             GParamSpec   *pspec)
{
#if 0
  ThunarDesktop *desktop = THUNAR_DESKTOP (object);
#endif

  switch (prop_id)
    {
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}
