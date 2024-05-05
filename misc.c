/* various ancillary functions */

#include <string.h>

#include "misc.h"

void DestroyWindow(GtkWidget *w, gpointer window)
{
  gtk_widget_destroy((GtkWidget *)window);
}

gint GotOK(GtkWidget *w, GdkEventKey *event,  gpointer Button)
{
  GdkEvent *dummyevent;
  gint return_val;

  if(event->keyval==GDK_KEY_Return) {
    dummyevent=(GdkEvent*)g_malloc(sizeof(GdkEvent));
    g_signal_emit_by_name((GObject *) Button,
			    "clicked", &dummyevent, &return_val);
    g_free(dummyevent);
    return TRUE;
  }
  return FALSE;
}

gint GotOKInDialog(GtkWidget *w, GdkEventKey *event,  gpointer dialog)
{
  gint return_val;

  if(event->keyval==GDK_KEY_Return) 
  {
     gtk_dialog_response( GTK_DIALOG( dialog ), OK_BUTTON );
     return TRUE;
  }
  return FALSE;
}

/* find the length (in characters) of the longest entry in a GList
   containing string data. */
gint getLongestEntry(GList *list)
{
  GList *plist;
  gint longest = 0;
  gint length;
  
  plist = list;
  while ( plist != NULL ) 
    {
    length = strlen( (gchar*)plist->data );
    if ( length > longest )
      longest = length;
    plist = g_list_next(plist);
  }
  return longest;
}


/* displays the message in an "OK" style dialog box.
   messageType indicates Warning, Error, etc (see GtkMessageDialog).
*/
void showMessageDialog( gchar *message, GtkMessageType messageType )
{
   GtkWidget *dialog;

   dialog = gtk_message_dialog_new( NULL,
				    0,
				    messageType,
				    GTK_BUTTONS_OK,
				    "%s", message);

   g_signal_connect_swapped (G_OBJECT (dialog), "response",
			     G_CALLBACK (gtk_widget_destroy),
			     G_OBJECT (dialog));
   gtk_widget_show(dialog);				     
}

