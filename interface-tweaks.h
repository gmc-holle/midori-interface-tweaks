/*
 Copyright (C) 2013 Stephan Haller <nomad@froevel.de>

 This library is free software; you can redistribute it and/or
 modify it under the terms of the GNU Lesser General Public
 License as published by the Free Software Foundation; either
 version 2.1 of the License, or (at your option) any later version.

 See the file COPYING for the full license text.
*/

#ifndef __INTERFACE_TWEAKS__
#define __INTERFACE_TWEAKS__

#include "config.h"
#include <midori/midori.h>

G_BEGIN_DECLS

/* Cookie permission manager object */
#define TYPE_INTERFACE_TWEAKS				(interface_tweaks_get_type())
#define INTERFACE_TWEAKS(obj)				(G_TYPE_CHECK_INSTANCE_CAST((obj), TYPE_INTERFACE_TWEAKS, InterfaceTweaks))
#define IS_INTERFACE_TWEAKS(obj)			(G_TYPE_CHECK_INSTANCE_TYPE((obj), TYPE_INTERFACE_TWEAKS))
#define INTERFACE_TWEAKS_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST((klass), TYPE_INTERFACE_TWEAKS, InterfaceTweaksClass))
#define IS_INTERFACE_TWEAKS_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE((klass), TYPE_INTERFACE_TWEAKS))
#define INTERFACE_TWEAKS_GET_CLASS(obj)		(G_TYPE_INSTANCE_GET_CLASS((obj), TYPE_INTERFACE_TWEAKS, InterfaceTweaksClass))

typedef struct _InterfaceTweaks				InterfaceTweaks;
typedef struct _InterfaceTweaksClass		InterfaceTweaksClass;
typedef struct _InterfaceTweaksPrivate		InterfaceTweaksPrivate;

struct _InterfaceTweaks
{
	/* Parent instance */
	GObject					parent_instance;

	/* Private structure */
	InterfaceTweaksPrivate	*priv;
};

struct _InterfaceTweaksClass
{
	/* Parent class */
	GObjectClass			parent_class;
};

/* Public API */
GType interface_tweaks_get_type(void);

InterfaceTweaks* interface_tweaks_new(MidoriExtension *inExtension, MidoriApp *inApp);

G_END_DECLS

#endif /* __INTERFACE_TWEAKS__ */
