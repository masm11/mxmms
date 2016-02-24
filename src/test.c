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
