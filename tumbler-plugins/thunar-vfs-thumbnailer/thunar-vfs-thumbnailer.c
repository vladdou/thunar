/* vi:set et ai sw=2 sts=2 ts=2: */
/*-
 * Copyright (c) 2009 Jannis Pohlmann <jannis@xfce.org>
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

#ifdef HAVE_SYS_MMAN_H
#include <sys/mman.h>
#endif
#ifdef HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif

#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif
#ifdef HAVE_STDIO_H
#include <stdio.h>
#endif
#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <math.h>

#include <glib.h>
#include <glib-object.h>

#include <gio/gio.h>

#include <gdk-pixbuf/gdk-pixbuf.h>

#include <tumbler/tumbler.h>

#include <libxfce4util/libxfce4util.h>

#include <thunar-vfs-thumbnailer.h>



/* use g_open(), g_rename() and g_unlink() on win32 */
#if defined(G_OS_WIN32)
#include <glib/gstdio.h>
#else
#define g_open(filename, flags, mode) (open ((filename), (flags), (mode)))
#define g_rename(oldfilename, newfilename) (rename ((oldfilename), (newfilename)))
#define g_unlink(filename) (unlink ((filename)))
#endif



/* thumbnailers cache support */
#define CACHE_MAJOR_VERSION (1)
#define CACHE_MINOR_VERSION (0)
#define CACHE_READ32(cache, offset) (GUINT32_FROM_BE (*((guint32 *) ((cache) + (offset)))))

/* fallback cache if the loading fails */
static const guint32 CACHE_FALLBACK[4] =
{
#if G_BYTE_ORDER == G_BIG_ENDIAN
  CACHE_MAJOR_VERSION,
  CACHE_MINOR_VERSION,
  0,
  0,
#else
  GUINT32_SWAP_LE_BE_CONSTANT (CACHE_MAJOR_VERSION),
  GUINT32_SWAP_LE_BE_CONSTANT (CACHE_MINOR_VERSION),
  GUINT32_SWAP_LE_BE_CONSTANT (0),
  GUINT32_SWAP_LE_BE_CONSTANT (0),
#endif
};



static void     thunar_vfs_thumbnailer_finalize            (GObject                    *object);
static void     thunar_vfs_thumbnailer_cache_load          (ThunarVfsThumbnailer       *thumbnailer);
static void     thunar_vfs_thumbnailer_cache_unload        (ThunarVfsThumbnailer       *thumbnailer);
static void     thunar_vfs_thumbnailer_cache_monitor       (GFileMonitor               *monitor,
                                                            GFile                      *file,
                                                            GFile                      *other_file,
                                                            GFileMonitorEvent           event,
                                                            gpointer                    user_data);
static void     thunar_vfs_thumbnailer_cache_update        (ThunarVfsThumbnailer       *thumbnailer);
static gboolean thunar_vfs_thumbnailer_cache_timer         (gpointer                    user_data);
static void     thunar_vfs_thumbnailer_cache_timer_destroy (gpointer                    user_data);
static void     thunar_vfs_thumbnailer_cache_watch         (GPid                        pid,
                                                            gint                        status,
                                                            gpointer                    user_data);
static void     thunar_vfs_thumbnailer_cache_watch_destroy (gpointer                    user_data);
static void     thunar_vfs_thumbnailer_create              (TumblerAbstractThumbnailer *thumbnailer,
                                                            GCancellable               *cancellable,
                                                            TumblerFileInfo            *info);



struct _ThunarVfsThumbnailerClass
{
  TumblerAbstractThumbnailerClass __parent__;
};

struct _ThunarVfsThumbnailer
{
  TumblerAbstractThumbnailer __parent__;

  gchar        *cache;
  guint         cache_size;
  GMutex       *cache_lock;
  guint         cache_timer_id;
  guint         cache_watch_id;
  guint         cache_was_mapped : 1;

  GFileMonitor *cache_monitor;
  GFile        *cache_file;
};



G_DEFINE_DYNAMIC_TYPE (ThunarVfsThumbnailer, 
                       thunar_vfs_thumbnailer,
                       TUMBLER_TYPE_ABSTRACT_THUMBNAILER);



void
thunar_vfs_thumbnailer_register (TumblerProviderPlugin *plugin)
{
  thunar_vfs_thumbnailer_register_type (G_TYPE_MODULE (plugin));
}



static void
thunar_vfs_thumbnailer_class_init (ThunarVfsThumbnailerClass *klass)
{
  TumblerAbstractThumbnailerClass *abstractthumbnailer_class;
  GObjectClass                    *gobject_class;

  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->finalize = thunar_vfs_thumbnailer_finalize;

  abstractthumbnailer_class = TUMBLER_ABSTRACT_THUMBNAILER_CLASS (klass);
  abstractthumbnailer_class->create = thunar_vfs_thumbnailer_create;
}



static void
thunar_vfs_thumbnailer_class_finalize (ThunarVfsThumbnailerClass *klass)
{
}



static void
thunar_vfs_thumbnailer_init (ThunarVfsThumbnailer *thumbnailer)
{
  gchar *cache_path;

  /* allocate the cache mutex */
  thumbnailer->cache_lock = g_mutex_new ();

  /* determine the thumbnailers cache file */
  cache_path = xfce_resource_save_location (XFCE_RESOURCE_CACHE, 
                                            "Thunar/thumbnailers.cache", FALSE);
  thumbnailer->cache_file = g_file_new_for_path (cache_path);

  /* monitor the thumbnailers.cache file for changes */
  thumbnailer->cache_monitor = g_file_monitor (thumbnailer->cache_file, 
                                               G_FILE_MONITOR_NONE, NULL, NULL);
  g_signal_connect (thumbnailer->cache_monitor, "changed", 
                    G_CALLBACK (thunar_vfs_thumbnailer_cache_monitor), thumbnailer);

  /* initially load the thumbnailers.cache file */
  thunar_vfs_thumbnailer_cache_load (thumbnailer);

  /* schedule a timer to update the thumbnailers.cache every 5 minutes */
  thumbnailer->cache_timer_id = 
    g_timeout_add_full (G_PRIORITY_LOW, 5 * 60 * 1000, 
                        thunar_vfs_thumbnailer_cache_timer, thumbnailer, 
                        thunar_vfs_thumbnailer_cache_timer_destroy);

  g_free (cache_path);
}



static void
thunar_vfs_thumbnailer_finalize (GObject *object)
{
  ThunarVfsThumbnailer *thumbnailer = THUNAR_VFS_THUMBNAILER (object);

  /* be sure to unload the cache file */
  thunar_vfs_thumbnailer_cache_unload (thumbnailer);

  /* stop any pending cache sources */
  if (G_LIKELY (thumbnailer->cache_timer_id != 0))
    g_source_remove (thumbnailer->cache_timer_id);
  if (G_UNLIKELY (thumbnailer->cache_watch_id != 0))
    g_source_remove (thumbnailer->cache_watch_id);

  /* unregister from and release the monitor */
  g_signal_handlers_disconnect_matched (thumbnailer->cache_monitor, G_SIGNAL_MATCH_DATA,
                                        0, 0, NULL, NULL, thumbnailer);
  g_object_unref (thumbnailer->cache_monitor);

  /* release the cache lock */
  g_mutex_free (thumbnailer->cache_lock);

  g_object_unref (thumbnailer->cache_file);

  (*G_OBJECT_CLASS (thunar_vfs_thumbnailer_parent_class)->finalize) (object);
}



static void
thunar_vfs_thumbnailer_cache_load (ThunarVfsThumbnailer *thumbnailer)
{
  struct stat statb;
  gssize      m;
  gchar      *cache_path;
  gsize       n;
  gint        fd;

  g_return_if_fail (IS_THUNAR_VFS_THUMBNAILER (thumbnailer));
  g_return_if_fail (G_IS_FILE (thumbnailer->cache_file));
  g_return_if_fail (thumbnailer->cache == NULL);

  cache_path = g_file_get_path (thumbnailer->cache_file);

  /* try to open the thumbnailers.cache file */
  fd = g_open (cache_path, O_RDONLY, 0000);
  if (G_LIKELY (fd >= 0))
    {
      /* stat the file to get proper size info */
      if (fstat (fd, &statb) == 0 && statb.st_size >= 16)
        {
          /* remember the size of the cache */
          thumbnailer->cache_size = statb.st_size;

#ifdef HAVE_MMAP
          /* try to mmap() the file into memory */
          thumbnailer->cache = mmap (NULL, statb.st_size, PROT_READ, MAP_SHARED, fd, 0);
          if (G_LIKELY (thumbnailer->cache != MAP_FAILED))
            {
              /* remember that mmap() succeed */
              thumbnailer->cache_was_mapped = TRUE;

#ifdef HAVE_POSIX_MADVISE
              /* tell the system that we'll use this buffer quite often */
              posix_madvise (thumbnailer->cache, statb.st_size, POSIX_MADV_WILLNEED);
#endif
            }
          else
#endif
            {
              /* remember that mmap() failed */
              thumbnailer->cache_was_mapped = FALSE;

              /* allocate memory for the cache */
              thumbnailer->cache = g_malloc (statb.st_size);

              /* read the cache file */
              for (n = 0; n < statb.st_size; n += m)
                {
                  /* read the next chunk */
                  m = read (fd, thumbnailer->cache + n, statb.st_size - n);
                  if (G_UNLIKELY (m <= 0))
                    {
                      /* reset the cache */
                      g_free (thumbnailer->cache);
                      thumbnailer->cache = NULL;
                      break;
                    }
                }
            }
        }

      /* close the file handle */
      close (fd);
    }

  /* check if the cache was loaded successfully */
  if (G_LIKELY (thumbnailer->cache != NULL))
    {
      /* check that we actually support the file version */
      if (CACHE_READ32 (thumbnailer->cache, 0) != CACHE_MAJOR_VERSION 
          || CACHE_READ32 (thumbnailer->cache, 4) != CACHE_MINOR_VERSION)
        {
          thunar_vfs_thumbnailer_cache_unload (thumbnailer);
        }
    }
  else
    {
      /* run the thunar-update-thunar-vfs-thumbnailers-cache-1 utility */
      thunar_vfs_thumbnailer_cache_update (thumbnailer);
    }

  /* use the fallback cache if the loading failed */
  if (G_UNLIKELY (thumbnailer->cache == NULL))
    {
      thumbnailer->cache = (gchar *) CACHE_FALLBACK;
      thumbnailer->cache_size = 16;
    }

  g_free (cache_path);
}



static void
thunar_vfs_thumbnailer_cache_unload (ThunarVfsThumbnailer *thumbnailer)
{
  g_return_if_fail (IS_THUNAR_VFS_THUMBNAILER (thumbnailer));
  g_return_if_fail (thumbnailer->cache != NULL);

  /* check if any cache is loaded (and not using the fallback cache) */
  if (thumbnailer->cache != (gchar *) CACHE_FALLBACK)
    {
#ifdef HAVE_MMAP
      /* check if mmap() succeed */
      if (thumbnailer->cache_was_mapped)
        {
          /* just unmap the memory */
          munmap (thumbnailer->cache, thumbnailer->cache_size);
        }
      else
#endif
        {
          /* need to release the memory */
          g_free (thumbnailer->cache);
        }
    }

  /* reset the cache pointer */
  thumbnailer->cache = NULL;
}



static void
thunar_vfs_thumbnailer_cache_monitor (GFileMonitor      *monitor,
                                      GFile             *file,
                                      GFile             *other_file,
                                      GFileMonitorEvent  event,
                                      gpointer           user_data)
{
  ThunarVfsThumbnailer *thumbnailer = THUNAR_VFS_THUMBNAILER (user_data);

  g_return_if_fail (IS_THUNAR_VFS_THUMBNAILER (thumbnailer));
  g_return_if_fail (thumbnailer->cache_monitor == monitor);
  g_return_if_fail (G_IS_FILE_MONITOR (monitor));

  /* check the event */
  if (G_UNLIKELY (event == G_FILE_MONITOR_EVENT_DELETED))
    {
      /* some idiot deleted the cache file, regenerate it */
      thunar_vfs_thumbnailer_cache_update (thumbnailer);
    }
  else
    {
      /* the thumbnailers.cache file was changed, reload it */
      g_mutex_lock (thumbnailer->cache_lock);
      thunar_vfs_thumbnailer_cache_unload (thumbnailer);
      thunar_vfs_thumbnailer_cache_load (thumbnailer);
      g_mutex_unlock (thumbnailer->cache_lock);
    }
}



static void
thunar_vfs_thumbnailer_cache_update (ThunarVfsThumbnailer *thumbnailer)
{
  gchar *argv[2] = { LIBEXECDIR G_DIR_SEPARATOR_S "/thunar-update-thunar-vfs-thumbnailers-cache-1", NULL };
  GPid   pid;

  g_return_if_fail (IS_THUNAR_VFS_THUMBNAILER (thumbnailer));

  /* check if we're already updating the cache */
  if (G_LIKELY (thumbnailer->cache_watch_id == 0))
    {
      /* try to spawn the command */
      if (g_spawn_async (NULL, argv, NULL, G_SPAWN_DO_NOT_REAP_CHILD, NULL, NULL, &pid, NULL))
        {
          /* add a child watch for the updater process */
          thumbnailer->cache_watch_id = 
            g_child_watch_add_full (G_PRIORITY_LOW, pid, thunar_vfs_thumbnailer_cache_watch,
                                    thumbnailer, thunar_vfs_thumbnailer_cache_watch_destroy);

#ifdef HAVE_SETPRIORITY
          /* decrease the priority of the updater process */
          setpriority (PRIO_PROCESS, pid, 10);
#endif
        }
    }
}




static gboolean
thunar_vfs_thumbnailer_cache_timer (gpointer user_data)
{
  /* run the thunar-update-thunar-vfs-thumbnailers-cache-1 utility... */
  thunar_vfs_thumbnailer_cache_update (THUNAR_VFS_THUMBNAILER (user_data));

  /* ...and keep the timer running */
  return TRUE;
}



static void
thunar_vfs_thumbnailer_cache_timer_destroy (gpointer user_data)
{
  THUNAR_VFS_THUMBNAILER (user_data)->cache_timer_id = 0;
}



static void
thunar_vfs_thumbnailer_cache_watch (GPid     pid,
                                    gint     status,
                                    gpointer user_data)
{
  ThunarVfsThumbnailer *thumbnailer = THUNAR_VFS_THUMBNAILER (user_data);

  g_return_if_fail (IS_THUNAR_VFS_THUMBNAILER (thumbnailer));

  /* a return value of 33 means that the thumbnailers.cache was updated by the
   * thunar-update-thunar-vfs-thumbnailers-cache-1 utility and must be reloaded.
   */
  if (WIFEXITED (status) && WEXITSTATUS (status) == 33)
    {
      /* TODO schedule a "changed" event for the thumbnailers.cache */
      g_file_monitor_emit_event (thumbnailer->cache_monitor, thumbnailer->cache_file, 
                                 NULL, G_FILE_MONITOR_EVENT_CHANGED);
    }

  /* close the PID (win32) */
  g_spawn_close_pid (pid);
}



static void
thunar_vfs_thumbnailer_cache_watch_destroy (gpointer user_data)
{
  THUNAR_VFS_THUMBNAILER (user_data)->cache_watch_id = 0;
}



static GdkPixbuf *
generate_pixbuf (GdkPixbuf              *source,
                 TumblerThumbnailFlavor *flavor)
{
  gdouble    hratio;
  gdouble    wratio;
  gint       dest_width;
  gint       dest_height;
  gint       source_width;
  gint       source_height;

  /* determine the source pixbuf dimensions */
  source_width = gdk_pixbuf_get_width (source);
  source_height = gdk_pixbuf_get_height (source);

  /* determine the desired size for this flavor */
  tumbler_thumbnail_flavor_get_size (flavor, &dest_width, &dest_height);

  /* return the same pixbuf if no scaling is required */
  if (source_width <= dest_width && source_height <= dest_height)
    return g_object_ref (source);

  /* determine which axis needs to be scaled down more */
  wratio = (gdouble) source_width / (gdouble) dest_width;
  hratio = (gdouble) source_height / (gdouble) dest_height;

  /* adjust the other axis */
  if (hratio > wratio)
    dest_width = rint (source_width / hratio);
  else
    dest_height = rint (source_height / wratio);
  
  /* scale the pixbuf down to the desired size */
  return gdk_pixbuf_scale_simple (source, 
                                  MAX (dest_width, 1), MAX (dest_height, 1), 
                                  GDK_INTERP_BILINEAR);
}



static void
thunar_vfs_thumbnailer_create (TumblerAbstractThumbnailer *thumbnailer,
                               GCancellable               *cancellable,
                               TumblerFileInfo            *info)
{
  TumblerThumbnailFlavor *flavor;
  GFileInputStream       *stream;
  TumblerImageData        data;
  TumblerThumbnail       *thumbnail;
  const gchar            *uri;
  GdkPixbuf              *source_pixbuf;
  GdkPixbuf              *pixbuf;
  GError                 *error = NULL;
  GFile                  *file;

  g_return_if_fail (IS_THUNAR_VFS_THUMBNAILER (thumbnailer));
  g_return_if_fail (cancellable == NULL || G_IS_CANCELLABLE (cancellable));
  g_return_if_fail (TUMBLER_IS_FILE_INFO (info));

  /* do nothing if cancelled */
  if (g_cancellable_is_cancelled (cancellable)) 
    return;

  uri = tumbler_file_info_get_uri (info);

  /* try to open the source file for reading */
  file = g_file_new_for_uri (uri);
  stream = g_file_read (file, NULL, &error);
  g_object_unref (file);

  if (stream == NULL)
    {
      g_signal_emit_by_name (thumbnailer, "error", uri, error->code, error->message);
      g_error_free (error);
      return;
    }

  source_pixbuf = gdk_pixbuf_new_from_stream (G_INPUT_STREAM (stream), 
                                              cancellable, &error);
  g_object_unref (stream);

  if (source_pixbuf == NULL)
    {
      g_signal_emit_by_name (thumbnailer, "error", uri, error->code, error->message);
      g_error_free (error);
      return;
    }

  thumbnail = tumbler_file_info_get_thumbnail (info);

  g_assert (thumbnail != NULL);

  /* generate a pixbuf for the thumbnail */
  flavor = tumbler_thumbnail_get_flavor (thumbnail);
  pixbuf = generate_pixbuf (source_pixbuf, flavor);
  g_object_unref (flavor);

  g_assert (pixbuf != NULL);

  data.data = gdk_pixbuf_get_pixels (pixbuf);
  data.has_alpha = gdk_pixbuf_get_has_alpha (pixbuf);
  data.bits_per_sample = gdk_pixbuf_get_bits_per_sample (pixbuf);
  data.width = gdk_pixbuf_get_width (pixbuf);
  data.height = gdk_pixbuf_get_height (pixbuf);
  data.rowstride = gdk_pixbuf_get_rowstride (pixbuf);
  data.colorspace = (TumblerColorspace) gdk_pixbuf_get_colorspace (pixbuf);

  tumbler_thumbnail_save_image_data (thumbnail, &data, 
                                     tumbler_file_info_get_mtime (info), 
                                     NULL, &error);

  if (error != NULL)
    {
      g_signal_emit_by_name (thumbnailer, "error", uri, error->code, error->message);
      g_error_free (error);
    }
  else
    {
      g_signal_emit_by_name (thumbnailer, "ready", uri);
    }

  g_object_unref (thumbnail);
  g_object_unref (pixbuf);
  g_object_unref (source_pixbuf);
}
