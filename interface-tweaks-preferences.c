/*
 Copyright (C) 2013 Stephan Haller <nomad@froevel.de>

 This library is free software; you can redistribute it and/or
 modify it under the terms of the GNU Lesser General Public
 License as published by the Free Software Foundation; either
 version 2.1 of the License, or (at your option) any later version.

 See the file COPYING for the full license text.
*/

#include "interface-tweaks-preferences.h"

/* Define this class in GObject system */
G_DEFINE_TYPE(InterfaceTweaksPreferences,
				interface_tweaks_preferences,
				GTK_TYPE_DIALOG)

/* Properties */
enum
{
	PROP_0,

	PROP_MANAGER,

	PROP_LAST
};

static GParamSpec* InterfaceTweaksPreferencesProperties[PROP_LAST]={ 0, };

/* Forward declarations */
static void _interface_tweaks_preferences_manager_group_minimized_tabs_changed(InterfaceTweaksPreferences *self,GParamSpec *inSpec, gpointer inUserData);
static void _interface_tweaks_preferences_group_minimized_tabs_changed(InterfaceTweaksPreferences *self, gpointer inUserData);

static void _interface_tweaks_preferences_manager_hide_close_on_minimized_tabs_changed(InterfaceTweaksPreferences *self,GParamSpec *inSpec, gpointer inUserData);
static void _interface_tweaks_preferences_hide_close_on_minimized_tabs_changed(InterfaceTweaksPreferences *self, gpointer inUserData);

static void _interface_tweaks_preferences_manager_show_throbber_on_request_starting_changed(InterfaceTweaksPreferences *self,GParamSpec *inSpec, gpointer inUserData);
static void _interface_tweaks_preferences_show_throbber_on_request_starting_changed(InterfaceTweaksPreferences *self, gpointer inUserData);

static void _interface_tweaks_preferences_manager_small_toolbar_changed(InterfaceTweaksPreferences *self,GParamSpec *inSpec, gpointer inUserData);
static void _interface_tweaks_preferences_small_toolbar_changed(InterfaceTweaksPreferences *self, gpointer inUserData);

/* Private structure - access only by public API if needed */
#define INTERFACE_TWEAKS_PREFERENCES_GET_PRIVATE(obj) \
	(G_TYPE_INSTANCE_GET_PRIVATE((obj), TYPE_INTERFACE_TWEAKS_PREFERENCES, InterfaceTweaksPreferencesPrivate))

struct _InterfaceTweaksPreferencesPrivate
{
	/* Extension related */
	InterfaceTweaks			*manager;

	/* Dialog related */
	GtkWidget				*contentArea;
	GtkWidget				*checkboxGroupMinimizedTabs;
	GtkWidget				*checkboxHideCloseOnMinimizedTabs;
	GtkWidget				*checkboxShowThrobberOnRequestStarting;
	GtkWidget				*checkboxSmallToolbar;
};

/* IMPLEMENTATION: Private variables and methods */

/* "group-minimized-tabs" in manager changed or check-box changed */
static void _interface_tweaks_preferences_manager_group_minimized_tabs_changed(InterfaceTweaksPreferences *self,
																				GParamSpec *inSpec,
																				gpointer inUserData)
{
	InterfaceTweaksPreferencesPrivate	*priv=self->priv;
	InterfaceTweaks						*manager=INTERFACE_TWEAKS(inUserData);
	gboolean							state;

	/* Set toogle in widget (but block signal for toggle) */
	g_signal_handlers_block_by_func(priv->checkboxGroupMinimizedTabs,
										G_CALLBACK(_interface_tweaks_preferences_group_minimized_tabs_changed),
										self);

	g_object_get(manager, "group-minimized-tabs", &state, NULL);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(priv->checkboxGroupMinimizedTabs), state);

	g_signal_handlers_unblock_by_func(priv->checkboxGroupMinimizedTabs,
										G_CALLBACK(_interface_tweaks_preferences_group_minimized_tabs_changed),
										self);
}

static void _interface_tweaks_preferences_group_minimized_tabs_changed(InterfaceTweaksPreferences *self, gpointer inUserData)
{
	InterfaceTweaksPreferencesPrivate	*priv=self->priv;
	gboolean							state;

	/* Get toogle state of widget (but block signal for manager) and set in manager */
	g_signal_handlers_block_by_func(priv->manager,
										G_CALLBACK(_interface_tweaks_preferences_manager_group_minimized_tabs_changed),
										self);
	
	state=gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(priv->checkboxGroupMinimizedTabs));
	g_object_set(priv->manager, "group-minimized-tabs", state, NULL);
	
	g_signal_handlers_unblock_by_func(priv->manager,
										G_CALLBACK(_interface_tweaks_preferences_manager_group_minimized_tabs_changed),
										self);
}

/* "hide-close-on-minimized-tab" in manager changed or check-box changed */
static void _interface_tweaks_preferences_manager_hide_close_on_minimized_tabs_changed(InterfaceTweaksPreferences *self,
																						GParamSpec *inSpec,
																						gpointer inUserData)
{
	InterfaceTweaksPreferencesPrivate	*priv=self->priv;
	InterfaceTweaks						*manager=INTERFACE_TWEAKS(inUserData);
	gboolean							state;

	/* Set toogle in widget (but block signal for toggle) */
	g_signal_handlers_block_by_func(priv->checkboxHideCloseOnMinimizedTabs,
										G_CALLBACK(_interface_tweaks_preferences_hide_close_on_minimized_tabs_changed),
										self);

	g_object_get(manager, "hide-close-on-minimized-tabs", &state, NULL);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(priv->checkboxHideCloseOnMinimizedTabs), state);

	g_signal_handlers_unblock_by_func(priv->checkboxHideCloseOnMinimizedTabs,
										G_CALLBACK(_interface_tweaks_preferences_hide_close_on_minimized_tabs_changed),
										self);
}

static void _interface_tweaks_preferences_hide_close_on_minimized_tabs_changed(InterfaceTweaksPreferences *self, gpointer inUserData)
{
	InterfaceTweaksPreferencesPrivate	*priv=self->priv;
	gboolean							state;

	/* Get toogle state of widget (but block signal for manager) and set in manager */
	g_signal_handlers_block_by_func(priv->manager,
										G_CALLBACK(_interface_tweaks_preferences_manager_hide_close_on_minimized_tabs_changed),
										self);
	
	state=gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(priv->checkboxHideCloseOnMinimizedTabs));
	g_object_set(priv->manager, "hide-close-on-minimized-tabs", state, NULL);
	
	g_signal_handlers_unblock_by_func(priv->manager,
										G_CALLBACK(_interface_tweaks_preferences_manager_hide_close_on_minimized_tabs_changed),
										self);
}

/* "show-start-request-throbber" in manager changed or check-box changed */
static void _interface_tweaks_preferences_manager_show_throbber_on_request_starting_changed(InterfaceTweaksPreferences *self,
																							GParamSpec *inSpec,
																							gpointer inUserData)
{
	InterfaceTweaksPreferencesPrivate	*priv=self->priv;
	InterfaceTweaks						*manager=INTERFACE_TWEAKS(inUserData);
	gboolean							state;

	/* Set toogle in widget (but block signal for toggle) */
	g_signal_handlers_block_by_func(priv->checkboxShowThrobberOnRequestStarting,
										G_CALLBACK(_interface_tweaks_preferences_show_throbber_on_request_starting_changed),
										self);
	
	g_object_get(manager, "show-start-request-throbber", &state, NULL);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(priv->checkboxShowThrobberOnRequestStarting), state);
	
	g_signal_handlers_unblock_by_func(priv->checkboxShowThrobberOnRequestStarting,
										G_CALLBACK(_interface_tweaks_preferences_show_throbber_on_request_starting_changed),
										self);
}

static void _interface_tweaks_preferences_show_throbber_on_request_starting_changed(InterfaceTweaksPreferences *self, gpointer inUserData)
{
	InterfaceTweaksPreferencesPrivate	*priv=self->priv;
	gboolean							state;

	/* Get toogle state of widget (but block signal for manager) and set in manager */
	g_signal_handlers_block_by_func(priv->manager,
										G_CALLBACK(_interface_tweaks_preferences_manager_show_throbber_on_request_starting_changed),
										self);
	
	state=gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(priv->checkboxShowThrobberOnRequestStarting));
	g_object_set(priv->manager, "show-start-request-throbber", state, NULL);
	
	g_signal_handlers_unblock_by_func(priv->manager,
										G_CALLBACK(_interface_tweaks_preferences_manager_show_throbber_on_request_starting_changed),
										self);
}

/* "small-toolbar" in manager changed or check-box changed */
static void _interface_tweaks_preferences_manager_small_toolbar_changed(InterfaceTweaksPreferences *self,
																		GParamSpec *inSpec,
																		gpointer inUserData)
{
	InterfaceTweaksPreferencesPrivate	*priv=self->priv;
	InterfaceTweaks						*manager=INTERFACE_TWEAKS(inUserData);
	gboolean							state;

	/* Set toogle in widget (but block signal for toggle) */
	g_signal_handlers_block_by_func(priv->checkboxSmallToolbar,
										G_CALLBACK(_interface_tweaks_preferences_small_toolbar_changed),
										self);
	
	g_object_get(manager, "small-toolbar", &state, NULL);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(priv->checkboxSmallToolbar), state);
	
	g_signal_handlers_unblock_by_func(priv->checkboxSmallToolbar,
										G_CALLBACK(_interface_tweaks_preferences_small_toolbar_changed),
										self);
}

static void _interface_tweaks_preferences_small_toolbar_changed(InterfaceTweaksPreferences *self, gpointer inUserData)
{
	InterfaceTweaksPreferencesPrivate	*priv=self->priv;
	gboolean							state;

	/* Get toogle state of widget (but block signal for manager) and set in manager */
	g_signal_handlers_block_by_func(priv->manager,
										G_CALLBACK(_interface_tweaks_preferences_manager_small_toolbar_changed),
										self);
	
	state=gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(priv->checkboxSmallToolbar));
	g_object_set(priv->manager, "small-toolbar", state, NULL);
	
	g_signal_handlers_unblock_by_func(priv->manager,
										G_CALLBACK(_interface_tweaks_preferences_manager_small_toolbar_changed),
										self);
}


/* IMPLEMENTATION: GObject */

/* Finalize this object */
static void interface_tweaks_preferences_finalize(GObject *inObject)
{
	InterfaceTweaksPreferences			*self=INTERFACE_TWEAKS_PREFERENCES(inObject);
	InterfaceTweaksPreferencesPrivate	*priv=self->priv;

	/* Dispose allocated resources */
	if(priv->manager)
	{
		g_signal_handlers_disconnect_by_data(priv->manager, self);

		g_object_unref(priv->manager);
		priv->manager=NULL;
	}

	/* Call parent's class finalize method */
	G_OBJECT_CLASS(interface_tweaks_preferences_parent_class)->finalize(inObject);
}

/* Set/get properties */
static void interface_tweaks_preferences_set_property(GObject *inObject,
																		guint inPropID,
																		const GValue *inValue,
																		GParamSpec *inSpec)
{
	InterfaceTweaksPreferences			*self=INTERFACE_TWEAKS_PREFERENCES(inObject);
	InterfaceTweaksPreferencesPrivate	*priv=self->priv;
	GObject								*manager;

	switch(inPropID)
	{
		/* Construct-only properties */
		case PROP_MANAGER:
			/* Release old manager if available and disconnect signals */
			if(priv->manager)
			{
				g_signal_handlers_disconnect_by_data(priv->manager, self);

				g_object_unref(priv->manager);
				priv->manager=NULL;
			}

			/* Set new manager and listen to changes of properties */
			manager=g_value_get_object(inValue);
			if(manager)
			{
				priv->manager=g_object_ref(manager);

				g_signal_connect_swapped(priv->manager,
											"notify::group-minimized-tabs",
											G_CALLBACK(_interface_tweaks_preferences_manager_group_minimized_tabs_changed),
											self);
				_interface_tweaks_preferences_manager_group_minimized_tabs_changed(self, NULL, priv->manager);

				g_signal_connect_swapped(priv->manager,
											"notify::hide-close-on-minimized-tabs",
											G_CALLBACK(_interface_tweaks_preferences_manager_hide_close_on_minimized_tabs_changed),
											self);
				_interface_tweaks_preferences_manager_hide_close_on_minimized_tabs_changed(self, NULL, priv->manager);

				g_signal_connect_swapped(priv->manager,
											"notify::show-start-request-throbber",
											G_CALLBACK(_interface_tweaks_preferences_manager_show_throbber_on_request_starting_changed),
											self);
				_interface_tweaks_preferences_manager_show_throbber_on_request_starting_changed(self, NULL, priv->manager);

				g_signal_connect_swapped(priv->manager,
											"notify::small-toolbar",
											G_CALLBACK(_interface_tweaks_preferences_manager_small_toolbar_changed),
											self);
				_interface_tweaks_preferences_manager_small_toolbar_changed(self, NULL, priv->manager);
			}
			break;

		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(inObject, inPropID, inSpec);
			break;
	}
}

static void interface_tweaks_preferences_get_property(GObject *inObject,
																		guint inPropID,
																		GValue *outValue,
																		GParamSpec *inSpec)
{
	InterfaceTweaksPreferences	*self=INTERFACE_TWEAKS_PREFERENCES(inObject);

	switch(inPropID)
	{
		case PROP_MANAGER:
			g_value_set_object(outValue, self->priv->manager);
			break;

		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(inObject, inPropID, inSpec);
			break;
	}
}

/* Class initialization
 * Override functions in parent classes and define properties and signals
 */
static void interface_tweaks_preferences_class_init(InterfaceTweaksPreferencesClass *klass)
{
	GObjectClass		*gobjectClass=G_OBJECT_CLASS(klass);

	/* Override functions */
	gobjectClass->finalize=interface_tweaks_preferences_finalize;
	gobjectClass->set_property=interface_tweaks_preferences_set_property;
	gobjectClass->get_property=interface_tweaks_preferences_get_property;

	/* Set up private structure */
	g_type_class_add_private(klass, sizeof(InterfaceTweaksPreferencesPrivate));

	/* Define properties */
	InterfaceTweaksPreferencesProperties[PROP_MANAGER]=
		g_param_spec_object("manager",
								_("Interface tweaks instance"),
								_("Instance of current interface tweaks manager"),
								TYPE_INTERFACE_TWEAKS,
								G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);

	g_object_class_install_properties(gobjectClass, PROP_LAST, InterfaceTweaksPreferencesProperties);
}

/* Object initialization
 * Create private structure and set up default values
 */
static void interface_tweaks_preferences_init(InterfaceTweaksPreferences *self)
{
	InterfaceTweaksPreferencesPrivate		*priv;
	GtkWidget								*widget;
	gchar									*dialogTitle;
	GtkWidget								*vbox;
	gint									width, height;

	priv=self->priv=INTERFACE_TWEAKS_PREFERENCES_GET_PRIVATE(self);

	/* Set up default values */
	priv->manager=NULL;

	/* Get content area to add gui controls to */
	priv->contentArea=gtk_dialog_get_content_area(GTK_DIALOG(self));
#ifdef GTK__3_0_VERSION
	vbox=gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
	gtk_box_set_homogeneous(GTK_BOX(vbox), FALSE);
#else
	vbox=gtk_vbox_new(FALSE, 0);
#endif

	/* Set up dialog */
	dialogTitle=_("Configure interface tweaks");

	gtk_window_set_title(GTK_WINDOW(self), dialogTitle);
	gtk_window_set_icon_name(GTK_WINDOW(self), GTK_STOCK_PROPERTIES);

	sokoke_widget_get_text_size(GTK_WIDGET(self), "M", &width, &height);
	gtk_window_set_default_size(GTK_WINDOW(self), width*52, -1);

	widget=sokoke_xfce_header_new(gtk_window_get_icon_name(GTK_WINDOW(self)), dialogTitle);
	if(widget) gtk_box_pack_start(GTK_BOX(priv->contentArea), widget, FALSE, FALSE, 0);

	gtk_dialog_add_button(GTK_DIALOG(self), GTK_STOCK_CLOSE, GTK_RESPONSE_CLOSE);

	/* Add "group-minimized-tabs" checkbox */
	priv->checkboxGroupMinimizedTabs=gtk_check_button_new_with_mnemonic(_("_Group minimized tabs at beginning"));
	g_signal_connect_swapped(priv->checkboxGroupMinimizedTabs,
								"toggled",
								G_CALLBACK(_interface_tweaks_preferences_group_minimized_tabs_changed),
								self);
	gtk_box_pack_start(GTK_BOX(vbox), priv->checkboxGroupMinimizedTabs, TRUE, TRUE, 5);

	/* Add "hide-close-on-minimized-tabs" checkbox */
	priv->checkboxHideCloseOnMinimizedTabs=gtk_check_button_new_with_mnemonic(_("_Hide close button at minimized tabs"));
	g_signal_connect_swapped(priv->checkboxHideCloseOnMinimizedTabs,
								"toggled",
								G_CALLBACK(_interface_tweaks_preferences_hide_close_on_minimized_tabs_changed),
								self);
	gtk_box_pack_start(GTK_BOX(vbox), priv->checkboxHideCloseOnMinimizedTabs, TRUE, TRUE, 5);

	/* Add "show-start-request-throbber" checkbox */
	priv->checkboxShowThrobberOnRequestStarting=gtk_check_button_new_with_mnemonic(_("_Show throbber as soon as request is sent"));
	g_signal_connect_swapped(priv->checkboxShowThrobberOnRequestStarting,
								"toggled",
								G_CALLBACK(_interface_tweaks_preferences_show_throbber_on_request_starting_changed),
								self);
	gtk_box_pack_start(GTK_BOX(vbox), priv->checkboxShowThrobberOnRequestStarting, TRUE, TRUE, 5);

	/* Add "small-toolbar" checkbox */
	priv->checkboxSmallToolbar=gtk_check_button_new_with_mnemonic(_("S_mall icons in toolbar"));
	g_signal_connect_swapped(priv->checkboxSmallToolbar,
								"toggled",
								G_CALLBACK(_interface_tweaks_preferences_small_toolbar_changed),
								self);
	gtk_box_pack_start(GTK_BOX(vbox), priv->checkboxSmallToolbar, TRUE, TRUE, 5);

	/* Finalize setup of content area */
	gtk_container_add(GTK_CONTAINER(priv->contentArea), vbox);
}

/* Implementation: Public API */

/* Create new object */
GtkWidget* interface_tweaks_preferences_new(InterfaceTweaks *inManager)
{
	return(g_object_new(TYPE_INTERFACE_TWEAKS_PREFERENCES,
							"manager", inManager,
							NULL));
}
