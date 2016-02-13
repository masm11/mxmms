#include <stdio.h>
#include <gtk/gtk.h>
#include <mate-panel-applet.h>
#include <xmmsclient/xmmsclient.h>
#include <xmmsclient/xmmsclient-glib.h>

static struct work_t {
    gint title_x;
    guint size;
    
    MatePanelApplet *applet;
    GtkWidget *button;
    GtkWidget *layout;
    GtkWidget *title;
    GtkWidget *status;
    GtkWidget *playtime;
    
    guint timer;
    
    xmmsc_connection_t *conn;
    
    
} work = {
    0,
    100,
};

static int playback_status_changed(xmmsv_t *val, void *user_data)
{
    struct work_t *w = user_data;
    
    int r;
    if (xmmsv_get_int(val, &r)) {
	switch (r) {
	case XMMS_PLAYBACK_STATUS_STOP:
	    gtk_image_set_from_icon_name(GTK_IMAGE(w->status),
		    "gtk-media-stop", GTK_ICON_SIZE_SMALL_TOOLBAR);
	    break;
	case XMMS_PLAYBACK_STATUS_PLAY:
	    gtk_image_set_from_icon_name(GTK_IMAGE(w->status),
		    "gtk-media-play", GTK_ICON_SIZE_SMALL_TOOLBAR);
	    break;
	case XMMS_PLAYBACK_STATUS_PAUSE:
	    gtk_image_set_from_icon_name(GTK_IMAGE(w->status),
		    "gtk-media-pause", GTK_ICON_SIZE_SMALL_TOOLBAR);
	    break;
	}
	
	// fixme: これじゃうまくいかん。
	guint w1 = gtk_widget_get_allocated_width(w->layout);
	guint w2 = gtk_widget_get_allocated_width(w->status);
	guint h1 = gtk_widget_get_allocated_height(w->layout);
	guint h2 = gtk_widget_get_allocated_height(w->status);
	gtk_layout_move(GTK_LAYOUT(w->layout), w->status, w1 - w2, h1 - h2);
    }
    
    return TRUE;
}

static int playback_current_id_changed(xmmsv_t *val, void *user_data)
{
    struct work_t *w = user_data;
    
    int r;
    if (xmmsv_get_int(val, &r)) {
	// fixme: もちろん仮。
	gchar *buf = g_strdup_printf("%d", r);
	gtk_label_set_text(GTK_LABEL(w->title), buf);
	g_free(buf);
	
	w->title_x = w->size;
	gtk_layout_move(GTK_LAYOUT(w->layout), w->title, w->title_x, 0);
    }
    
    return TRUE;
}

static int playback_playtime_changed(xmmsv_t *val, void *user_data)
{
    struct work_t *w = user_data;
    
    gint64 r;
    if (xmmsv_get_int64(val, &r)) {
	r = r / 1000;
	gchar *buf = g_strdup_printf("%d:%02d", r / 60, r % 60);
	gtk_label_set_text(GTK_LABEL(w->playtime), buf);
	
	guint h1 = gtk_widget_get_allocated_height(w->layout);
	guint h2 = gtk_widget_get_allocated_height(w->playtime);
	gtk_layout_move(GTK_LAYOUT(w->layout), w->playtime, 0, h1 - h2);
	
	g_free(buf);
    }
    
    return TRUE;
}

/****************************************************************/

static gboolean timer(gpointer user_data)
{
    struct work_t *w = user_data;
    
    if (--w->title_x < -gtk_widget_get_allocated_width(w->title))
	w->title_x = w->size;
    
    gtk_layout_move(GTK_LAYOUT(w->layout), w->title, w->title_x, 0);
    
    return G_SOURCE_CONTINUE;
}

static gboolean callback(MatePanelApplet *applet, const gchar *iid, gpointer user_data)
{
    struct work_t *w = &work;
    w->applet = applet;
    
    w->title_x = w->size;
    
    w->button = gtk_button_new();
    gtk_widget_set_size_request(w->button, w->size, 48);
    gtk_widget_show(w->button);
    gtk_container_add(GTK_CONTAINER(w->applet), w->button);
    
    w->layout = gtk_layout_new(NULL, NULL);
    gtk_widget_show(w->layout);
    gtk_container_add(GTK_CONTAINER(w->button), w->layout);
    
    w->title = gtk_label_new("-");
    gtk_widget_show(w->title);
    gtk_layout_put(GTK_LAYOUT(w->layout), w->title, w->title_x, 0);
    
    w->status = gtk_image_new_from_icon_name("gtk-media-stop", GTK_ICON_SIZE_SMALL_TOOLBAR);
    gtk_widget_show(w->status);
    gtk_layout_put(GTK_LAYOUT(w->layout), w->status, w->size, 0);
    
    w->playtime = gtk_label_new("0:00");
    gtk_widget_show(w->playtime);
    gtk_layout_put(GTK_LAYOUT(w->layout), w->playtime, w->size, 0);
    
    gtk_widget_show(GTK_WIDGET(w->applet));
    
    w->timer = g_timeout_add(50, timer, w);
    
    w->conn = xmmsc_init("Mxmms");
    xmmsc_connect(w->conn, NULL);
    xmmsc_mainloop_gmain_init(w->conn);
    
    xmmsc_result_t *res;
    
    res = xmmsc_broadcast_playback_status(w->conn);
    xmmsc_result_notifier_set(res, playback_status_changed, w);
    res = xmmsc_broadcast_playback_current_id(w->conn);
    xmmsc_result_notifier_set(res, playback_current_id_changed, w);
    res = xmmsc_signal_playback_playtime(w->conn);
    xmmsc_result_notifier_set(res, playback_playtime_changed, w);
    
    res = xmmsc_playback_status(w->conn);
    xmmsc_result_notifier_set(res, playback_status_changed, w);
    res = xmmsc_playback_current_id(w->conn);
    xmmsc_result_notifier_set(res, playback_current_id_changed, w);
    res = xmmsc_playback_playtime(w->conn);
    xmmsc_result_notifier_set(res, playback_playtime_changed, w);
    
    return TRUE;
}

MATE_PANEL_APPLET_OUT_PROCESS_FACTORY("MxmmsAppletFactory", PANEL_TYPE_APPLET, NULL, callback, NULL)
