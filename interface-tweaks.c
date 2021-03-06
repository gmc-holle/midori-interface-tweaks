/*
 Copyright (C) 2013 Stephan Haller <nomad@froevel.de>

 This library is free software; you can redistribute it and/or
 modify it under the terms of the GNU Lesser General Public
 License as published by the Free Software Foundation; either
 version 2.1 of the License, or (at your option) any later version.

 See the file COPYING for the full license text.
*/

#include "interface-tweaks.h"

#define MYTYPENAME(obj) ((obj) ? G_OBJECT_TYPE_NAME((obj)) : "<nil>")

#define HISTORY_DATABASE_FILENAME	"history.db"

/* Define this class in GObject system */
G_DEFINE_TYPE(InterfaceTweaks,
				interface_tweaks,
				G_TYPE_OBJECT)

/* Properties */
enum
{
	PROP_0,

	PROP_EXTENSION,
	PROP_APPLICATION,

	PROP_GROUP_MINIMIZED_TABS,
	PROP_HIDE_CLOSE_ON_MINIMIZED_TABS,
	PROP_SHOW_START_REQUEST_THROBBER,
	PROP_SMALL_TOOLBAR,
	PROP_AUTOCOMPLETE_LOCATIONBAR,

	PROP_LAST
};

static GParamSpec* InterfaceTweaksProperties[PROP_LAST]={ 0, };

/* Private structure - access only by public API if needed */
#define INTERFACE_TWEAKS_GET_PRIVATE(obj) \
	(G_TYPE_INSTANCE_GET_PRIVATE((obj), TYPE_INTERFACE_TWEAKS, InterfaceTweaksPrivate))

struct _InterfaceTweaksPrivate
{
	/* Extension related */
	MidoriExtension		*extension;
	MidoriApp			*application;
	gboolean			groupMinimizedTabs;
	gboolean			hideCloseOnMinimizedTabs;
	gboolean			showStartRequestThrobber;
	gboolean			smallToolbar;
	gboolean			autocompleteLocationbar;

	/* Initialization state */
	gboolean			inited;

	/* Additional data */
	GtkIconSize			originToolbarIconSize;
	GtkToolbarStyle		originToolbarStyle;

	GtkListStore		*completionModel;
	GHashTable			*completionModelDomains;
};

typedef struct
{
	GtkAction			*action;
	GtkWidget			*widget;
} InterfaceTweaksLocationbarLookup;

/* IMPLEMENTATION: Private variables and methods */

/* Unblock all signal handlers in list retrieved when blocking them in
 * _interface_tweaks_block_all_handlers()
 */
static void _interface_tweaks_unblock_handlers(GObject *inObject, GSList *inHandlers)
{
	/* Iterate through list of signal handler IDs and unblock them in object instance */
	while(inHandlers)
	{
		g_signal_handler_unblock(inObject, GPOINTER_TO_SIZE(inHandlers->data));
		inHandlers=inHandlers->next;
	}
}

/* Block all unblocked signal handlers connect to signal ID in object instance.
 * Returns a list of all signal handlers block to get them unblocked again in
 * _interface_tweaks_unblock_handlers()
 */
static GSList* _interface_tweaks_block_all_handlers(GObject *inObject, guint inSignalID)
{
	gulong		handlerID;
	GSList		*blockedHandlers=NULL;

	/* Block unblocked signal handlers and keep a list of them */
	do
	{
		/* Find unblocked signal handler to signal ID in object instance.
		 * This will always the next one as the previous one has blocked
		 * or 0L if all signal handlers were blocked.
		 */
		handlerID=g_signal_handler_find(	inObject,
											G_SIGNAL_MATCH_ID | G_SIGNAL_MATCH_UNBLOCKED,
											inSignalID,
											0,
											NULL,
											NULL,
											NULL);

		/* Check if we found an unblocked signal handler */
		if(handlerID!=0L)
		{
			/* Add its signal handler ID to list to signal handlers blocked by us */
			blockedHandlers=g_slist_prepend(blockedHandlers, GSIZE_TO_POINTER(handlerID));

			/* Block signal */
			g_signal_handler_block(inObject, handlerID);
		}
	}
	while(handlerID!=0);

	/* Return list of block signal handlers */
	return(blockedHandlers);
}

/* Find position of first maximized tab of browser.
 * Returns -1 if position could not determined.
 */
static gint _interface_tweak_get_browser_first_maximized_tab_position(MidoriBrowser *inBrowser, MidoriView *inIgnoreView)
{
	g_return_val_if_fail(MIDORI_IS_BROWSER(inBrowser), -1);
	g_return_val_if_fail(inIgnoreView==NULL || MIDORI_IS_VIEW(inIgnoreView), -1);

	GList			*tabs, *iter;
	gint			position, iterPosition;

	/* Get list of browser tabs and return index of first maximized tab */
	tabs=midori_browser_get_tabs(inBrowser);
	position=iterPosition=-1;
	for(iter=tabs; iter && position==-1; iter=iter->next)
	{
		MidoriView	*view=MIDORI_VIEW(iter->data);
		gboolean	minimized;

		iterPosition++;
		if(view && view!=inIgnoreView)
		{
			minimized=FALSE;
			g_object_get(view, "minimized", &minimized, NULL);
			if(minimized==FALSE) position=iterPosition;
		}
	}
	g_list_free(tabs);

	return(position);
}

/* Find locationbar of browser (and entry of locationbar) */
static void _interface_tweaks_find_browser_locationbar_entry_callback(GtkWidget *inWidget, gpointer inUserData)
{
	g_return_if_fail(GTK_IS_WIDGET(inWidget));

	if(GTK_IS_ENTRY(inWidget))
	{
		InterfaceTweaksLocationbarLookup	**item=(InterfaceTweaksLocationbarLookup**)inUserData;
	
		if(item) (*item)->widget=inWidget;
	}
		else if(GTK_IS_CONTAINER(inWidget))
		{
			gtk_container_foreach(GTK_CONTAINER(inWidget), _interface_tweaks_find_browser_locationbar_entry_callback, inUserData);
		}
}

static void _interface_tweaks_find_browser_locationbar_callback(GtkWidget *inWidget, gpointer inUserData)
{
	g_return_if_fail(GTK_IS_WIDGET(inWidget));

	if(GTK_IS_TOOL_ITEM(inWidget))
	{
		GtkAction	*action;

		action=gtk_activatable_get_related_action(GTK_ACTIVATABLE(inWidget));
		if(g_strcmp0(gtk_action_get_name(action), "Location")==0)
		{
			InterfaceTweaksLocationbarLookup	**item=(InterfaceTweaksLocationbarLookup**)inUserData;

			if(item) (*item)->action=action;

			gtk_container_foreach(GTK_CONTAINER(inWidget), _interface_tweaks_find_browser_locationbar_entry_callback, inUserData);
		}
			else if(g_strcmp0(gtk_action_get_name(action), "LocationSearch")==0)
			{
				InterfaceTweaksLocationbarLookup	**item=(InterfaceTweaksLocationbarLookup**)inUserData;
				GtkWidget							*locationChild;

				if(item) (*item)->action=action;

				locationChild=midori_paned_action_get_child_by_name(MIDORI_PANED_ACTION(action), "Location");
				if(locationChild)
				{
					gtk_container_foreach(GTK_CONTAINER(locationChild), _interface_tweaks_find_browser_locationbar_entry_callback, inUserData);
				}
			}
	}
		else if(GTK_IS_CONTAINER(inWidget))
		{
			gtk_container_foreach(GTK_CONTAINER(inWidget), _interface_tweaks_find_browser_locationbar_callback, inUserData);
		}
}

static void _interface_tweaks_find_browser_locationbar(MidoriBrowser *inBrowser, InterfaceTweaksLocationbarLookup *outResult)
{
	g_return_if_fail(MIDORI_IS_BROWSER(inBrowser));
	g_return_if_fail(outResult);

	GtkToolbar		*navigationbar=NULL;

	g_object_get(inBrowser, "navigationbar", &navigationbar, NULL);
	if(navigationbar)
	{
		outResult->action=NULL;
		outResult->widget=NULL;

		if(GTK_IS_CONTAINER(navigationbar))
		{
			gtk_container_foreach(GTK_CONTAINER(navigationbar), _interface_tweaks_find_browser_locationbar_callback, &outResult);
		}

		g_object_unref(navigationbar);
	}
}

/* Find close button of view */
static void _interface_tweaks_find_view_close_button_callback(GtkWidget *inWidget, gpointer inUserData)
{
	g_return_if_fail(GTK_IS_WIDGET(inWidget));

	if(g_strcmp0(gtk_widget_get_name(inWidget), "midori-close-button")==0)
	{
		GtkWidget	**closeButton=(GtkWidget**)inUserData;

		if(closeButton) *closeButton=GTK_WIDGET(inWidget);
	}
		else if(GTK_IS_CONTAINER(inWidget))
		{
			gtk_container_foreach(GTK_CONTAINER(inWidget), _interface_tweaks_find_view_close_button_callback, inUserData);
		}
}

static GtkWidget* _interface_tweaks_find_view_close_button(MidoriView *inView)
{
	g_return_val_if_fail(MIDORI_IS_VIEW(inView), NULL);

	GtkWidget		*widget;
	GtkWidget		*closeButton=NULL;

	widget=midori_view_get_proxy_tab_label(inView);
	if(g_strcmp0(gtk_widget_get_name(widget), "midori-close-button")==0) closeButton=GTK_WIDGET(widget);
		else if(GTK_IS_CONTAINER(widget)) gtk_container_foreach(GTK_CONTAINER(widget), _interface_tweaks_find_view_close_button_callback, &closeButton);

	return(closeButton);
}

/* Find throbber of view or browser */
static void _interface_tweaks_find_view_throbber_callback(GtkWidget *inWidget, gpointer inUserData)
{
	g_return_if_fail(GTK_IS_WIDGET(inWidget));

	if(KATZE_IS_THROBBER(inWidget))
	{
		KatzeThrobber	**throbber=(KatzeThrobber**)inUserData;

		if(throbber) *throbber=KATZE_THROBBER(inWidget);
	}
		else if(GTK_IS_CONTAINER(inWidget))
		{
			gtk_container_foreach(GTK_CONTAINER(inWidget), _interface_tweaks_find_view_throbber_callback, inUserData);
		}
}

static KatzeThrobber* _interface_tweaks_find_view_throbber(MidoriView *inView)
{
	g_return_val_if_fail(MIDORI_IS_VIEW(inView), NULL);

	GtkWidget		*widget;
	KatzeThrobber	*throbber=NULL;

	widget=midori_view_get_proxy_tab_label(inView);
	if(KATZE_IS_THROBBER(widget)) throbber=KATZE_THROBBER(widget);
		else if(GTK_IS_CONTAINER(widget)) gtk_container_foreach(GTK_CONTAINER(widget), _interface_tweaks_find_view_throbber_callback, &throbber);

	return(throbber);
}

/* Add an URI to completion model for autocompletion */
static void _interface_tweaks_add_uri_to_completion_model(InterfaceTweaks *self, const gchar *inURI)
{
	g_return_if_fail(IS_INTERFACE_TWEAKS(self));
	g_return_if_fail(inURI!=NULL);

	InterfaceTweaksPrivate	*priv=self->priv;
	SoupURI					*soupURI;
	gchar					*modelURI;
	GtkTreeIter				iter;

	/* Create SoupURI from URI to retrieve host and/or scheme for completion */
	soupURI=soup_uri_new(inURI);
	g_return_if_fail(soupURI!=NULL);

	/* Add URI with scheme and host to completion */
	modelURI=g_strdup_printf("%s://%s", soup_uri_get_scheme(soupURI), soup_uri_get_host(soupURI));
	if(modelURI && g_hash_table_lookup(priv->completionModelDomains, modelURI)==NULL)
	{
		gtk_list_store_append(priv->completionModel, &iter);
		gtk_list_store_set(priv->completionModel, &iter, 0, modelURI, -1);
		g_hash_table_insert(priv->completionModelDomains, modelURI, GINT_TO_POINTER(TRUE));
	}
	if(modelURI) g_free(modelURI);

	/* Add URI without scheme - only host - to completion */
	modelURI=g_strdup(soup_uri_get_host(soupURI));
	if(modelURI && g_hash_table_lookup(priv->completionModelDomains, modelURI)==NULL)
	{
		gtk_list_store_append(priv->completionModel, &iter);
		gtk_list_store_set(priv->completionModel, &iter, 0, modelURI, -1);
		g_hash_table_insert(priv->completionModelDomains, modelURI, GINT_TO_POINTER(TRUE));
	}
	if(modelURI) g_free(modelURI);

	/* Release allocated resources */
	soup_uri_free(soupURI);
}

/* Autocompletion in locationbar has found a match */
static gboolean _interface_tweaks_on_insert_prefix(InterfaceTweaks *self,
													gchar *inPrefix,
													gpointer inUserData)
{
	g_return_val_if_fail(IS_INTERFACE_TWEAKS(self), FALSE);
	g_return_val_if_fail(GTK_IS_ENTRY_COMPLETION(inUserData), FALSE);

	InterfaceTweaksPrivate	*priv=self->priv;
	GtkEntryCompletion		*completion=GTK_ENTRY_COMPLETION(inUserData);
	GtkEntry				*entry;
	guint					changedSignalID;
	const gchar				*text;
	gint					textLength;
	GSList					*handlers;

	/* Check if we should auto-complete URIs in locationbar */
	if(priv->autocompleteLocationbar==FALSE) return(TRUE);

	/* Get entry of completion */
	entry=GTK_ENTRY(gtk_entry_completion_get_entry(completion));

	/* Get current text (that is before text is set by completion)
	 * and remember its length
	 */
	text=gtk_entry_get_text(entry);
	textLength=strlen(text);

	/* Get "changed" signal ID */
	changedSignalID=g_signal_lookup("changed", GTK_TYPE_ENTRY);

	/* Emit "changed" event for all signal handlers as the entry
	 * still contains the unmodified text.
	 */
	g_signal_emit(entry, changedSignalID, 0);

	/* Block all unblocked signal handlers for "changed" signal, update entry text
	 * to the text of completion and unblock these signal handlers again.
	 * This way we prevent "changed" signal emissions for completion-text.
	 */
	handlers=_interface_tweaks_block_all_handlers(G_OBJECT(entry), changedSignalID);

	gtk_entry_set_text(entry, inPrefix);
	gtk_editable_select_region(GTK_EDITABLE(entry), textLength, -1);

	_interface_tweaks_unblock_handlers(G_OBJECT(entry), handlers);
	g_slist_free(handlers);

	/* Tell emission that we handled signal and stop further processing */
	return(TRUE);
}

/* A key was pressed in locationbar */
static gboolean _interface_tweaks_on_key_press_event(GSignalInvocationHint *inHint,
														guint inNumberParams,
														const GValue *inParams,
														gpointer inUserData)
{
	g_return_val_if_fail(GTK_IS_ENTRY(inUserData), FALSE);

	GtkEntry				*entry=GTK_ENTRY(inUserData);
	GtkWidget				*target;
	GdkEventKey				*event;
	guint					changedSignalID;
	GSList					*handlers;

	/* Get target of this event and check if it is for locationbar entry */
	target=GTK_WIDGET(g_value_get_object(inParams));
	if(target==GTK_WIDGET(entry))
	{
		/* Get key-event data */
		inParams++;
		event=(GdkEventKey*)g_value_get_boxed(inParams);

		/* Check if key-event would _not_ add any characters */
		if(!(gdk_keyval_is_upper(event->keyval)==gdk_keyval_is_lower(event->keyval) &&
				gdk_unicode_to_keyval(gdk_keyval_to_unicode(event->keyval))!=event->keyval))
		{
			/* Get "changed" signal ID */
			changedSignalID=g_signal_lookup("changed", GTK_TYPE_ENTRY);

			/* Block all unblocked signal handlers for "changed" signal, remove selected
			 * text region from entry text and unblock these signal handlers again.
			 * This way we keep Midori's auto-completion working. Otherwise it fetches the
			 * complete entry text with the text portion in selected text region and adds
			 * the pressed key to it which results in a completely wrong text for auto-completion.
			 */
			handlers=_interface_tweaks_block_all_handlers(G_OBJECT(entry), changedSignalID);

			gtk_editable_delete_selection(GTK_EDITABLE(entry));

			_interface_tweaks_unblock_handlers(G_OBJECT(entry), handlers);
			g_slist_free(handlers);
		}
	}

	return(TRUE);
}

/* The locationbar entry was destroyed */
static void _interface_tweaks_on_destroyed(InterfaceTweaks *self, gpointer inUserData)
{
	g_return_if_fail(IS_INTERFACE_TWEAKS(self));
	g_return_if_fail(GTK_IS_ENTRY(inUserData));

	GtkEntry				*entry=GTK_ENTRY(inUserData);
	guint					keyPressSignalID;
	gulong					hookID;

	/* Remove emission handler from entry if it exists and stops further callbacks
	 * to our emission hook for "key-press-event"s
	 */
	hookID=GPOINTER_TO_SIZE(g_object_get_data(G_OBJECT(entry), "interface-tweaks-hook-id"));
	if(hookID>0)
	{
		keyPressSignalID=g_signal_lookup("key-press-event", GTK_TYPE_ENTRY);
		g_signal_remove_emission_hook(keyPressSignalID, hookID);

		hookID=0L;
		g_object_set_data(G_OBJECT(entry), "interface-tweaks-hook-id", GSIZE_TO_POINTER(hookID));
	}
}

/* A tab was minimized (pinned) or maximized (unpinned) - Check for grouping tabs */
static void _interface_tweaks_on_notify_minimized_tab_for_group_tabs(InterfaceTweaks *self, GParamSpec *inSpec, gpointer inUserData)
{
	g_return_if_fail(IS_INTERFACE_TWEAKS(self));
	g_return_if_fail(MIDORI_IS_VIEW(inUserData));

	InterfaceTweaksPrivate	*priv=self->priv;
	MidoriView				*view=MIDORI_VIEW(inUserData);
	gboolean				minimized;
	MidoriBrowser			*browser;
	GtkContainer			*notebook;
	gint					firstMaximizedPosition;

	/* Check if tab was minimized */
	minimized=FALSE;
	g_object_get(view, "minimized", &minimized, NULL);

	/* Check if tab we should group minimized tabs to front */
	if(!priv->groupMinimizedTabs) return;
	
	/* Get browser of tab */
	browser=midori_browser_get_for_widget(GTK_WIDGET(view));
	if(!browser) return;

	/* Get notebook containing tabs of browser */
	notebook=NULL;
	g_object_get(browser, "notebook", &notebook, NULL);
	if(!notebook) return;

	/* Find position of first maximized tab */
	firstMaximizedPosition=_interface_tweak_get_browser_first_maximized_tab_position(browser, view);

	/* If tab is maximized then it moves away from front and
	 * the position must be decreased by one to get the first maximized tab
	 */
	if(!minimized) firstMaximizedPosition--;

	/* Reorder tab now */
	g_signal_handlers_block_by_func(view, G_CALLBACK(_interface_tweaks_on_notify_minimized_tab_for_group_tabs), self);

	if(firstMaximizedPosition<0) firstMaximizedPosition=0;
	gtk_notebook_reorder_child(GTK_NOTEBOOK(notebook), GTK_WIDGET(view), firstMaximizedPosition);

	g_signal_handlers_unblock_by_func(view, G_CALLBACK(_interface_tweaks_on_notify_minimized_tab_for_group_tabs), self);

	g_object_unref(notebook);
}

/* A tab was minimized (pinned) or maximized (unpinned) - Check for hiding close button */
static void _interface_tweaks_on_notify_minimized_tab_for_close_button(InterfaceTweaks *self, GParamSpec *inSpec, gpointer inUserData)
{
	g_return_if_fail(IS_INTERFACE_TWEAKS(self));
	g_return_if_fail(MIDORI_IS_VIEW(inUserData));

	InterfaceTweaksPrivate	*priv=self->priv;
	MidoriView				*view=MIDORI_VIEW(inUserData);
	gboolean				minimized;
	MidoriSettings			*settings;
	gboolean				hasCloseButton;
	GtkWidget				*closeButton;

	/* Check if tab was minimized */
	minimized=FALSE;
	g_object_get(view, "minimized", &minimized, NULL);

	/* Get Midori settings */
	if(!priv->application) return;

	settings=NULL;
	g_object_get(priv->application, "settings", &settings, NULL);
	if(!settings) return;
	
	/* Only hide close button if tabs have close buttons */
	hasCloseButton=FALSE;
	g_object_get(settings, "close-buttons-on-tabs", &hasCloseButton, NULL);
	if(hasCloseButton)
	{
		/* Find close button and show or hide it */
		closeButton=_interface_tweaks_find_view_close_button(view);
		if(closeButton)
		{
			/* Check if we should hide close button on minimized tab */
			if(minimized && priv->hideCloseOnMinimizedTabs) gtk_widget_hide(closeButton);
				else gtk_widget_show(closeButton);
		}
	}

	g_object_unref(settings);
}

/* Status of loading a site has changed */
static void _interface_tweaks_on_notify_view_load_status(InterfaceTweaks *self, GParamSpec *inSpec, gpointer inUserData)
{
	g_return_if_fail(IS_INTERFACE_TWEAKS(self));
	g_return_if_fail(WEBKIT_IS_WEB_VIEW(inUserData));

	InterfaceTweaksPrivate	*priv=self->priv;
	WebKitWebView			*webkitView=WEBKIT_WEB_VIEW(inUserData);
	WebKitLoadStatus		status;

	/* Check load status */ 
	status=webkit_web_view_get_load_status(webkitView);

	/* If webview was prepared to start network request */
	if(status==WEBKIT_LOAD_PROVISIONAL)
	{
		/* Show start-request throbber now if set */
		if(priv->showStartRequestThrobber)
		{
			MidoriView		*view;
			KatzeThrobber	*throbber;

			/* Get Midori view and the size for icon in tab */
			view=midori_view_get_for_widget(GTK_WIDGET(webkitView));

			/* Start throbber animation of view */
			throbber=_interface_tweaks_find_view_throbber(view);
			if(throbber) katze_throbber_set_animated(throbber, TRUE);
		}
	}

	/* First data of network response has been sent or first data of
	 * network response has been received
	 */
	if(status==WEBKIT_LOAD_COMMITTED)
	{
		const gchar			*uri;

		/* Get URI for current request */
		uri=webkit_web_view_get_uri(webkitView);

		/* Add URI to completion model */
		_interface_tweaks_add_uri_to_completion_model(self, uri);
	}
}

/* A tab of a browser was reordered */
static void _interface_tweaks_on_tab_reordered(InterfaceTweaks *self,
												GtkWidget *inChild,
												guint inPageNumber,
												gpointer inUserData)
{
	g_return_if_fail(IS_INTERFACE_TWEAKS(self));
	g_return_if_fail(MIDORI_IS_VIEW(inChild));
	g_return_if_fail(GTK_IS_NOTEBOOK(inUserData));

	InterfaceTweaksPrivate	*priv=self->priv;
	MidoriView				*view=MIDORI_VIEW(inChild);
	GtkNotebook				*notebook=GTK_NOTEBOOK(inUserData);
	MidoriBrowser			*browser;
	gboolean				minimized;
	gint					firstMaximizedPosition;

	/* Check if tab we should group minimized tabs to front */
	if(!priv->groupMinimizedTabs) return;

	/* Check if tab was minimized */
	minimized=FALSE;
	g_object_get(view, "minimized", &minimized, NULL);

	/* Get browser of tab */
	browser=midori_browser_get_for_widget(GTK_WIDGET(view));
	if(!browser) return;

	/* Find position of first maximized tab */
	firstMaximizedPosition=_interface_tweak_get_browser_first_maximized_tab_position(browser, view);

	/* If tab is maximized then it moves away from front and
	 * the position must be decreased by one to get the first maximized tab
	 */
	if(!minimized) firstMaximizedPosition--;

	/* Reorder tab now */
	if(firstMaximizedPosition<0) firstMaximizedPosition=0;
	if((minimized==TRUE && inPageNumber>=firstMaximizedPosition) ||
		(minimized==FALSE && inPageNumber<firstMaximizedPosition))
	{
		g_signal_handlers_block_by_func(view, G_CALLBACK(_interface_tweaks_on_notify_minimized_tab_for_group_tabs), self);

		gtk_notebook_reorder_child(GTK_NOTEBOOK(notebook), GTK_WIDGET(view), firstMaximizedPosition);

		g_signal_handlers_unblock_by_func(view, G_CALLBACK(_interface_tweaks_on_notify_minimized_tab_for_group_tabs), self);
	}
}

/* A tab of a browser was added */
static void _interface_tweaks_on_add_tab(InterfaceTweaks *self, MidoriView *inView, gpointer inUserData)
{
	g_return_if_fail(IS_INTERFACE_TWEAKS(self));
	g_return_if_fail(MIDORI_IS_VIEW(inView));
	g_return_if_fail(MIDORI_IS_BROWSER(inUserData));

	WebKitWebView			*webkitView;

	/* Connect signals to midori view and webkit webview to monitor loading state */
	g_signal_connect_swapped(inView, "notify::minimized", G_CALLBACK(_interface_tweaks_on_notify_minimized_tab_for_close_button), self);
	_interface_tweaks_on_notify_minimized_tab_for_close_button(self, NULL, inView);

	g_signal_connect_swapped(inView, "notify::minimized", G_CALLBACK(_interface_tweaks_on_notify_minimized_tab_for_group_tabs), self);
	_interface_tweaks_on_notify_minimized_tab_for_group_tabs(self, NULL, inView);

	webkitView=WEBKIT_WEB_VIEW(midori_view_get_web_view(inView));
	g_signal_connect_swapped(webkitView, "notify::load-status", G_CALLBACK(_interface_tweaks_on_notify_view_load_status), self);
	_interface_tweaks_on_notify_view_load_status(self, NULL, webkitView);
}

/* A browser window was added */
static void _interface_tweaks_on_add_browser(InterfaceTweaks *self, MidoriBrowser *inBrowser, gpointer inUserData)
{
	g_return_if_fail(IS_INTERFACE_TWEAKS(self));
	g_return_if_fail(MIDORI_IS_BROWSER(inBrowser));

	InterfaceTweaksPrivate				*priv=self->priv;
	GtkNotebook							*notebook;
	GList								*tabs, *iter;
	InterfaceTweaksLocationbarLookup	locationbar;
	GtkEntryCompletion					*completion;
	guint								keyPressSignalID;
	gulong								hookID;

	/* Set up all current available tabs in browser */
	tabs=midori_browser_get_tabs(inBrowser);
	for(iter=tabs; iter; iter=g_list_next(iter))
	{
		_interface_tweaks_on_add_tab(self, iter->data, inBrowser);
	}
	g_list_free(tabs);

	/* Listen to new tabs opened in browser */
	g_signal_connect_swapped(inBrowser, "add-tab", G_CALLBACK(_interface_tweaks_on_add_tab), self);

	notebook=NULL;
	g_object_get(inBrowser, "notebook", &notebook, NULL);
	if(notebook)
	{
		g_signal_connect_swapped(notebook, "page-added", G_CALLBACK(_interface_tweaks_on_tab_reordered), self);
		g_signal_connect_swapped(notebook, "page-reordered", G_CALLBACK(_interface_tweaks_on_tab_reordered), self);
		g_object_unref(notebook);
	}

	/* Listen to locationbar signals */
	_interface_tweaks_find_browser_locationbar(inBrowser, &locationbar);
	if(locationbar.widget)
	{
		/* Add completion to location entry and setup signals */
		completion=gtk_entry_completion_new();
		gtk_entry_completion_set_inline_completion(completion, TRUE);
		gtk_entry_completion_set_inline_selection(completion, FALSE);
		gtk_entry_completion_set_popup_completion(completion, FALSE);
		gtk_entry_completion_set_model(completion, GTK_TREE_MODEL(priv->completionModel));
		gtk_entry_completion_set_text_column(completion, 0);
		gtk_entry_set_completion(GTK_ENTRY(locationbar.widget), completion);
		g_signal_connect_swapped(completion, "insert-prefix", G_CALLBACK(_interface_tweaks_on_insert_prefix), self);
		g_object_unref(completion);

		/* Add emission handler to entry if it does not exist to ensure that
		 * our "key-press-event" handler gets called first
		 */
		hookID=GPOINTER_TO_SIZE(g_object_get_data(G_OBJECT(locationbar.widget), "interface-tweaks-hook-id"));
		if(hookID==0)
		{
			keyPressSignalID=g_signal_lookup("key-press-event", GTK_TYPE_ENTRY);
			hookID=g_signal_add_emission_hook(keyPressSignalID, 0, _interface_tweaks_on_key_press_event, locationbar.widget, NULL);
			g_object_set_data(G_OBJECT(locationbar.widget), "interface-tweaks-hook-id", GSIZE_TO_POINTER(hookID)) ;
		}

		/* Add signal to recognize widget destruction to release emission hook */
		g_signal_connect_swapped(locationbar.widget, "destroy", G_CALLBACK(_interface_tweaks_on_destroyed), self);
	}
}

/* "group-minimized-tabs" has changed */
static void _interface_tweaks_on_group_minimized_tabs_changed(InterfaceTweaks *self, gboolean inValue)
{
	g_return_if_fail(IS_INTERFACE_TWEAKS(self));

	InterfaceTweaksPrivate	*priv=self->priv;
	GList					*browsers, *browsersIter;
	GList					*tabs, *tabsIter;
	MidoriBrowser			*browser;
	GtkNotebook				*notebook;
	MidoriView				*view;
	gboolean				minimized;

	/* If value changed set it and emit notification of property change */
	if(priv->groupMinimizedTabs!=inValue || !priv->inited)
	{
		/* Change property */
		priv->groupMinimizedTabs=inValue;
		if(priv->inited) midori_extension_set_boolean(priv->extension, "group-minimized-tabs", priv->groupMinimizedTabs);

		/* Move minimized tabs in all browsers to front if activated */
		if(priv->groupMinimizedTabs)
		{
			browsers=midori_app_get_browsers(priv->application);
			for(browsersIter=browsers; browsersIter; browsersIter=g_list_next(browsersIter))
			{
				browser=MIDORI_BROWSER(browsersIter->data);
				if(browser)
				{
					notebook=NULL;
					g_object_get(browser, "notebook", &notebook, NULL);
					if(notebook)
					{
						tabs=midori_browser_get_tabs(MIDORI_BROWSER(browsersIter->data));
						for(tabsIter=tabs; tabsIter; tabsIter=g_list_next(tabsIter))
						{
							view=MIDORI_VIEW(tabsIter->data);
							if(view)
							{
								g_signal_handlers_block_by_func(view, G_CALLBACK(_interface_tweaks_on_notify_minimized_tab_for_group_tabs), self);

								minimized=FALSE;
								g_object_get(view, "minimized", &minimized, NULL);
								if(minimized) gtk_notebook_reorder_child(GTK_NOTEBOOK(notebook), GTK_WIDGET(view), 0);

								g_signal_handlers_unblock_by_func(view, G_CALLBACK(_interface_tweaks_on_notify_minimized_tab_for_group_tabs), self);
							}
						}
						g_list_free(tabs);

						g_object_unref(notebook);
					}
				}
			}
			g_list_free(browsers);
		}

		/* Notify about property change */
		g_object_notify_by_pspec(G_OBJECT(self), InterfaceTweaksProperties[PROP_GROUP_MINIMIZED_TABS]);
	}
}

/* "hide-close-on-minimized-tabs" has changed */
static void _interface_tweaks_on_hide_close_on_minimized_tabs_changed(InterfaceTweaks *self, gboolean inValue)
{
	g_return_if_fail(IS_INTERFACE_TWEAKS(self));

	InterfaceTweaksPrivate	*priv=self->priv;
	GList					*browsers, *browsersIter;
	GList					*tabs, *tabsIter;

	/* If value changed set it and emit notification of property change */
	if(priv->hideCloseOnMinimizedTabs!=inValue || !priv->inited)
	{
		/* Change property */
		priv->hideCloseOnMinimizedTabs=inValue;
		if(priv->inited) midori_extension_set_boolean(priv->extension, "hide-close-on-minimized-tabs", priv->hideCloseOnMinimizedTabs);

		/* Apply new value to all tabs in all browsers */
		browsers=midori_app_get_browsers(priv->application);
		for(browsersIter=browsers; browsersIter; browsersIter=g_list_next(browsersIter))
		{
			tabs=midori_browser_get_tabs(MIDORI_BROWSER(browsersIter->data));
			for(tabsIter=tabs; tabsIter; tabsIter=g_list_next(tabsIter))
			{
				_interface_tweaks_on_notify_minimized_tab_for_close_button(self, NULL, MIDORI_VIEW(tabsIter->data));
			}
			g_list_free(tabs);
		}
		g_list_free(browsers);

		/* Notify about property change */
		g_object_notify_by_pspec(G_OBJECT(self), InterfaceTweaksProperties[PROP_HIDE_CLOSE_ON_MINIMIZED_TABS]);
	}
}

/* "show-start-request-throbber" has changed */
static void _interface_tweaks_on_show_start_request_throbber_changed(InterfaceTweaks *self, gboolean inValue)
{
	g_return_if_fail(IS_INTERFACE_TWEAKS(self));

	InterfaceTweaksPrivate	*priv=self->priv;

	/* If value changed set it and emit notification of property change */
	if(priv->showStartRequestThrobber!=inValue || !priv->inited)
	{
		/* Change property */
		priv->showStartRequestThrobber=inValue;
		if(priv->inited) midori_extension_set_boolean(priv->extension, "show-start-request-throbber", priv->showStartRequestThrobber);

		/* Notify about property change */
		g_object_notify_by_pspec(G_OBJECT(self), InterfaceTweaksProperties[PROP_SHOW_START_REQUEST_THROBBER]);
	}
}

/* "small-toolbar" has changed */
static void _interface_tweaks_on_small_toolbar_changed(InterfaceTweaks *self, gboolean inValue)
{
	g_return_if_fail(IS_INTERFACE_TWEAKS(self));

	InterfaceTweaksPrivate	*priv=self->priv;

	/* If value changed set it and emit notification of property change */
	if(priv->smallToolbar!=inValue || !priv->inited)
	{
		/* Change property */
		priv->smallToolbar=inValue;
		if(priv->inited) midori_extension_set_boolean(priv->extension, "small-toolbar", priv->smallToolbar);

		/* If small toolbar should be displayed remember current settings */
		if(inValue==TRUE)
		{
			/* Get old toolbar values */
			g_object_get(gtk_settings_get_default(),
							"gtk-toolbar-icon-size", &priv->originToolbarIconSize,
							"gtk-toolbar-style", &priv->originToolbarStyle,
							NULL);

			/* Set new toolbar values */
			g_object_set(gtk_settings_get_default(),
							"gtk-toolbar-icon-size", GTK_ICON_SIZE_SMALL_TOOLBAR,
							"gtk-toolbar-style", GTK_TOOLBAR_ICONS,
							NULL);
		}
			/* Otherwise restore old settings */
			else
			{
				if(priv->inited==TRUE)
				{
					/* Set old toolbar values */
					g_object_set(gtk_settings_get_default(),
									"gtk-toolbar-icon-size", priv->originToolbarIconSize,
									"gtk-toolbar-style", priv->originToolbarStyle,
									NULL);
				}
			}

		/* Notify about property change */
		g_object_notify_by_pspec(G_OBJECT(self), InterfaceTweaksProperties[PROP_SMALL_TOOLBAR]);
	}
}

/* "autocomplete-locationbar" has changed */
static void _interface_tweaks_on_autocomplete_locationbar_changed(InterfaceTweaks *self, gboolean inValue)
{
	g_return_if_fail(IS_INTERFACE_TWEAKS(self));

	InterfaceTweaksPrivate	*priv=self->priv;

	/* If value changed set it and emit notification of property change */
	if(priv->autocompleteLocationbar!=inValue || !priv->inited)
	{
		/* Change property */
		priv->autocompleteLocationbar=inValue;
		if(priv->inited) midori_extension_set_boolean(priv->extension, "autocomplete-locationbar", priv->autocompleteLocationbar);

		/* Notify about property change */
		g_object_notify_by_pspec(G_OBJECT(self), InterfaceTweaksProperties[PROP_AUTOCOMPLETE_LOCATIONBAR]);
	}
}

/* Application property has changed */
static void _interface_tweaks_on_application_changed(InterfaceTweaks *self, MidoriApp *inApplication)
{
	g_return_if_fail(IS_INTERFACE_TWEAKS(self));
	g_return_if_fail(inApplication==NULL || MIDORI_IS_APP(inApplication));

	InterfaceTweaksPrivate				*priv=INTERFACE_TWEAKS(self)->priv;
	GtkNotebook							*notebook;
	MidoriBrowser						*browser;
	GList								*browsers, *browsersIter;
	GList								*tabs, *tabsIter;
	WebKitWebView						*webkitView;
	InterfaceTweaksLocationbarLookup	locationbar;

	/* Release resources on current application object */
	if(priv->application)
	{
		g_signal_handlers_disconnect_by_data(priv->application, self);

		browsers=midori_app_get_browsers(priv->application);
		for(browsersIter=browsers; browsersIter; browsersIter=g_list_next(browsersIter))
		{
			browser=MIDORI_BROWSER(browsersIter->data);
			g_signal_handlers_disconnect_by_data(browser, self);

			tabs=midori_browser_get_tabs(MIDORI_BROWSER(browsersIter->data));
			for(tabsIter=tabs; tabsIter; tabsIter=g_list_next(tabsIter))
			{
				g_signal_handlers_disconnect_by_data(tabsIter->data, self);

				webkitView=WEBKIT_WEB_VIEW(midori_view_get_web_view(MIDORI_VIEW(tabsIter->data)));
				g_signal_handlers_disconnect_by_data(webkitView, self);
			}
			g_list_free(tabs);

			notebook=NULL;
			g_object_get(browser, "notebook", &notebook, NULL);
			if(notebook)
			{
				g_signal_handlers_disconnect_by_data(notebook, self);
				g_object_unref(notebook);
			}

			_interface_tweaks_find_browser_locationbar(browser, &locationbar);
			if(locationbar.widget)
			{
				GtkEntry				*entry=GTK_ENTRY(locationbar.widget);
				GtkEntryCompletion		*completion;
				gulong					hookID;
				guint					keyPressSignalID;

				g_signal_handlers_disconnect_by_data(entry, self);

				completion=gtk_entry_get_completion(GTK_ENTRY(entry));
				if(completion)
				{
					g_signal_handlers_disconnect_by_data(completion, self);
					gtk_entry_set_completion(entry, NULL);
				}

				hookID=GPOINTER_TO_SIZE(g_object_get_data(G_OBJECT(entry), "interface-tweaks-hook-id"));
				if(hookID!=0)
				{
					keyPressSignalID=g_signal_lookup("key-press-event", GTK_TYPE_ENTRY);
					g_signal_remove_emission_hook(keyPressSignalID, hookID);

					hookID=0L;
					g_object_set_data(G_OBJECT(entry), "interface-tweaks-hook-id", GSIZE_TO_POINTER(hookID));
				}
			}
		}
		g_list_free(browsers);

		g_object_unref(priv->application);
		priv->application=NULL;
	}

	/* Set new application object */
	if(!inApplication) return;
	priv->application=g_object_ref(inApplication);

	/* Set up all current open browser windows */
	browsers=midori_app_get_browsers(priv->application);
	for(browsersIter=browsers; browsersIter; browsersIter=g_list_next(browsersIter))
	{
		_interface_tweaks_on_add_browser(self, MIDORI_BROWSER(browsersIter->data), priv->application);
	}
	g_list_free(browsers);

	/* Listen to new browser windows opened */
	g_signal_connect_swapped(priv->application, "add-browser", G_CALLBACK(_interface_tweaks_on_add_browser), self);

	/* Notify about property change */
	g_object_notify_by_pspec(G_OBJECT(self), InterfaceTweaksProperties[PROP_APPLICATION]);
}

/* Extension property has changed */
static void _interface_tweaks_on_extension_changed(InterfaceTweaks *self, MidoriExtension *inExtension)
{
	g_return_if_fail(IS_INTERFACE_TWEAKS(self));
	g_return_if_fail(inExtension==NULL || MIDORI_IS_EXTENSION(inExtension));

	InterfaceTweaksPrivate	*priv=INTERFACE_TWEAKS(self)->priv;
	gchar					*dbFilename;
	gint					dbSuccess;
	sqlite3					*dbHandle;
	sqlite3_stmt			*dbStatement;
	gchar					*dbURI;

	/* Release resources on current extension object */
	if(priv->extension)
	{
		g_object_unref(priv->extension);
		priv->extension=NULL;
	}

	/* Set new extension object */
	if(!inExtension) return;
	priv->extension=g_object_ref(inExtension);

	/* Clear completion model and fill model from history database */
	if(priv->completionModel)
	{
		gtk_list_store_clear(priv->completionModel);

		dbFilename=midori_paths_get_config_filename_for_writing(HISTORY_DATABASE_FILENAME);
		if(dbFilename)
		{
			dbSuccess=sqlite3_open_v2(dbFilename, &dbHandle, SQLITE_OPEN_READONLY, NULL);
			if(dbSuccess==SQLITE_OK)
			{
				dbStatement=NULL;
				dbSuccess=sqlite3_prepare_v2(dbHandle,
												"SELECT uri FROM history GROUP BY uri ORDER BY date,uri;", /* Found row should also contain highest date so no ORDER BY */
												-1,
												&dbStatement,
												NULL);
				if(dbStatement && dbSuccess==SQLITE_OK)
				{
					/* Iterate through URIs found in database by the query above */
					while(sqlite3_step(dbStatement)==SQLITE_ROW)
					{
						/* Get URI from database */
						dbURI=(gchar*)sqlite3_column_text(dbStatement, 0);

						/* Add to completion model */
						_interface_tweaks_add_uri_to_completion_model(self, dbURI);
					}
				}
					else g_warning(_("Could not fetch history from database: %s"), sqlite3_errmsg(dbHandle));

				sqlite3_finalize(dbStatement);
			}

			sqlite3_close(dbHandle);
			g_free(dbFilename);
		}
	}

	/* Notify about property change */
	g_object_notify_by_pspec(G_OBJECT(self), InterfaceTweaksProperties[PROP_EXTENSION]);
}

/* IMPLEMENTATION: GObject */

/* Finalize this object */
static void interface_tweaks_finalize(GObject *inObject)
{
	InterfaceTweaks			*self=INTERFACE_TWEAKS(inObject);

	/* Dispose allocated resources */
	_interface_tweaks_on_group_minimized_tabs_changed(self, FALSE);
	_interface_tweaks_on_hide_close_on_minimized_tabs_changed(self, FALSE);
	_interface_tweaks_on_show_start_request_throbber_changed(self, FALSE);
	_interface_tweaks_on_small_toolbar_changed(self, FALSE);
	_interface_tweaks_on_autocomplete_locationbar_changed(self, FALSE);
	_interface_tweaks_on_application_changed(self, NULL);
	_interface_tweaks_on_extension_changed(self, NULL);

	if(self->priv->completionModel)
	{
		g_object_unref(self->priv->completionModel);
		self->priv->completionModel=NULL;
	}

	if(self->priv->completionModelDomains)
	{
		g_hash_table_destroy(self->priv->completionModelDomains);
		self->priv->completionModelDomains=NULL;
	}

	/* Call parent's class finalize method */
	G_OBJECT_CLASS(interface_tweaks_parent_class)->finalize(inObject);
}

/* Set/get properties */
static void interface_tweaks_set_property(GObject *inObject,
													guint inPropID,
													const GValue *inValue,
													GParamSpec *inSpec)
{
	InterfaceTweaks		*self=INTERFACE_TWEAKS(inObject);

	switch(inPropID)
	{
		/* Construct-only properties */
		case PROP_EXTENSION:
			_interface_tweaks_on_extension_changed(self, g_value_get_object(inValue));
			break;

		case PROP_APPLICATION:
			_interface_tweaks_on_application_changed(self, g_value_get_object(inValue));
			break;

		case PROP_GROUP_MINIMIZED_TABS:
			_interface_tweaks_on_group_minimized_tabs_changed(self, g_value_get_boolean(inValue));
			break;

		case PROP_HIDE_CLOSE_ON_MINIMIZED_TABS:
			_interface_tweaks_on_hide_close_on_minimized_tabs_changed(self, g_value_get_boolean(inValue));
			break;

		case PROP_SHOW_START_REQUEST_THROBBER:
			_interface_tweaks_on_show_start_request_throbber_changed(self, g_value_get_boolean(inValue));
			break;

		case PROP_SMALL_TOOLBAR:
			_interface_tweaks_on_small_toolbar_changed(self, g_value_get_boolean(inValue));
			break;

		case PROP_AUTOCOMPLETE_LOCATIONBAR:
			_interface_tweaks_on_autocomplete_locationbar_changed(self, g_value_get_boolean(inValue));
			break;

		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(inObject, inPropID, inSpec);
			break;
	}
}

static void interface_tweaks_get_property(GObject *inObject,
													guint inPropID,
													GValue *outValue,
													GParamSpec *inSpec)
{
	InterfaceTweaks		*self=INTERFACE_TWEAKS(inObject);

	switch(inPropID)
	{
		case PROP_EXTENSION:
			g_value_set_object(outValue, self->priv->extension);
			break;

		case PROP_APPLICATION:
			g_value_set_object(outValue, self->priv->application);
			break;

		case PROP_GROUP_MINIMIZED_TABS:
			g_value_set_boolean(outValue, self->priv->groupMinimizedTabs);
			break;

		case PROP_HIDE_CLOSE_ON_MINIMIZED_TABS:
			g_value_set_boolean(outValue, self->priv->hideCloseOnMinimizedTabs);
			break;

		case PROP_SHOW_START_REQUEST_THROBBER:
			g_value_set_boolean(outValue, self->priv->showStartRequestThrobber);
			break;

		case PROP_SMALL_TOOLBAR:
			g_value_set_boolean(outValue, self->priv->smallToolbar);
			break;

		case PROP_AUTOCOMPLETE_LOCATIONBAR:
			g_value_set_boolean(outValue, self->priv->autocompleteLocationbar);
			break;

		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(inObject, inPropID, inSpec);
			break;
	}
}

/* Class initialization
 * Override functions in parent classes and define properties and signals
 */
static void interface_tweaks_class_init(InterfaceTweaksClass *klass)
{
	GObjectClass		*gobjectClass=G_OBJECT_CLASS(klass);

	/* Override functions */
	gobjectClass->finalize=interface_tweaks_finalize;
	gobjectClass->set_property=interface_tweaks_set_property;
	gobjectClass->get_property=interface_tweaks_get_property;

	/* Set up private structure */
	g_type_class_add_private(klass, sizeof(InterfaceTweaksPrivate));

	/* Define properties */
	InterfaceTweaksProperties[PROP_EXTENSION]=
		g_param_spec_object("extension",
								_("Extension instance"),
								_("The Midori extension instance for this extension"),
								MIDORI_TYPE_EXTENSION,
								G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);

	InterfaceTweaksProperties[PROP_APPLICATION]=
		g_param_spec_object("application",
								_("Application instance"),
								_("The Midori application instance this extension belongs to"),
								MIDORI_TYPE_APP,
								G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);

	InterfaceTweaksProperties[PROP_GROUP_MINIMIZED_TABS]=
		g_param_spec_boolean("group-minimized-tabs",
								_("Group minimized tabs"),
								_("If true this extension groups minimized tabs to front."),
								FALSE,
								G_PARAM_READWRITE | G_PARAM_CONSTRUCT);

	InterfaceTweaksProperties[PROP_HIDE_CLOSE_ON_MINIMIZED_TABS]=
		g_param_spec_boolean("hide-close-on-minimized-tabs",
								_("Hide close on minimized tabs"),
								_("If true this extension hides the close button on minized tabs."),
								FALSE,
								G_PARAM_READWRITE | G_PARAM_CONSTRUCT);

	InterfaceTweaksProperties[PROP_SHOW_START_REQUEST_THROBBER]=
		g_param_spec_boolean("show-start-request-throbber",
								_("Show start-request throbber"),
								_("If true this extension shows a throbber when a request was started "
								  "which will be replaced with Midori's standard throbber as soon as data is loaded."),
								FALSE,
								G_PARAM_READWRITE | G_PARAM_CONSTRUCT);

	InterfaceTweaksProperties[PROP_SMALL_TOOLBAR]=
		g_param_spec_boolean("small-toolbar",
								_("Small toolbar"),
								_("If true this extension will make toolbar smaller in size."),
								FALSE,
								G_PARAM_READWRITE | G_PARAM_CONSTRUCT);

	InterfaceTweaksProperties[PROP_AUTOCOMPLETE_LOCATIONBAR]=
		g_param_spec_boolean("autocomplete-locationbar",
								_("Autocomplete locationbar"),
								_("If true this extension will autocomplete known domains in locationbar."),
								FALSE,
								G_PARAM_READWRITE | G_PARAM_CONSTRUCT);

	g_object_class_install_properties(gobjectClass, PROP_LAST, InterfaceTweaksProperties);
}

/* Object initialization
 * Create private structure and set up default values
 */
static void interface_tweaks_init(InterfaceTweaks *self)
{
	InterfaceTweaksPrivate	*priv;

	priv=self->priv=INTERFACE_TWEAKS_GET_PRIVATE(self);

	/* Set up default values */
	priv->inited=FALSE;

	priv->groupMinimizedTabs=FALSE;
	priv->hideCloseOnMinimizedTabs=FALSE;
	priv->showStartRequestThrobber=FALSE;
	priv->smallToolbar=FALSE;
	priv->autocompleteLocationbar=FALSE;

	priv->originToolbarIconSize=GTK_ICON_SIZE_INVALID;
	priv->originToolbarStyle=GTK_TOOLBAR_BOTH;

	priv->completionModel=gtk_list_store_new(1, G_TYPE_STRING);
	priv->completionModelDomains=g_hash_table_new(g_str_hash, g_str_equal);
}

/* Implementation: Public API */

/* Create new object */
InterfaceTweaks* interface_tweaks_new(MidoriExtension *inExtension, MidoriApp *inApp)
{
	InterfaceTweaks		*object;

	object=g_object_new(TYPE_INTERFACE_TWEAKS,
							"extension", inExtension,
							"application", inApp,
							NULL);
	object->priv->inited=TRUE;

	return(object);
}
