#include <stdio.h>
#include <stdlib.h>
#include <gtk/gtk.h>
#include <mate-panel-applet.h>
#include <xmmsclient/xmmsclient.h>
#include <xmmsclient/xmmsclient-glib.h>

struct music_t {
    guint id;
    gchar *artist;
    gchar *title;
};

static struct work_t {
    gint title_x;
    guint size;
    
    MatePanelApplet *applet;
    GtkWidget *obox;
    GtkWidget *button;
    GtkWidget *vbox, *hbox;
    GtkWidget *layout;
    GtkWidget *title;
    GtkWidget *status;
    GtkWidget *playtime;
    
    guint timer;
    
    xmmsc_connection_t *conn;
    int last_status;
    gchar *last_playlist_name;
    gint last_pos;
    GList *last_playlist;	// struct music_t
    GQueue *playlist_getting_ids;
} work = {
    0,
    100,
};

static void print_indent(int indent);
static void print_dict_entry(const gchar *key, xmmsv_t *value, void *user_data);
static void print_xmmsv(xmmsv_t *val, int indent);

static void print_indent(int indent)
{
    for (gint i = 0; i < indent; i++)
	putchar(' ');
}

static void print_dict_entry(const gchar *key, xmmsv_t *value, void *user_data)
{
    gint indent = (gint) user_data;
    print_indent(indent);
    printf("key: %s\n", key);
    print_indent(indent);
    printf("val: \n");
    print_xmmsv(value, indent + 2);
}

static void print_xmmsv(xmmsv_t *val, int indent)
{
    const gchar *str;
    gint64 n64;
    switch (xmmsv_get_type(val)) {
    case XMMSV_TYPE_NONE:
	print_indent(indent);
	printf("none: \n");
	break;
    case XMMSV_TYPE_ERROR:
	if (xmmsv_get_error(val, &str)) {
	    print_indent(indent);
	    printf("error: %s\n", str);
	} else {
	    print_indent(indent);
	    printf("error: unknown\n");
	}
	break;
    case XMMSV_TYPE_INT64:
	if (xmmsv_get_int64(val, &n64)) {
	    print_indent(indent);
	    printf("int64: %ld\n", n64);
	} else {
	    print_indent(indent);
	    printf("int64: unknown.\n");
	}
	break;
    case XMMSV_TYPE_STRING:
	if (xmmsv_get_string(val, &str)) {
	    print_indent(indent);
	    printf("string: %s\n", str);
	} else {
	    print_indent(indent);
	    printf("string: unknown\n");
	}
	break;
    case XMMSV_TYPE_COLL:
	print_indent(indent);
	printf("coll: \n");
	break;
	
    case XMMSV_TYPE_BIN:
	print_indent(indent);
	printf("bin: \n");
	break;
	
    case XMMSV_TYPE_LIST:
	print_indent(indent);
	printf("list: \n");
	{
	    xmmsv_list_iter_t *it;
	    if (xmmsv_get_list_iter(val, &it)) {
		do {
		    xmmsv_t *v;
		    xmmsv_list_iter_entry(it, &v);
		    print_xmmsv(v, indent + 2);
		} while (xmmsv_list_iter_next(it), xmmsv_list_iter_valid(it));
	    }
	}
	break;
	
    case XMMSV_TYPE_DICT:
	print_indent(indent);
	printf("dict: \n");
	xmmsv_dict_foreach(val, print_dict_entry, (void *)(indent + 2));
	break;
	
    case XMMSV_TYPE_BITBUFFER:
	print_indent(indent);
	printf("bitbuf: \n");
	break;
	
    case XMMSV_TYPE_FLOAT:
	print_indent(indent);
	printf("float: \n");
	break;
    }
}

/****************/

static int playback_status_changed(xmmsv_t *val, void *user_data)
{
    struct work_t *w = user_data;
    
    int r;
    if (xmmsv_get_int(val, &r)) {
	w->last_status = r;
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
	
/*
	// fixme: これじゃうまくいかん。
	guint w1 = gtk_widget_get_allocated_width(w->layout);
	guint w2 = gtk_widget_get_allocated_width(w->status);
	guint h1 = gtk_widget_get_allocated_height(w->layout);
	guint h2 = gtk_widget_get_allocated_height(w->status);
	gtk_layout_move(GTK_LAYOUT(w->layout), w->status, w1 - w2, h1 - h2);
*/
    }
    
    return TRUE;
}

static void getting_string(const char *key, xmmsv_t *val, void *user_data)
{
    const char *p;
    if (xmmsv_get_string(val, &p))
	*(const char **) user_data = p;
}

static void getting_int64(const char *key, xmmsv_t *val, void *user_data)
{
    gint64 n;
    if (xmmsv_get_int64(val, &n))
	*(gint64 *) user_data = n;
}

static int playback_title_got(xmmsv_t *val, void *user_data)
{
    struct work_t *w = user_data;
    
    xmmsv_t *dict;
    char buf[1024];
    const char *artist = NULL, *title = NULL;
    
    /* 面倒な構造してる。*/
    if (xmmsv_dict_get(val, "title", &dict))
	xmmsv_dict_foreach(dict, getting_string, &title);
    if (title == NULL)
	title = "No Title";
    
    if (xmmsv_dict_get(val, "artist", &dict))
	xmmsv_dict_foreach(dict, getting_string, &artist);
    
    if (artist != NULL)
	g_snprintf(buf, sizeof buf, "%s - %s", artist, title);
    else
	g_snprintf(buf, sizeof buf, "%s", title);
    
    gtk_label_set_text(GTK_LABEL(w->title), buf);
    
    w->title_x = w->size;
    gtk_layout_move(GTK_LAYOUT(w->layout), w->title, w->title_x, 0);
    
    return TRUE;
}

static int playback_current_id_changed(xmmsv_t *val, void *user_data)
{
    struct work_t *w = user_data;
    
    int r;
    if (xmmsv_get_int(val, &r)) {
	xmmsc_result_t *res = xmmsc_medialib_get_info(w->conn, r);
	xmmsc_result_notifier_set(res, playback_title_got, w);
	xmmsc_result_unref(res);
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
	
/*
	guint h1 = gtk_widget_get_allocated_height(w->layout);
	guint h2 = gtk_widget_get_allocated_height(w->playtime);
	gtk_layout_move(GTK_LAYOUT(w->layout), w->playtime, 0, h1 - h2);
*/
	
	g_free(buf);
    }
    
    return TRUE;
}

static int playlist_music_changed(xmmsv_t *val, void *user_data)
{
    struct work_t *w = user_data;
    
    printf("music changed.\n");
    printf("type=%d.\n", xmmsv_get_type(val));
    const char *str;
    if (xmmsv_get_error(val, &str)) {
	printf("%s\n", str);
	return TRUE;
    }
    
    gint pos;
    const gchar *name;
    if (xmmsv_dict_entry_get_int(val, "position", &pos)
	    && xmmsv_dict_entry_get_string(val, "name", &name)) {
	printf("%s:%d\n", name, pos);
	
	g_free(w->last_playlist_name);
	w->last_playlist_name = g_strdup(name);
	w->last_pos = pos;
	
	// fixme: 必要なら playlist を取得。
    }
    
    return TRUE;
}

static int playlist_list_got__iter(xmmsv_t *val, void *user_data)
{
    struct work_t *w = user_data;
    printf("iter.\n");
    print_xmmsv(val, 0);
    
    gint64 id = 0;
    const gchar *artist = NULL;
    const gchar *title = NULL;
    
    xmmsv_t *dict;
    if (xmmsv_dict_get(val, "id", &dict))
	xmmsv_dict_foreach(dict, getting_int64, &id);
    if (xmmsv_dict_get(val, "title", &dict))
	xmmsv_dict_foreach(dict, getting_string, &title);
    if (xmmsv_dict_get(val, "artist", &dict))
	xmmsv_dict_foreach(dict, getting_string, &artist);
    
    struct music_t *mp = g_new0(struct music_t, 1);
    mp->id = id;
    mp->artist = g_strdup(artist);
    mp->title = g_strdup(title);
    
    w->last_playlist = g_list_append(w->last_playlist, mp);
    
    if (g_queue_get_length(w->playlist_getting_ids) == 0) {
	// 終了。
	// とりあえず出力してみる。
	GList *p;
	for (p = w->last_playlist; p != NULL; p = p->next) {
	    struct music_t *mp = p->data;
	    printf("%d: %s - %s\n", mp->id, mp->artist, mp->title);
	}
    } else {
	gint id = g_queue_pop_head(w->playlist_getting_ids);
	xmmsc_result_t *res = xmmsc_medialib_get_info(w->conn, id);
	xmmsc_result_notifier_set(res, playlist_list_got__iter, w);
	xmmsc_result_unref(res);
    }
}

static int playlist_list_got(xmmsv_t *val, void *user_data)
{
    struct work_t *w = user_data;
    
    printf("playlist got.\n");
    print_xmmsv(val, 0);
    
    xmmsv_list_iter_t *it;
    if (xmmsv_get_list_iter(val, &it)) {
	do {
	    xmmsv_t *v;
	    xmmsv_list_iter_entry(it, &v);
	    gint id;
	    if (xmmsv_get_int(v, &id))
		g_queue_push_tail(w->playlist_getting_ids, id);
	} while (xmmsv_list_iter_next(it), xmmsv_list_iter_valid(it));
    }
    
    printf("ids got! %p.\n", w->playlist_getting_ids);
    GList *playlist = NULL;
    
    if (g_queue_get_length(w->playlist_getting_ids) == 0) {
	// There is no music.
	w->last_playlist = NULL;
	return;
    }
    
    gint id = g_queue_pop_head(w->playlist_getting_ids);
    printf("1st id: %d.\n", id);
    xmmsc_result_t *res = xmmsc_medialib_get_info(w->conn, id);
    xmmsc_result_notifier_set(res, playlist_list_got__iter, w);
    xmmsc_result_unref(res);
    printf("1st end.\n");
    
    return TRUE;
}

static int playlist_list_loaded(xmmsv_t *val, void *user_data)
{
    struct work_t *w = user_data;
    
    printf("playlist loaded.\n");
    
    const gchar *r;
    if (xmmsv_get_string(val, &r)) {
	g_free(w->last_playlist_name);
	w->last_playlist_name = g_strdup(r);
	
	xmmsc_result_t *res;
	res = xmmsc_playlist_list_entries(w->conn, w->last_playlist_name);
	xmmsc_result_notifier_set(res, playlist_list_got, w);
	xmmsc_result_unref(res);
    }
    
    return TRUE;
}

/****************************************************************/

static void clicked(GtkButton *button, gpointer user_data)
{
    struct work_t *w = user_data;
    xmmsc_result_t *res;
    
    switch (w->last_status) {
    case XMMS_PLAYBACK_STATUS_PLAY:
	res = xmmsc_playback_pause(w->conn);
	xmmsc_result_unref(res);
	break;
	
    case XMMS_PLAYBACK_STATUS_PAUSE:
    case XMMS_PLAYBACK_STATUS_STOP:
	res = xmmsc_playback_start(w->conn);
	xmmsc_result_unref(res);
	break;
    }
}

static gboolean timer(gpointer user_data)
{
    struct work_t *w = user_data;
    
    if (--w->title_x < -gtk_widget_get_allocated_width(w->title))
	w->title_x = w->size;
    
    gtk_layout_move(GTK_LAYOUT(w->layout), w->title, w->title_x, 0);
    
    return G_SOURCE_CONTINUE;
}

static void about(GtkAction *action, gpointer data)
{
    gtk_show_about_dialog(NULL,
	    "program-name", "Mxmms",
	    "comments", "Masm's XMMS2 Client With a Large Button.",
	    "copyright", "Copyright © 2016 Yuuki Harano",
	    "license-type", GTK_LICENSE_GPL_3_0,
	    "version", "0.1.0",
	    NULL);
}

static void setup_menu(struct work_t *w)
{
    GSList *group = NULL;
    
    
    GtkRadioAction *action = NULL;
    
    
    
    
}

static gboolean callback(MatePanelApplet *applet, const gchar *iid, gpointer user_data)
{
    struct work_t *w = &work;
    w->applet = applet;
    
    static const char *xml =
	    "<menu name=\"Playlist\" action=\"ShowMxmmsPlaylist\">"
	    "<menuitem name=\"About2\" action=\"ShowMxmmsAbout\"/>"
	    "</menu>"
	    "<menuitem name=\"About1\" action=\"ShowMxmmsAbout\" />";
    
    static const GtkActionEntry actions[] = {
	{ "ShowMxmmsPlaylist", NULL, "Playlist", NULL, NULL, NULL },
	{ "ShowMxmmsAbout", GTK_STOCK_ABOUT, "About", NULL, NULL, G_CALLBACK(about) },
	// { "ShowMxmmsHelp", GTK_STOCK_HELP, "Help", NULL, NULL, G_CALLBACK(help) },
    };
    
    GtkActionGroup *agrp;
    
    agrp = gtk_action_group_new("Mxmms Applet Actions");
    gtk_action_group_add_actions(agrp, actions, sizeof actions / sizeof actions[0], NULL);
    
    mate_panel_applet_setup_menu(MATE_PANEL_APPLET(applet), xml, agrp);
    
    w->title_x = w->size;
    
    // fixme: 入り切らないみたい… 後で調整しよう。
    
    w->obox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_widget_show(w->obox);
    gtk_container_add(GTK_CONTAINER(w->applet), w->obox);
    
    w->button = gtk_button_new();
    gtk_widget_set_size_request(w->button, w->size, 56);
    gtk_widget_show(w->button);
    gtk_box_pack_start(GTK_BOX(w->obox), w->button, TRUE, TRUE, 0);
    g_signal_connect(w->button, "clicked", G_CALLBACK(clicked), w);
    
    w->vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_widget_show(w->vbox);
    gtk_container_add(GTK_CONTAINER(w->button), w->vbox);
    
    w->layout = gtk_layout_new(NULL, NULL);
    gtk_widget_show(w->layout);
    gtk_box_pack_start(GTK_BOX(w->vbox), w->layout, TRUE, TRUE, 0);
    
    w->title = gtk_label_new("");
    gtk_widget_show(w->title);
    gtk_layout_put(GTK_LAYOUT(w->layout), w->title, 0, 0);
    
    w->hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_widget_show(w->hbox);
    gtk_box_pack_end(GTK_BOX(w->vbox), w->hbox, FALSE, FALSE, 0);
    
    w->playtime = gtk_label_new("");
    gtk_widget_show(w->playtime);
    gtk_box_pack_start(GTK_BOX(w->hbox), w->playtime, FALSE, FALSE, 0);
    
    w->status = gtk_image_new_from_icon_name("gtk-media-stop", GTK_ICON_SIZE_SMALL_TOOLBAR);
    gtk_widget_show(w->status);
    gtk_box_pack_end(GTK_BOX(w->hbox), w->status, FALSE, FALSE, 0);
    
    gtk_widget_show(GTK_WIDGET(w->applet));
    
    w->timer = g_timeout_add(50, timer, w);
    w->playlist_getting_ids = g_queue_new();
    
    w->conn = xmmsc_init("Mxmms");
    if (!xmmsc_connect(w->conn, NULL)) {
	system("xmms2-launcher");
	xmmsc_connect(w->conn, NULL);
    }
    xmmsc_mainloop_gmain_init(w->conn);
    
    xmmsc_result_t *res;
    
    res = xmmsc_broadcast_playback_status(w->conn);
    xmmsc_result_notifier_set(res, playback_status_changed, w);
    xmmsc_result_unref(res);
    res = xmmsc_broadcast_playback_current_id(w->conn);
    xmmsc_result_notifier_set(res, playback_current_id_changed, w);
    xmmsc_result_unref(res);
    res = xmmsc_signal_playback_playtime(w->conn);
    xmmsc_result_notifier_set(res, playback_playtime_changed, w);
    xmmsc_result_unref(res);
    res = xmmsc_broadcast_playlist_current_pos(w->conn);
    xmmsc_result_notifier_set(res, playlist_music_changed, w);
    xmmsc_result_unref(res);
    
    res = xmmsc_playback_status(w->conn);
    xmmsc_result_notifier_set(res, playback_status_changed, w);
    xmmsc_result_unref(res);
    res = xmmsc_playback_current_id(w->conn);
    xmmsc_result_notifier_set(res, playback_current_id_changed, w);
    xmmsc_result_unref(res);
    res = xmmsc_playback_playtime(w->conn);
    xmmsc_result_notifier_set(res, playback_playtime_changed, w);
    xmmsc_result_unref(res);
    res = xmmsc_playlist_current_active(w->conn);
    xmmsc_result_notifier_set(res, playlist_list_loaded, w);
    xmmsc_result_unref(res);
    
    return TRUE;
}

MATE_PANEL_APPLET_OUT_PROCESS_FACTORY("MxmmsAppletFactory", PANEL_TYPE_APPLET, NULL, callback, NULL)
