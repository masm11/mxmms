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
#include <string.h>
#include <gtk/gtk.h>
#include <mate-panel-applet.h>
#include <xmmsclient/xmmsclient.h>
#include <xmmsclient/xmmsclient-glib.h>
#include "common.h"

struct playlists_get_work_t {
    xmmsc_connection_t *conn;
    GList *list;	// gchar *
    void (*callback)(GList *, gint64, void *);
    void *user_data;
    gint64 stamp;
};


static gboolean playlists_got(xmmsv_t *val, void *user_data)
{
    struct playlists_get_work_t *w = user_data;
    
    // printf("playlists got.\n");
    // print_xmmsv(val, 0);
    
    xmmsv_list_iter_t *it;
    if (xmmsv_get_list_iter(val, &it)) {
	do {
	    xmmsv_t *v;
	    xmmsv_list_iter_entry(it, &v);
	    const gchar *str;
	    if (xmmsv_get_string(v, &str)) {
		if (str[0] != '_')
		    w->list = g_list_insert_sorted(w->list, g_strdup(str), (GCompareFunc) strcmp);
	    }
	} while (xmmsv_list_iter_next(it), xmmsv_list_iter_valid(it));
    }
    
    (*w->callback)(w->list, w->stamp, w->user_data);
    
    return TRUE;
}

void playlists_get(xmmsc_connection_t *conn,
	void (*callback)(GList *, gint64, void *), void *user_data)
{
    struct playlists_get_work_t *w = g_new0(struct playlists_get_work_t, 1);
    
    w->callback = callback;
    w->user_data = user_data;
    w->conn = conn;
    w->stamp = g_get_real_time();
    
    xmmsc_result_t *res;
    res = xmmsc_playlist_list(w->conn);
    xmmsc_result_notifier_set(res, playlists_got, w);
    xmmsc_result_unref(res);
}
