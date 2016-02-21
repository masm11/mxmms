#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <gtk/gtk.h>
#include <mate-panel-applet.h>
#include <xmmsclient/xmmsclient.h>
#include <xmmsclient/xmmsclient-glib.h>
#include "common.h"

enum {
    COL_NOW,
    COL_NUM,
    COL_NO,
    COL_XALIGN,
    COL_TITLE,
    COL_PTR,
    COL_NR
};

enum {
    COL0_NOW,
    COL0_NAME,
    COL0_NR
};

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
    guint timer_pl;
    
    xmmsc_connection_t *conn;	// xmms2d への接続
    int last_status;		// 現在の status。play/pause/stop
    gchar *last_playlist_name;	// 現在の playlist の名前
    GList *last_playlist;	// 現在の playlist の内容。struct music_t
    gint64 last_playlist_stamp;	// 現在の playlist を取得開始した時刻
    gint last_pos;		// playlist 中、現在再生中の曲番号 0..
    gint64 last_playtime;	// 現在の再生時間(msec)
    gint64 last_duration;	// 現在の曲の長さ(msec)
    GList *last_playlists;	// 現在の playlist の list
    gint64 last_playlists_stamp;// 現在の playlist の list を取得開始した時刻
    
    GtkListStore *title_store;
    GtkListStore *plist_store;
    
    GtkAdjustment *seekbar_adj;
};

static void title_store_renew(struct work_t *w);
static void title_store_update_now(struct work_t *w);
static void plist_store_renew(struct work_t *w);
static void plist_store_update_now(struct work_t *w);

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
    
    if (xmmsv_dict_get(val, "duration", &dict)) {
	xmmsv_dict_foreach(dict, getting_int64, &w->last_duration);
	gtk_adjustment_set_upper(w->seekbar_adj, w->last_duration);
    }
    
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
	w->last_playtime = r;
	r = r / 1000;
	gchar *buf = g_strdup_printf("%d:%02d", r / 60, r % 60);
	gtk_label_set_text(GTK_LABEL(w->playtime), buf);
	
	g_free(buf);
	
	gtk_adjustment_set_value(w->seekbar_adj, w->last_playtime);
    }
    
    return TRUE;
}

static void playlist_renew(GList *list, gint64 stamp, void *user_data)
{
    void free_music(gpointer data) {
	struct music_t *mp = data;
	g_free(mp->artist);
	g_free(mp->title);
	g_free(mp);
    }
    
    struct work_t *w = user_data;
    if (w->last_playlist_stamp > stamp) {
	// 古いのが来た。
	g_list_free_full(list, free_music);
	return;
    }
    
    GList *old_list = w->last_playlist;
    w->last_playlist_stamp = stamp;
    w->last_playlist = list;	// リストそのものをもらう。
    
    // 古いリストを解放
    g_list_free_full(old_list, free_music);
    
    // とりあえず出力してみる。
    GList *p;
    for (p = w->last_playlist; p != NULL; p = p->next) {
	struct music_t *mp = p->data;
	printf("%d: %s - %s\n", mp->id, mp->artist, mp->title);
    }
    
    title_store_renew(w);
    title_store_update_now(w);
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
	title_store_update_now(w);
	plist_store_update_now(w);
	
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
	
	plist_store_update_now(w);
	
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

/* title store を作り直す。*/
static void title_store_renew(struct work_t *w)
{
    while (TRUE) {
	GtkTreeIter iter;
	if (!gtk_tree_model_get_iter_first(GTK_TREE_MODEL(w->title_store), &iter))
	    break;
	gtk_list_store_remove(w->title_store, &iter);
    }
    
    gint num = 0;
    for (GList *lp = w->last_playlist; lp != NULL; lp = g_list_next(lp)) {
	GtkTreeIter iter;
	gtk_list_store_append(w->title_store, &iter);
	gtk_list_store_set(w->title_store, &iter,
		COL_NOW, FALSE,
		COL_NUM, num,
		COL_NO, num + 1,
		COL_XALIGN, 1.0,
		COL_TITLE, ((struct music_t *) lp->data)->title,
		COL_PTR, lp->data,
		-1);
	num++;
    }
}

/* radio button を更新する */
static void title_store_update_now(struct work_t *w)
{
    GtkTreeIter iter;
    if (!gtk_tree_model_get_iter_first(GTK_TREE_MODEL(w->title_store), &iter))
	return;
    do {
	gint num;
	gtk_tree_model_get(GTK_TREE_MODEL(w->title_store), &iter,
		COL_NUM, &num, -1);
	gtk_list_store_set(w->title_store, &iter,
		COL_NOW, (num == w->last_pos), -1);
    } while (gtk_tree_model_iter_next(GTK_TREE_MODEL(w->title_store), &iter));
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
    
    pos = w->last_pos;
    if (w->last_playtime < 3000) {
	if (--pos < 0)
	    pos = g_list_length(w->last_playlist) - 1;
    }
    
    xmmsc_result_t *res;
    res = xmmsc_playlist_set_next(w->conn, pos);
    xmmsc_result_notifier_set(res, playlist_play, w);
    xmmsc_result_unref(res);
}

static void menu_playlist_row_activated(GtkTreeView *view,
	GtkTreePath *path, GtkTreeViewColumn *column, gpointer user_data)
{
    struct work_t *w = user_data;
    GtkTreeIter iter;
    if (!gtk_tree_model_get_iter(GTK_TREE_MODEL(w->title_store), &iter, path))
	return;
    gint pos;
    gtk_tree_model_get(GTK_TREE_MODEL(w->title_store), &iter,
	    COL_NUM, &pos, -1);
    
    xmmsc_result_t *res;
    res = xmmsc_playlist_set_next(w->conn, pos);
    xmmsc_result_notifier_set(res, playlist_play, w);
    xmmsc_result_unref(res);
}

static gchar *seekbar_format(GtkScale *scale, gdouble value)
{
    gint sec = value / 1000.0;
    return g_strdup_printf("%d:%02d", sec / 60, sec % 60);
}

static gboolean seekbar_change(GtkRange *range,
	GtkScrollType scroll, gdouble value, gpointer user_data)
{
    struct work_t *w = user_data;
    
    xmmsc_result_t *res;
    res = xmmsc_playback_seek_ms(w->conn, value, XMMS_PLAYBACK_SEEK_SET);
    xmmsc_result_unref(res);
    
    return TRUE;
}

static GtkWidget *create_seekbar_page(struct work_t *w)
{
    GtkWidget *frame = gtk_frame_new(NULL);
    gtk_container_set_border_width(GTK_CONTAINER(frame), 20);
    gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_NONE);
    gtk_widget_show(frame);
    
    GtkWidget *bar = gtk_scale_new(GTK_ORIENTATION_HORIZONTAL, w->seekbar_adj);
    g_signal_connect(bar, "format-value", G_CALLBACK(seekbar_format), NULL);
    // value-changed だとうまくいかない。
    g_signal_connect(bar, "change-value", G_CALLBACK(seekbar_change), w);
    gtk_container_add(GTK_CONTAINER(frame), bar);
    gtk_widget_show(bar);
    
    return frame;
}

static GtkWidget *create_title_list_page(struct work_t *w)
{
    GtkWidget *scr = gtk_scrolled_window_new(NULL, NULL);
    gtk_widget_show(scr);
    
    GtkWidget *view = gtk_tree_view_new_with_model(GTK_TREE_MODEL(w->title_store));
    g_signal_connect(view, "row-activated", G_CALLBACK(menu_playlist_row_activated), w);
    
    GtkCellRenderer *renderer;
    GtkTreeViewColumn *column;
    
    renderer = gtk_cell_renderer_toggle_new();
    gtk_cell_renderer_toggle_set_radio(GTK_CELL_RENDERER_TOGGLE(renderer), true);
    column = gtk_tree_view_column_new_with_attributes(
	    "", renderer,
	    "active", COL_NOW,
	    NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(view), column);
    
    renderer = gtk_cell_renderer_text_new();
    column = gtk_tree_view_column_new_with_attributes(
	    "#", renderer,
	    "xalign", COL_XALIGN,
	    "text", COL_NO,
	    NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(view), column);
    
    renderer = gtk_cell_renderer_text_new();
    column = gtk_tree_view_column_new_with_attributes(
	    "Title", renderer,
	    "text", COL_TITLE,
	    NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(view), column);
    
    gtk_container_add(GTK_CONTAINER(scr), view);
    gtk_widget_show(view);
    
    return scr;
}

static void menu_playlists_row_activated(GtkTreeView *view,
	GtkTreePath *path, GtkTreeViewColumn *column, gpointer user_data)
{
    struct work_t *w = user_data;
    GtkTreeIter iter;
    if (!gtk_tree_model_get_iter(GTK_TREE_MODEL(w->plist_store), &iter, path))
	return;
    const gchar *name;
    gtk_tree_model_get(GTK_TREE_MODEL(w->plist_store), &iter,
	    COL0_NAME, &name, -1);
    
    xmmsc_result_t *res;
    res = xmmsc_playlist_load(w->conn, name);
    xmmsc_result_notifier_set(res, playlist_play, w);	// fixme: 要るか?
    xmmsc_result_unref(res);
}

static GtkWidget *create_playlist_list_page(struct work_t *w)
{
    GtkWidget *scr = gtk_scrolled_window_new(NULL, NULL);
    gtk_widget_show(scr);
    
    GtkWidget *view = gtk_tree_view_new_with_model(GTK_TREE_MODEL(w->plist_store));
    g_signal_connect(view, "row-activated", G_CALLBACK(menu_playlists_row_activated), w);
    
    GtkCellRenderer *renderer;
    GtkTreeViewColumn *column;
    
    renderer = gtk_cell_renderer_toggle_new();
    gtk_cell_renderer_toggle_set_radio(GTK_CELL_RENDERER_TOGGLE(renderer), true);
    column = gtk_tree_view_column_new_with_attributes(
	    "", renderer,
	    "active", COL0_NOW,
	    NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(view), column);
    
    renderer = gtk_cell_renderer_text_new();
    column = gtk_tree_view_column_new_with_attributes(
	    "Playlist Name", renderer,
	    "text", COL0_NAME,
	    NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(view), column);
    
    gtk_container_add(GTK_CONTAINER(scr), view);
    gtk_widget_show(view);
    
    return scr;
}

static void menu_controller(GtkWidget *ww, gpointer user_data)
{
    struct work_t *w = user_data;
    
    GtkWidget *win = gtk_dialog_new_with_buttons("Mxmms", NULL, GTK_DIALOG_MODAL,
	    "Close", GTK_RESPONSE_CLOSE,
	    NULL);
    gtk_window_set_default_size(GTK_WINDOW(win), 600, 400);
    GtkWidget *content = gtk_dialog_get_content_area(GTK_DIALOG(win));
    
    GtkWidget *notebook = gtk_notebook_new();
    gtk_box_pack_start(GTK_BOX(content), notebook, TRUE, TRUE, 0);
    gtk_container_set_border_width(GTK_CONTAINER(content), 10);
    gtk_widget_show(notebook);
    
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook),
	    create_seekbar_page(w), gtk_label_new("Seek Bar"));
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook),
	    create_title_list_page(w), gtk_label_new("Title List"));
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook),
	    create_playlist_list_page(w), gtk_label_new("Playlists"));
    
    gtk_dialog_run(GTK_DIALOG(win));
    gtk_widget_destroy(win);
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

static void plist_store_renew(struct work_t *w)
{
    while (TRUE) {
	GtkTreeIter iter;
	if (!gtk_tree_model_get_iter_first(GTK_TREE_MODEL(w->plist_store), &iter))
	    break;
	gtk_list_store_remove(w->plist_store, &iter);
    }
    
    for (GList *lp = w->last_playlists; lp != NULL; lp = g_list_next(lp)) {
	GtkTreeIter iter;
	gtk_list_store_append(w->plist_store, &iter);
	gtk_list_store_set(w->plist_store, &iter,
		COL0_NOW, FALSE,
		COL0_NAME, lp->data,
		-1);
    }
}

static void plist_store_update_now(struct work_t *w)
{
    GtkTreeIter iter;
    if (!gtk_tree_model_get_iter_first(GTK_TREE_MODEL(w->plist_store), &iter))
	return;
    do {
	const gchar *name;
	gtk_tree_model_get(GTK_TREE_MODEL(w->plist_store), &iter,
		COL0_NAME, &name, -1);
	gtk_list_store_set(w->plist_store, &iter,
		COL0_NOW, strcmp(name, w->last_playlist_name) == 0, -1);
    } while (gtk_tree_model_iter_next(GTK_TREE_MODEL(w->plist_store), &iter));
}

static void playlists_got(GList *list, gint64 stamp, void *user_data)
{
    struct work_t *w = user_data;
    
    if (w->last_playlists_stamp > stamp) {
	g_list_free_full(list, g_free);
	return;
    }
    
    GList *lp1, *lp2;
    for (lp1 = list, lp2 = w->last_playlists;
	 lp1 != NULL && lp2 != NULL; lp1 = g_list_next(lp1), lp2 = g_list_next(lp2)) {
	const gchar *p1 = lp1->data;
	const gchar *p2 = lp2->data;
	if (strcmp(p1, p2) != 0)
	    break;
    }
    
    if (lp1 == NULL && lp2 == NULL) {
	// リストが変わってない
	g_list_free_full(list, g_free);
	return;
    }
    
    GList *old_list = w->last_playlists;
    w->last_playlists = list;
    w->last_playlists_stamp = stamp;
    
    g_list_free_full(old_list, g_free);
    
    printf("New List of Playlists\n");
#if 0
    for (GList *lp = w->last_playlists; lp != NULL; lp = g_list_next(lp))
	printf("%s\n", lp->data);
#endif
    
    plist_store_renew(w);
    plist_store_update_now(w);
}

static gboolean timer_playlists(gpointer user_data)
{
    struct work_t *w = user_data;
    
    playlists_get(w->conn, playlists_got, w);
    
    return TRUE;
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

static void change_size(MatePanelApplet *applet, gint size, gpointer user_data)
{
    struct work_t *w = user_data;
    gtk_widget_set_size_request(w->button, w->size, size);
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

static void destroy(MatePanelApplet *applet, gpointer user_data)
{
    struct work_t *w = user_data;
    
    xmmsc_io_disconnect(w->conn);
    g_object_unref(w->agrp);
    g_source_remove(w->timer);
    g_source_remove(w->timer_pl);
    g_free(w->last_playlist_name);
    void free_music(gpointer data) {
	struct music_t *mp = data;
	g_free(mp->artist);
	g_free(mp->title);
	g_free(mp);
    }
    g_list_free_full(w->last_playlist, free_music);
    g_list_free_full(w->last_playlists, g_free);
    g_object_unref(w->title_store);
    g_object_unref(w->plist_store);
    g_object_unref(w->seekbar_adj);
    g_free(w);
}

static gboolean callback(MatePanelApplet *applet, const gchar *iid, gpointer user_data)
{
    struct work_t *w = g_new0(struct work_t, 1);
    w->applet = applet;
    w->size = 100;
    
    static const char *xml =
	    "<menuitem name=\"Next\" action=\"MxmmsNext\"/>"
	    "<menuitem name=\"Previous\" action=\"MxmmsPrevious\"/>"
	    "<menuitem name=\"Controller\" action=\"MxmmsShowController\"/>"
	    "<separator/>"
	    "<menuitem name=\"About\" action=\"MxmmsShowAbout\" />";
    
    static const GtkActionEntry actions[] = {
	{ "MxmmsNext",           NULL, "Next",       NULL, NULL, G_CALLBACK(menu_next) },
	{ "MxmmsPrevious",       NULL, "Previous",   NULL, NULL, G_CALLBACK(menu_prev) },
	{ "MxmmsShowController", NULL, "Others...",  NULL, NULL, G_CALLBACK(menu_controller) },
	{ "MxmmsShowAbout", GTK_STOCK_ABOUT, "About", NULL, NULL, G_CALLBACK(about) },
    };
    
    w->agrp = gtk_action_group_new("Mxmms Applet Actions");
    gtk_action_group_add_actions(w->agrp, actions, sizeof actions / sizeof actions[0], w);
    
    mate_panel_applet_setup_menu(MATE_PANEL_APPLET(applet), xml, w->agrp);
    
    g_signal_connect(applet, "change_size", G_CALLBACK(change_size), w);
    g_signal_connect(applet, "destroy", G_CALLBACK(destroy), w);
    
    w->title_x = w->size;
    
    w->button = gtk_button_new();
    gtk_widget_set_size_request(w->button, w->size, mate_panel_applet_get_size(applet));
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
    w->timer_pl = g_timeout_add(1000, timer_playlists, w);
    w->last_playlist_name = g_strdup("");
    
    w->title_store = gtk_list_store_new(COL_NR,
	    G_TYPE_BOOLEAN,
	    G_TYPE_INT,
	    G_TYPE_INT,
	    G_TYPE_FLOAT,
	    G_TYPE_STRING,
	    G_TYPE_POINTER);
    w->plist_store = gtk_list_store_new(COL0_NR,
	    G_TYPE_BOOLEAN,
	    G_TYPE_STRING);
    
    w->seekbar_adj = gtk_adjustment_new(0, 0, 1, 0, 0, 0);
    g_object_ref(w->seekbar_adj);
    
    
    
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
