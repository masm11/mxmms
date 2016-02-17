#include <gtk/gtk.h>
#include "mxmms_console.h"

int main(int argc, char **argv)
{
    gtk_init(&argc, &argv);
    
    GtkWidget *top = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_widget_set_size_request(top, 48, 48);
    gtk_widget_show(top);
    
    GtkWidget *cons = mxmms_console_new();
    gtk_widget_show(cons);
    gtk_container_add(GTK_CONTAINER(top), cons);
    
    mxmms_console_set_title(MXMMS_CONSOLE(cons), "TITLE");
    mxmms_console_set_playtime(MXMMS_CONSOLE(cons), 0);
    
    gtk_main();
    
    return 0;
}
