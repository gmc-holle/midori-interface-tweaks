/*
 Copyright (C) 2013 Stephan Haller <nomad@froevel.de>

 This library is free software; you can redistribute it and/or
 modify it under the terms of the GNU Lesser General Public
 License as published by the Free Software Foundation; either
 version 2.1 of the License, or (at your option) any later version.

 See the file COPYING for the full license text.
*/

#ifndef __INTERFACE_TWEAKS_PREFERENCES__
#define __INTERFACE_TWEAKS_PREFERENCES__

#include "config.h"
#include <midori/midori.h>

#include "interface-tweaks.h"

G_BEGIN_DECLS

#define TYPE_INTERFACE_TWEAKS_PREFERENCES				(interface_tweaks_preferences_get_type())
#define INTERFACE_TWEAKS_PREFERENCES(obj)				(G_TYPE_CHECK_INSTANCE_CAST((obj), TYPE_INTERFACE_TWEAKS_PREFERENCES, InterfaceTweaksPreferences))
#define IS_INTERFACE_TWEAKS_PREFERENCES(obj)			(G_TYPE_CHECK_INSTANCE_TYPE((obj), TYPE_INTERFACE_TWEAKS_PREFERENCES))
#define INTERFACE_TWEAKS_PREFERENCES_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST((klass), TYPE_INTERFACE_TWEAKS_PREFERENCES, InterfaceTweaksPreferencesClass))
#define IS_INTERFACE_TWEAKS_PREFERENCES_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE((klass), TYPE_INTERFACE_TWEAKS_PREFERENCES))
#define INTERFACE_TWEAKS_PREFERENCES_GET_CLASS(obj)		(G_TYPE_INSTANCE_GET_CLASS((obj), TYPE_INTERFACE_TWEAKS_PREFERENCES, InterfaceTweaksPreferencesClass))

typedef struct _InterfaceTweaksPreferences				InterfaceTweaksPreferences;
typedef struct _InterfaceTweaksPreferencesClass			InterfaceTweaksPreferencesClass;
typedef struct _InterfaceTweaksPreferencesPrivate		InterfaceTweaksPreferencesPrivate;

struct _InterfaceTweaksPreferences
{
	/* Parent instance */
	GtkDialog							parent_instance;

	/* Private structure */
	InterfaceTweaksPreferencesPrivate	*priv;
};

struct _InterfaceTweaksPreferencesClass
{
	/* Parent class */
	GtkDialogClass						parent_class;
};

/* Public API */
GType interface_tweaks_preferences_get_type(void);

GtkWidget* interface_tweaks_preferences_new(InterfaceTweaks *inManager);

G_END_DECLS

#endif /* __INTERFACE_TWEAKS_PREFERENCES__ */
