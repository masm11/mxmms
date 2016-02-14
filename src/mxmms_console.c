#include "./mxmms_console.h"
#include <string.h>

#define P_(s) s

struct _MxmmsConsolePrivate {
    GtkWidget *layout;
    GtkWidget *title;
    GtkWidget *status;
    GtkWidget *playtime;
    
    gchar *title_str;
    xmms_playback_status_t pbstatus;
    gint64 playtime_msec;
    
    gint title_x;
    guint timer;
};

enum {
    PROP_0,
    PROP_TITLE,
    PROP_STATUS,
    PROP_PLAYTIME,
    NUM_PROPERTIES
};

static GParamSpec *console_props[NUM_PROPERTIES] = { NULL, };

static void mxmms_console_set_property    (GObject          *object,
					   guint             prop_id,
					   const GValue     *value,
					   GParamSpec       *pspec);
static void mxmms_console_get_property    (GObject          *object,
					   guint             prop_id,
					   GValue           *value,
					   GParamSpec       *pspec);
static void mxmms_console_finalize        (GObject          *object);
static void mxmms_console_destroy         (GtkWidget        *widget);
static void mxmms_console_size_allocate   (GtkWidget        *widget,
					   GtkAllocation    *allocation);
static gboolean timer(gpointer user_data)
{
}

G_GNUC_BEGIN_IGNORE_DEPRECATIONS
G_DEFINE_TYPE_WITH_CODE(MxmmsConsole, mxmms_console, GTK_TYPE_BUTTON,
			G_ADD_PRIVATE(MxmmsConsole))
G_GNUC_END_IGNORE_DEPRECATIONS

static void mxmms_console_class_init(MxmmsConsoleClass *class)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS(class);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS(class);
  GtkBindingSet *binding_set;

  gobject_class->set_property = mxmms_console_set_property;
  gobject_class->get_property = mxmms_console_get_property;
  gobject_class->finalize = mxmms_console_finalize;

  widget_class->destroy = mxmms_console_destroy;
  widget_class->size_allocate = mxmms_console_size_allocate;
  //widget_class->state_flags_changed = mxmms_console_state_flags_changed;
  //widget_class->style_updated = mxmms_console_style_updated;
  //widget_class->query_tooltip = mxmms_console_query_tooltip;
  //widget_class->draw = mxmms_console_draw;
  //widget_class->realize = mxmms_console_realize;
  //widget_class->unrealize = mxmms_console_unrealize;
  //widget_class->map = mxmms_console_map;
  //widget_class->unmap = mxmms_console_unmap;
  //widget_class->motion_notify_event = mxmms_console_motion;
  //widget_class->leave_notify_event = mxmms_console_leave_notify;
  //widget_class->hierarchy_changed = mxmms_console_hierarchy_changed;
  //widget_class->screen_changed = mxmms_console_screen_changed;
  //widget_class->mnemonic_activate = mxmms_console_mnemonic_activate;
  //widget_class->drag_data_get = mxmms_console_drag_data_get;
  //widget_class->grab_focus = mxmms_console_grab_focus;
  //widget_class->popup_menu = mxmms_console_popup_menu;
  //widget_class->focus = mxmms_console_focus;
  //widget_class->get_request_mode = mxmms_console_get_request_mode;
  //widget_class->get_preferred_width = mxmms_console_get_preferred_width;
  //widget_class->get_preferred_height = mxmms_console_get_preferred_height;
  //widget_class->get_preferred_width_for_height = mxmms_console_get_preferred_width_for_height;
  //widget_class->get_preferred_height_for_width = mxmms_console_get_preferred_height_for_width;
  //widget_class->get_preferred_height_and_baseline_for_width = mxmms_console_get_preferred_height_and_baseline_for_width;

  //class->move_cursor = mxmms_console_move_cursor;
  //class->copy_clipboard = mxmms_console_copy_clipboard;
  //class->activate_link = mxmms_console_activate_link;

  /**
   * MxmmsConsole:title:
   *
   * The title of the current music.
   */
  console_props[PROP_TITLE] =
      g_param_spec_string ("title",
                           P_("Title"),
                           P_("The title of the current music"),
                           "-",
                           G_PARAM_READWRITE);

  console_props[PROP_STATUS] =
      g_param_spec_int ("status",
                         P_("Status"),
                         P_("Playback status"),
                         XMMS_PLAYBACK_STATUS_STOP,
                         XMMS_PLAYBACK_STATUS_PAUSE,
                         XMMS_PLAYBACK_STATUS_STOP,
                         G_PARAM_READWRITE);

  console_props[PROP_PLAYTIME] =
      g_param_spec_uint64 ("playtime",
                            P_("Playtime"),
                            P_("Current playtime in milliseconds"),
                            0, 100 * 60 * 1000 - 1, 0,
                            G_PARAM_READWRITE);

  g_object_class_install_properties (gobject_class, NUM_PROPERTIES, console_props);
}

static void 
mxmms_console_set_property (GObject      *object,
			    guint         prop_id,
			    const GValue *value,
			    GParamSpec   *pspec)
{
    MxmmsConsole *cons = MXMMS_CONSOLE(object);
    
    switch (prop_id) {
    case PROP_TITLE:
	mxmms_console_set_title(cons, g_value_get_string(value));
	break;
    case PROP_STATUS:
	mxmms_console_set_status(cons, g_value_get_int(value));
	break;
    case PROP_PLAYTIME:
	mxmms_console_set_playtime(cons, g_value_get_int64(value));
	break;
    default:
	G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
	break;
    }
}

static void mxmms_console_get_property (GObject     *object,
					guint        prop_id,
					GValue      *value,
					GParamSpec  *pspec)
{
    MxmmsConsole *cons = MXMMS_CONSOLE(object);
    MxmmsConsolePrivate *priv = cons->priv;
    
    switch(prop_id) {
    case PROP_TITLE:
	g_value_set_string(value, priv->title_str);
	break;
    case PROP_STATUS:
	g_value_set_int(value, priv->pbstatus);
	break;
    case PROP_PLAYTIME:
	g_value_set_int64(value, priv->playtime_msec);
      break;
    default:
	G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
	break;
    }
}

static void mxmms_console_init(MxmmsConsole *cons)
{
    MxmmsConsolePrivate *priv = mxmms_console_get_instance_private(cons);
    cons->priv = priv;
    
    priv->layout = gtk_layout_new(NULL, NULL);
    gtk_container_add(GTK_CONTAINER(cons), priv->layout);
    gtk_widget_show(priv->layout);
    
    priv->title = gtk_label_new("");
    gtk_layout_put(GTK_LAYOUT(priv->layout), priv->title, 0, 0);
    gtk_widget_show(priv->title);
    
    priv->status = gtk_image_new_from_icon_name("gtk-media-stop", GTK_ICON_SIZE_SMALL_TOOLBAR);
    gtk_layout_put(GTK_LAYOUT(priv->layout), priv->status, 0, 0);
    gtk_widget_show(priv->status);
    
    priv->playtime = gtk_label_new("");
    gtk_layout_put(GTK_LAYOUT(priv->layout), priv->playtime, 0, 0);
    gtk_widget_show(priv->playtime);
    
    priv->title_str = NULL;
    priv->pbstatus = 0;
    priv->playtime_msec = 0;
    
    priv->title_x = 0;
    priv->timer = g_timeout_add(50, timer, cons);
}

static void mxmms_console_finalize(GObject *object)
{
    MxmmsConsole *cons = MXMMS_CONSOLE(object);
    MxmmsConsolePrivate *priv = cons->priv;
    
    g_source_remove(priv->timer);
    g_free(priv->title_str);
    
    G_OBJECT_CLASS(mxmms_console_parent_class)->finalize(object);
}

static void mxmms_console_destroy(GtkWidget *widget)
{
    MxmmsConsole *cons = MXMMS_CONSOLE(widget);
    MxmmsConsolePrivate *priv = cons->priv;
    
    gtk_widget_destroy(priv->layout);
    priv->layout = NULL;
    priv->title = NULL;
    priv->status = NULL;
    priv->playtime = NULL;
    
    GTK_WIDGET_CLASS(mxmms_console_parent_class)->destroy(widget);
}

static void mxmms_console_size_allocate   (GtkWidget        *widget,
					   GtkAllocation    *allocation)
{
    GTK_WIDGET_CLASS(mxmms_console_parent_class)->size_allocate(widget, allocation);
    
    // fixme: 配置。
}

void mxmms_console_set_title(MxmmsConsole *cons, const gchar *title)
{
    MxmmsConsolePrivate *priv = cons->priv;
    
    if (priv->title_str == title)
	return;
    
    if (priv->title_str == NULL || title == NULL ||
	    strcmp(priv->title_str, title) != 0) {
	if (priv->title_str != NULL)
	    g_free(priv->title_str);
	priv->title_str = g_strdup(title);
	
	gtk_label_set_text(GTK_LABEL(priv->title), priv->title_str);
    }
}

void mxmms_console_set_status(MxmmsConsole *cons, xmms_playback_status_t status)
{
    MxmmsConsolePrivate *priv = cons->priv;
    
    if (priv->pbstatus == status)
	return;
    
    switch (status) {
    case XMMS_PLAYBACK_STATUS_STOP:
	gtk_image_set_from_icon_name(GTK_IMAGE(priv->status),
		"gtk-media-stop", GTK_ICON_SIZE_SMALL_TOOLBAR);
	break;
    case XMMS_PLAYBACK_STATUS_PLAY:
	gtk_image_set_from_icon_name(GTK_IMAGE(priv->status),
		"gtk-media-play", GTK_ICON_SIZE_SMALL_TOOLBAR);
	break;
    case XMMS_PLAYBACK_STATUS_PAUSE:
	gtk_image_set_from_icon_name(GTK_IMAGE(priv->status),
		"gtk-media-pause", GTK_ICON_SIZE_SMALL_TOOLBAR);
	break;
    }
}

void mxmms_console_set_playtime(MxmmsConsole *cons, gint64 msec)
{
    MxmmsConsolePrivate *priv = cons->priv;
    
    if (priv->playtime_msec == msec)
	return;
    
    gint sec = msec / 1000;
    gchar buf[16];
    g_snprintf(buf, sizeof buf, "%d:%02d", sec / 60, sec % 60);
    gtk_label_set_text(GTK_LABEL(priv->playtime), buf);
}
