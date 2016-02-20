#include <stdio.h>
#include <stdlib.h>
#include <gtk/gtk.h>
#include <mate-panel-applet.h>
#include <xmmsclient/xmmsclient.h>
#include <xmmsclient/xmmsclient-glib.h>
#include "common.h"

static struct work_t {
    gint title_x;
    guint size;
    
    MatePanelApplet *applet;
    GtkWidget *button;
    GtkWidget *layout;
    GtkWidget *title;
    GtkWidget *status;
    GtkWidget *playtime;
    
    GtkActionGroup *agrp;
    
    guint timer;
    
    xmmsc_connection_t *conn;	// xmms2d への接続
    int last_status;		// 現在の status。play/pause/stop
    gchar *last_playlist_name;	// 現在の playlist の名前
    GList *last_playlist;	// 現在の playlist の内容。struct music_t
    gint64 last_playlist_stamp;	// 現在の playlist を取得開始した時刻
    gint last_pos;		// playlist 中、現在再生中の曲番号 0..
} work = {
    0,
    100,
};

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
	
	g_free(buf);
    }
    
    return TRUE;
}

static void playlist_renew(GList *list, gint64 stamp, void *user_data)
{
    struct work_t *w = user_data;
    if (w->last_playlist_stamp > stamp) {
	// 古いのが来た。
	return;
    }
    
    GList *old_list = w->last_playlist;
    w->last_playlist_stamp = stamp;
    w->last_playlist = list;	// リストそのものをもらう。
    
    // 古いリストを解放
    void free_music(gpointer data) {
	struct music_t *mp = data;
	g_free(mp->artist);
	g_free(mp->title);
	g_free(mp);
    }
    g_list_free_full(old_list, free_music);
    
    // とりあえず出力してみる。
    GList *p;
    for (p = w->last_playlist; p != NULL; p = p->next) {
	struct music_t *mp = p->data;
	printf("%d: %s - %s\n", mp->id, mp->artist, mp->title);
    }
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
	
	gboolean load_need = (strcmp(w->last_playlist_name, name) != 0);
	
	g_free(w->last_playlist_name);
	w->last_playlist_name = g_strdup(name);
	w->last_pos = pos;
	
	/* 現在のプレイリストと、そのプレイリスト中での現在の位置が得られる。
	 * プレイリストを切り替えると、プレイリストは切り替わるが、
	 * 再生中の曲はそのまま。次に再生する曲から新しいプレイリストになる。
	 * それまでの間、プレイリスト名は新しいもの、pos は -1 になるみたい。
	 * …そうとも限らないみたいだが、プレイリスト名は新しいものには
	 * なるようだ。
	 */
	if (load_need) {
	    /* 要らないと思うけど、
	     * せっかくどの playlist かまで返答をくれてるので、
	     * 一応違ったら取り寄せる、ってことで。
	     */
	    playlist_get(w->conn, w->last_playlist_name, playlist_renew, w);
	}
    }
    
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
	
	/* 現在の曲番号を得るにはプレイリスト名を渡さないといけない。
	 * 理由が解らないが、そうすると、ここで得るのが良いのかな。
	 */
	xmmsc_result_t *res;
	res = xmmsc_playlist_current_pos(w->conn, w->last_playlist_name);
	xmmsc_result_notifier_set(res, playlist_music_changed, w);
	xmmsc_result_unref(res);
	
	playlist_get(w->conn, w->last_playlist_name, playlist_renew, w);
    }
    
    return TRUE;
}

/****************************************************************/

static gboolean playlist_play(xmmsv_t *val, void *user_data)
{
    struct work_t *w = user_data;
    
    xmmsc_result_t *res;
    res = xmmsc_playback_tickle(w->conn);
    xmmsc_result_unref(res);
    
    return TRUE;
}

static void menu_next(GtkWidget *ww, gpointer user_data)
{
    struct work_t *w = user_data;
    gint  pos;
    
    pos = w->last_pos;
    if (++pos >= g_list_length(w->last_playlist))
	pos = 0;
    
    xmmsc_result_t *res;
    res = xmmsc_playlist_set_next(w->conn, pos);
    xmmsc_result_notifier_set(res, playlist_play, w);
    xmmsc_result_unref(res);
}

static void menu_prev(GtkWidget *ww, gpointer user_data)
{
    struct work_t *w = user_data;
    gint  pos;
    
    // fixme: 3秒ルール適用。
    pos = w->last_pos;
    if (--pos < 0)
	pos = g_list_length(w->last_playlist) - 1;
    
    xmmsc_result_t *res;
    res = xmmsc_playlist_set_next(w->conn, pos);
    xmmsc_result_notifier_set(res, playlist_play, w);
    xmmsc_result_unref(res);
}

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
	w->title_x = gtk_widget_get_allocated_width(w->layout);;
    
    GtkLayout *layout = GTK_LAYOUT(w->layout);
    
    gtk_layout_move(layout, w->title, w->title_x, 0);
    
    gtk_layout_move(layout, w->playtime,
	    0,
	    gtk_widget_get_allocated_height(w->layout) - gtk_widget_get_allocated_height(w->playtime));
    
    gtk_layout_move(layout, w->status,
	    gtk_widget_get_allocated_width(w->layout) - gtk_widget_get_allocated_width(w->status),
	    gtk_widget_get_allocated_height(w->layout) - gtk_widget_get_allocated_height(w->status));
    
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

static gboolean callback(MatePanelApplet *applet, const gchar *iid, gpointer user_data)
{
    struct work_t *w = &work;
    w->applet = applet;
    
    static const char *xml =
	    "<menuitem name=\"Seek\" action=\"MxmmsSeek\"/>"
	    "<menuitem name=\"Next\" action=\"MxmmsNext\"/>"
	    "<menuitem name=\"Previous\" action=\"MxmmsPrevious\"/>"
	    "<menuitem name=\"Musics\" action=\"MxmmsShowMusiclist\"/>"
	    "<menuitem name=\"Playlists\" action=\"MxmmsShowPlaylists\"/>"
	    "<separator/>"
	    "<menuitem name=\"About\" action=\"MxmmsShowAbout\" />";
    
    static const GtkActionEntry actions[] = {
	{ "MxmmsSeek",          NULL, "Seek",       NULL, NULL, NULL },
	{ "MxmmsNext",          NULL, "Next",       NULL, NULL, G_CALLBACK(menu_next) },
	{ "MxmmsPrevious",      NULL, "Previous",   NULL, NULL, G_CALLBACK(menu_prev) },
	{ "MxmmsShowMusiclist", NULL, "Playlist",  NULL, NULL, NULL },
	{ "MxmmsShowPlaylists", NULL, "Playlists",  NULL, NULL, NULL },
	{ "MxmmsShowAbout", GTK_STOCK_ABOUT, "About", NULL, NULL, G_CALLBACK(about) },
    };
    
    w->agrp = gtk_action_group_new("Mxmms Applet Actions");
    gtk_action_group_add_actions(w->agrp, actions, sizeof actions / sizeof actions[0], w);
    
    mate_panel_applet_setup_menu(MATE_PANEL_APPLET(applet), xml, w->agrp);
    
    w->title_x = w->size;
    
    w->button = gtk_button_new();
    gtk_widget_set_size_request(w->button, w->size, 48);
    gtk_container_add(GTK_CONTAINER(applet), w->button);
    gtk_container_set_border_width(GTK_CONTAINER(w->button), 0);
    gtk_widget_show(w->button);
    g_signal_connect(w->button, "clicked", G_CALLBACK(clicked), w);
    
    w->layout = gtk_layout_new(NULL, NULL);
    gtk_container_add(GTK_CONTAINER(w->button), w->layout);
    gtk_widget_show(w->layout);
    
    w->title = gtk_label_new("");
    gtk_layout_put(GTK_LAYOUT(w->layout), w->title, 0, 0);
    gtk_widget_show(w->title);
    
    w->playtime = gtk_label_new("");
    gtk_layout_put(GTK_LAYOUT(w->layout), w->playtime, 0, 0);
    gtk_widget_show(w->playtime);
    
    w->status = gtk_image_new_from_icon_name("gtk-media-stop", GTK_ICON_SIZE_SMALL_TOOLBAR);
    gtk_layout_put(GTK_LAYOUT(w->layout), w->status, 0, 0);
    gtk_widget_show(w->status);
    
    gtk_widget_show(GTK_WIDGET(w->applet));
    
    w->timer = g_timeout_add(50, timer, w);
    w->last_playlist_name = g_strdup("");
    
    w->conn = xmmsc_init("Mxmms");
    if (!xmmsc_connect(w->conn, NULL)) {
	system("xmms2-launcher");
	xmmsc_connect(w->conn, NULL);
    }
    xmmsc_mainloop_gmain_init(w->conn);
    
    xmmsc_result_t *res;
    
    // play/pause/stop の状態が変化
    res = xmmsc_broadcast_playback_status(w->conn);
    xmmsc_result_notifier_set(res, playback_status_changed, w);
    xmmsc_result_unref(res);
    // 再生中の曲 ID が変わった
    res = xmmsc_broadcast_playback_current_id(w->conn);
    xmmsc_result_notifier_set(res, playback_current_id_changed, w);
    xmmsc_result_unref(res);
    // playlist 中の番号が変わった
    res = xmmsc_broadcast_playlist_current_pos(w->conn);
    xmmsc_result_notifier_set(res, playlist_music_changed, w);
    xmmsc_result_unref(res);
    // playlist が切り替わった
    res = xmmsc_broadcast_playlist_loaded(w->conn);
    xmmsc_result_notifier_set(res, playlist_list_loaded, w);
    xmmsc_result_unref(res);
    // 再生時間が変わった
    res = xmmsc_signal_playback_playtime(w->conn);
    xmmsc_result_notifier_set(res, playback_playtime_changed, w);
    xmmsc_result_unref(res);
    
    // ここから先は、現在の状態の取得
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
