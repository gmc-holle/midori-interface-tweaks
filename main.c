/*
 Copyright (C) 2013 Stephan Haller <nomad@froevel.de>

 This library is free software; you can redistribute it and/or
 modify it under the terms of the GNU Lesser General Public
 License as published by the Free Software Foundation; either
 version 2.1 of the License, or (at your option) any later version.

 See the file COPYING for the full license text.
*/

#include "interface-tweaks.h"
#include "interface-tweaks-preferences.h"

/* Global instance */
InterfaceTweaks		*it=NULL;

/* This extension was activated */
static void _it_on_activate(MidoriExtension *inExtension, MidoriApp *inApp, gpointer inUserData)
{
	g_return_if_fail(it==NULL);

	it=interface_tweaks_new(inExtension, inApp);
	// TODO: g_object_set(it, "ask-for-unknown-policy", midori_extension_get_boolean(inExtension, "ask-for-unknown-policy"), NULL);
}

/* This extension was deactivated */
static void _it_on_deactivate(MidoriExtension *inExtension, gpointer inUserData)
{
	g_return_if_fail(it);

	g_object_unref(it);
	it=NULL;
}

/* Preferences of this extension should be opened */
static void _it_on_open_preferences_response(GtkWidget* inDialog,
												gint inResponse,
												MidoriExtension* inExtension)
{
	gtk_widget_destroy(inDialog);
}

static void _it_on_open_preferences(MidoriExtension *inExtension)
{
	g_return_if_fail(it);

	/* Show preferences window */
	GtkWidget* dialog;

	dialog=interface_tweaks_preferences_new(it);
	gtk_window_set_modal(GTK_WINDOW(dialog), TRUE);
	g_signal_connect(dialog, "response", G_CALLBACK (_it_on_open_preferences_response), inExtension);
	gtk_widget_show_all(dialog);
}

/* Main entry for extension */
MidoriExtension *extension_init(void)
{
	/* Set up extension */
	MidoriExtension	*extension=g_object_new(MIDORI_TYPE_EXTENSION,
												"name", _("Interface tweaks"),
												"description", _("Tweak interface with uncommon options"),
												"version", "0.1" MIDORI_VERSION_SUFFIX,
												"authors", "Stephan Haller <nomad@froevel.de>",
												NULL);

	// TODO: midori_extension_install_boolean(extension, "ask-for-unknown-policy", TRUE);

	g_signal_connect(extension, "activate", G_CALLBACK(_it_on_activate), NULL);
	g_signal_connect(extension, "deactivate", G_CALLBACK(_it_on_deactivate), NULL);
	g_signal_connect(extension, "open-preferences", G_CALLBACK(_it_on_open_preferences), NULL);

	return(extension);
}
