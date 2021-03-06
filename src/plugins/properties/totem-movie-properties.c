/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* 
 * Copyright (C) 2007 Bastien Nocera <hadess@hadess.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301  USA.
 *
 * The Totem project hereby grant permission for non-gpl compatible GStreamer
 * plugins to be used and distributed together with GStreamer and Totem. This
 * permission are above and beyond the permissions granted by the GPL license
 * Totem is covered by.
 *
 * See license_change file for details.
 *
 */

#include "config.h"

#include <glib.h>
#include <glib-object.h>
#include <glib/gi18n-lib.h>
#include <gmodule.h>
#include <string.h>
#include <libpeas/peas-extension-base.h>
#include <libpeas/peas-object-module.h>
#include <libpeas/peas-activatable.h>
#include <bacon-video-widget-properties.h>

#include "totem-plugin.h"
#include "totem.h"

#define TOTEM_TYPE_MOVIE_PROPERTIES_PLUGIN		(totem_movie_properties_plugin_get_type ())
#define TOTEM_MOVIE_PROPERTIES_PLUGIN(o)		(G_TYPE_CHECK_INSTANCE_CAST ((o), TOTEM_TYPE_MOVIE_PROPERTIES_PLUGIN, TotemMoviePropertiesPlugin))
#define TOTEM_MOVIE_PROPERTIES_PLUGIN_CLASS(k)	(G_TYPE_CHECK_CLASS_CAST((k), TOTEM_TYPE_MOVIE_PROPERTIES_PLUGIN, TotemMoviePropertiesPluginClass))
#define TOTEM_IS_MOVIE_PROPERTIES_PLUGIN(o)		(G_TYPE_CHECK_INSTANCE_TYPE ((o), TOTEM_TYPE_MOVIE_PROPERTIES_PLUGIN))
#define TOTEM_IS_MOVIE_PROPERTIES_PLUGIN_CLASS(k)	(G_TYPE_CHECK_CLASS_TYPE ((k), TOTEM_TYPE_MOVIE_PROPERTIES_PLUGIN))
#define TOTEM_MOVIE_PROPERTIES_PLUGIN_GET_CLASS(o)	(G_TYPE_INSTANCE_GET_CLASS ((o), TOTEM_TYPE_MOVIE_PROPERTIES_PLUGIN, TotemMoviePropertiesPluginClass))

typedef struct {
	GtkWidget    *props;
	guint         handler_id_stream_length;
} TotemMoviePropertiesPluginPrivate;

TOTEM_PLUGIN_REGISTER(TOTEM_TYPE_MOVIE_PROPERTIES_PLUGIN,
		      TotemMoviePropertiesPlugin,
		      totem_movie_properties_plugin)

static void
stream_length_notify_cb (TotemObject *totem,
			 GParamSpec *arg1,
			 TotemMoviePropertiesPlugin *plugin)
{
	gint64 stream_length;

	g_object_get (G_OBJECT (totem),
		      "stream-length", &stream_length,
		      NULL);

	bacon_video_widget_properties_from_time
		(BACON_VIDEO_WIDGET_PROPERTIES (plugin->priv->props),
		 stream_length);
}

static void
totem_movie_properties_plugin_file_opened (TotemObject *totem,
					   const char *mrl,
					   TotemMoviePropertiesPlugin *plugin)
{
	GtkWidget *bvw;

	bvw = totem_get_video_widget (totem);
	bacon_video_widget_properties_update
		(BACON_VIDEO_WIDGET_PROPERTIES (plugin->priv->props), bvw);
	g_object_unref (bvw);
	gtk_widget_set_sensitive (plugin->priv->props, TRUE);
}

static void
totem_movie_properties_plugin_file_closed (TotemObject *totem,
					   TotemMoviePropertiesPlugin *plugin)
{
        /* Reset the properties and wait for the signal*/
        bacon_video_widget_properties_reset
		(BACON_VIDEO_WIDGET_PROPERTIES (plugin->priv->props));
	gtk_widget_set_sensitive (plugin->priv->props, FALSE);
}

static void
totem_movie_properties_plugin_metadata_updated (TotemObject *totem,
						const char *artist, 
						const char *title, 
						const char *album,
						guint track_num,
						TotemMoviePropertiesPlugin *plugin)
{
	GtkWidget *bvw;

	bvw = totem_get_video_widget (totem);
	bacon_video_widget_properties_update
		(BACON_VIDEO_WIDGET_PROPERTIES (plugin->priv->props), bvw);
	g_object_unref (bvw);
}

static void
impl_activate (PeasActivatable *plugin)
{
	TotemMoviePropertiesPlugin *pi;
	TotemObject *totem;

	pi = TOTEM_MOVIE_PROPERTIES_PLUGIN (plugin);
	totem = g_object_get_data (G_OBJECT (plugin), "object");

	pi->priv->props = bacon_video_widget_properties_new ();
	gtk_widget_show (pi->priv->props);
	totem_add_sidebar_page (totem,
				"properties",
				_("Properties"),
				pi->priv->props);
	gtk_widget_set_sensitive (pi->priv->props, FALSE);

	g_signal_connect (G_OBJECT (totem),
			  "file-opened",
			  G_CALLBACK (totem_movie_properties_plugin_file_opened),
			  plugin);
	g_signal_connect (G_OBJECT (totem),
			  "file-closed",
			  G_CALLBACK (totem_movie_properties_plugin_file_closed),
			  plugin);
	g_signal_connect (G_OBJECT (totem),
			  "metadata-updated",
			  G_CALLBACK (totem_movie_properties_plugin_metadata_updated),
			  plugin);
	pi->priv->handler_id_stream_length = g_signal_connect (G_OBJECT (totem),
							 "notify::stream-length",
							 G_CALLBACK (stream_length_notify_cb),
							 plugin);
}

static void
impl_deactivate (PeasActivatable *plugin)
{
	TotemMoviePropertiesPlugin *pi;
	TotemObject *totem;

	pi = TOTEM_MOVIE_PROPERTIES_PLUGIN (plugin);
	totem = g_object_get_data (G_OBJECT (plugin), "object");

	g_signal_handler_disconnect (G_OBJECT (totem), pi->priv->handler_id_stream_length);
	g_signal_handlers_disconnect_by_func (G_OBJECT (totem),
					      totem_movie_properties_plugin_metadata_updated,
					      plugin);
	g_signal_handlers_disconnect_by_func (G_OBJECT (totem),
					      totem_movie_properties_plugin_file_opened,
					      plugin);
	g_signal_handlers_disconnect_by_func (G_OBJECT (totem),
					      totem_movie_properties_plugin_file_closed,
					      plugin);
	pi->priv->handler_id_stream_length = 0;
	totem_remove_sidebar_page (totem, "properties");
}

