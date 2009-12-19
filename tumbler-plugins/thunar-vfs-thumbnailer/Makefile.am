# vi:set ts=8 sw=8 noet ai nocindent:
# -
# Copyright (c) 2009 Jannis Pohlmann <jannis@xfce.org>
# 
# This program is free software; you can redistribute it and/or 
# modify it under the terms of the GNU General Public License as
# published by the Free Software Foundation; either version 2 of 
# the License, or (at your option) any later version.
# 
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the 
# GNU General Public License for more details.
# 
# You should have received a copy of the GNU General Public 
# License along with this program; if not, write to the Free 
# Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
# Boston, MA 02110-1301, USA.

if HAVE_TUMBLER

tumbler_plugindir = $(libdir)/tumbler-1/plugins
tumbler_plugin_LTLIBRARIES =						\
	thunar-vfs-thumbnailer.la

thunar_vfs_thumbnailer_la_SOURCES =					\
	thunar-vfs-thumbnailer-plugin.c					\
	thunar-vfs-thumbnailer-provider.c				\
	thunar-vfs-thumbnailer-provider.h				\
	thunar-vfs-thumbnailer.c					\
	thunar-vfs-thumbnailer.h

thunar_vfs_thumbnailer_la_CFLAGS =					\
	-I$(top_builddir)						\
	-I$(top_builddir)/plugins					\
	-I$(top_srcdir)							\
	-I$(top_srcdir)/plugins						\
	-DG_LOG_DOMAIN=\"thunar-vfs-thumbnailer\"			\
	-DLIBEXECDIR=\"$(libexecdir)\"					\
	-DPACKAGE_LOCALE_DIR=\"$(localedir)\"				\
	$(GDK_PIXBUF_CFLAGS)						\
	$(GIO_CFLAGS)							\
	$(GLIB_CFLAGS)							\
	$(LIBXFCE4UTIL_CFLAGS)						\
	$(TUMBLER_CFLAGS)						\
	$(PLATFORM_CPPFLAGS)

thunar_vfs_thumbnailer_la_LDFLAGS =					\
	-avoid-version							\
	-export-dynamic							\
	-module								\
	$(PLATFORM_LDFLAGS)

thunar_vfs_thumbnailer_la_LIBADD =					\
	$(GDK_PIXBUF_LIBS)						\
	$(GIO_LIBS)							\
	$(GLIB_LIBS)							\
	$(LIBXFCE4UTIL_LIBS)						\
	$(TUMBLER_LIBS)

endif
