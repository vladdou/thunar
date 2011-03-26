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

#include <glib/gi18n-lib.h>

#include <thunarx/thunarx-desktop-view.h>
#include <thunarx/thunarx-private.h>



#define THUNARX_DESKTOP_VIEW_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), THUNARX_TYPE_DESKTOP_VIEW, ThunarxDesktopViewPrivate))



/* Property identifiers */
enum
{
  PROP_0,
};

/* Signal identifiers */
enum
{
  LAST_SIGNAL,
};



static void     thunarx_desktop_view_finalize     (GObject                *object);
static GObject *thunarx_desktop_view_constructor  (GType                   type,
                                                   guint                   n_construct_properties,
                                                   GObjectConstructParam  *construct_properties);
static void     thunarx_desktop_view_get_property (GObject                *object,
                                                   guint                   prop_id,
                                                   GValue                 *value,
                                                   GParamSpec             *pspec);
static void     thunarx_desktop_view_set_property (GObject                *object,
                                                   guint                   prop_id,
                                                   const GValue           *value,
                                                   GParamSpec             *pspec);



struct _ThunarxDesktopViewPrivate
{
  int dummy;
};



#if 0
static guint desktop_view_signals[LAST_SIGNAL];
#endif



G_DEFINE_ABSTRACT_TYPE (ThunarxDesktopView, thunarx_desktop_view, GTK_TYPE_WINDOW)



static void
thunarx_desktop_view_class_init (ThunarxDesktopViewClass *klass)
{
  GObjectClass *gobject_class;

  /* add private data */
  g_type_class_add_private (klass, sizeof (ThunarxDesktopViewPrivate));

  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->finalize = thunarx_desktop_view_finalize;
  gobject_class->constructor = thunarx_desktop_view_constructor;
  gobject_class->get_property = thunarx_desktop_view_get_property;
  gobject_class->set_property = thunarx_desktop_view_set_property;
}



static void
thunarx_desktop_view_init (ThunarxDesktopView *desktop_view)
{
  /* grab a pointer on the private data */
  desktop_view->priv = THUNARX_DESKTOP_VIEW_GET_PRIVATE (desktop_view);
}



static void
thunarx_desktop_view_finalize (GObject *object)
{
#if 0
  ThunarxDesktopView *desktop_view = THUNARX_DESKTOP_VIEW (object);
#endif

  (*G_OBJECT_CLASS (thunarx_desktop_view_parent_class)->finalize) (object);
}



static GObject*
thunarx_desktop_view_constructor (GType                  type,
                                  guint                  n_construct_properties,
                                  GObjectConstructParam *construct_properties)
{
  GObject *object;

  /* let the parent class constructor create the instance */
  object = (*G_OBJECT_CLASS (thunarx_desktop_view_parent_class)->constructor) (type, n_construct_properties, construct_properties);

  return object;
}



static void
thunarx_desktop_view_get_property (GObject    *object,
                                   guint       prop_id,
                                   GValue     *value,
                                   GParamSpec *pspec)
{
#if 0
  ThunarxDesktopView *desktop_view = THUNARX_DESKTOP_VIEW (object);
#endif

  switch (prop_id)
    {
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}



static void
thunarx_desktop_view_set_property (GObject      *object,
                                   guint         prop_id,
                                   const GValue *value,
                                   GParamSpec   *pspec)
{
#if 0
  ThunarxDesktopView *desktop_view = THUNARX_DESKTOP_VIEW (object);
#endif

  switch (prop_id)
    {
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}
