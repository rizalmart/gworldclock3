/* various ancillary functions */

#ifndef GWORLDCLOCK_MISC
#define GWORLDCLOCK_MISC

#include <gdk/gdkkeysyms.h>
#include <gtk/gtk.h>

#define OK_BUTTON       1
#define CANCEL_BUTTON   2

void DestroyWindow(GtkWidget *w, gpointer window);

gint GotOK(GtkWidget *w, GdkEventKey *event,  gpointer Button);

gint GotOKInDialog(GtkWidget *w, GdkEventKey *event,  gpointer dialog);

/* find the length (in characters) of the longest entry in a GList
   containing string data. */
gint getLongestEntry(GList *list);

/* displays the message in an "OK" style dialog box.
   messageType indicates Warning, Error, etc (see GtkMessageDialog).
*/
void showMessageDialog( gchar *message, GtkMessageType messageType );

#endif
