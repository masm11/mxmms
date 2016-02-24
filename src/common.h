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
#ifndef COMMON_H_INCLUDED
#define COMMON_H_INCLUDED

struct music_t {
    guint id;
    gchar *artist;
    gchar *title;
};

void print_xmmsv(xmmsv_t *val, int indent);

void playlist_get(xmmsc_connection_t *conn, const gchar *name,
	void (*callback)(GList *, gint64, void *), void *user_data);

void playlists_get(xmmsc_connection_t *conn,
	void (*callback)(GList *, gint64, void *), void *user_data);

#endif	/* ifndef COMMON_H_INCLUDED */
