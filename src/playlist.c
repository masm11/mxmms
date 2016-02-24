/*    Mxmms - An XMMS2 client with a large button.
 *    Copyright (C) 2016 Yuuki Harano
 *
 *    This program is free software: you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation, either version 3 of the License, or
 *    (at your option) any later version.
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include <stdio.h>
#include <stdlib.h>
#include <gtk/gtk.h>
#include <mate-panel-applet.h>
#include <xmmsclient/xmmsclient.h>
#include <xmmsclient/xmmsclient-glib.h>
#include "common.h"

struct playlist_get_work_t {
    xmmsc_connection_t *conn;
    GList *list;	// struct music_t
    GQueue *ids;
    void (*callback)(GList *, gint64, void *);
    void *user_data;
    gint64 stamp;
};

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

static void playlist_get_completed(struct playlist_get_work_t *w)
{
    // w->list の所有権は呼び出し側に移る。
    (*w->callback)(w->list, w->stamp, w->user_data);
    
    g_queue_free(w->ids);
    g_free(w);
}

/* 1つの ID に対応する曲情報が得られた */
static int playlist_list_got__iter(xmmsv_t *val, void *user_data)
{
    struct playlist_get_work_t *w = user_data;
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
    
    w->list = g_list_append(w->list, mp);
    
    if (g_queue_get_length(w->ids) == 0) {
	// 終了。
	playlist_get_completed(w);
    } else {
	gint id = GPOINTER_TO_INT(g_queue_pop_head(w->ids));
	xmmsc_result_t *res = xmmsc_medialib_get_info(w->conn, id);
	xmmsc_result_notifier_set(res, playlist_list_got__iter, w);
	xmmsc_result_unref(res);
    }
    
    return TRUE;
}

/* ID のリストが得られた */
static int playlist_list_got(xmmsv_t *val, void *user_data)
{
    struct playlist_get_work_t *w = user_data;
    
    printf("playlist got.\n");
    print_xmmsv(val, 0);
    
    xmmsv_list_iter_t *it;
    if (xmmsv_get_list_iter(val, &it)) {
	do {
	    xmmsv_t *v;
	    xmmsv_list_iter_entry(it, &v);
	    gint id;
	    if (xmmsv_get_int(v, &id))
		g_queue_push_tail(w->ids, GINT_TO_POINTER(id));
	} while (xmmsv_list_iter_next(it), xmmsv_list_iter_valid(it));
    }
    
    printf("ids got! %p.\n", w->ids);
    GList *playlist = NULL;
    
    if (g_queue_get_length(w->ids) == 0) {
	// There is no music.
	w->list = NULL;
	playlist_get_completed(w);
	return TRUE;
    }
    
    gint id = GPOINTER_TO_INT(g_queue_pop_head(w->ids));
    printf("1st id: %d.\n", id);
    xmmsc_result_t *res = xmmsc_medialib_get_info(w->conn, id);
    xmmsc_result_notifier_set(res, playlist_list_got__iter, w);
    xmmsc_result_unref(res);
    printf("1st end.\n");
    
    return TRUE;
}

/* playlist 取得エントリ関数 */
void playlist_get(xmmsc_connection_t *conn, const gchar *name,
	void (*callback)(GList *, gint64, void *), void *user_data)
{
    struct playlist_get_work_t *w = g_new0(struct playlist_get_work_t, 1);
    
    w->callback = callback;
    w->user_data = user_data;
    w->conn = conn;
    w->ids = g_queue_new();
    w->stamp = g_get_real_time();
    
    xmmsc_result_t *res;
    res = xmmsc_playlist_list_entries(w->conn, name);
    xmmsc_result_notifier_set(res, playlist_list_got, w);
    xmmsc_result_unref(res);
}
