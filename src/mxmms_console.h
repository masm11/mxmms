#ifndef MXMMS_CONSOLE_H__INCLUDED
#define MXMMS_CONSOLE_H__INCLUDED

#include <gtk/gtk.h>
#include <xmmsclient/xmmsclient.h>

#define MXMMS_TYPE_CONSOLE	      (mxmms_console_get_type ())
#define MXMMS_CONSOLE(obj)	      (G_TYPE_CHECK_INSTANCE_CAST ((obj), MXMMS_TYPE_CONSOLE, MxmmsConsole))
#define MXMMS_CONSOLE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), MXMMS_TYPE_CONSOLE, MxmmsConsoleClass))
#define MXMMS_IS_CONSOLE(obj)	      (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MXMMS_TYPE_CONSOLE))
#define MXMMS_IS_CONSOLE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), MXMMS_TYPE_CONSOLE))
#define MXMMS_CONSOLE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), MXMMS_TYPE_CONSOLE, MxmmsConsoleClass))

typedef struct _MxmmsConsole        MxmmsConsole;
typedef struct _MxmmsConsolePrivate MxmmsConsolePrivate;
typedef struct _MxmmsConsoleClass   MxmmsConsoleClass;

struct _MxmmsConsole {
    GtkButton button;
    
    /*< private >*/
    MxmmsConsolePrivate *priv;
};

struct _MxmmsConsoleClass {
    GtkButtonClass parent_class;
};

GType                 mxmms_console_get_type          (void) G_GNUC_CONST;
GtkWidget*            mxmms_console_new               (void);
void                  mxmms_console_set_title         (MxmmsConsole *cons,
						       const gchar *title);
void                  mxmms_console_set_status        (MxmmsConsole *cons,
						       xmms_playback_status_t status);
void                  mxmms_console_set_playtime      (MxmmsConsole *cons,
						       gint64 msec);

#endif	/* ifndef MXMMS_CONSOLE_H__INCLUDED */
