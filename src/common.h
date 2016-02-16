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

#endif	/* ifndef COMMON_H_INCLUDED */
