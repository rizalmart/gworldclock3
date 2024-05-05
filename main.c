/*  gworldclock

This program is designed to keep track of the time and date in various 
time zones around the world.  

The zones are kept in a configuration file (default ~/.tzlist), one zone 
per line.  Each line has one or two entries: the first is the TZ value 
corresponding, the second is an optional description string enclosed in 
inverted commas ('"').

The config file is compatible for use with tzwatch, a shell script 
writing the time in the given zones to stdout.

Note, time_t is evil.  It apparently resolves to a 32bit integer (on x86 at 
least), which only lets the clocks go back to 8:48pm, 13 Dec 1901 (GMT)
(2147483648 seconds before 1 Jan 1970).
On the other side, the limit is 19 January 2038, 3:13am.
There appears to be no simple way around this limitation.  If we use struct tm
throughout, then on printing to display for each time zone, asctime does not
update the display for the zone in the way that ctime does.
(curiously, asctime *is* updated if preceded by ctime in the same line, e.g.
asprintf( &text, "%.0s%s", ctime(&t), asctime( tp ) );  
but in this case we still suffer the limitation of the 32bit counter).
So it's all very silly really.  You'll just have to put up with not syncing
earlier than 1902.  Too bad for your time machine.
The alternative is to do some dreadful dreadful frigging around with the
daylight savings and time difference fields in struct tm, which I'm
certainly not going to do right now.
  

To do:  
- change CLists (add zone) to TreeViews
- rewrite "About" text using neater GTK text formatting capabilities.
- update list of days in month to be correct for the given month 
- convert time_t references to struct tm and tweak time difference
by hand to allow years beyond the 2^32 sec limit (?)
- have someone compile it for Win32 so I can give it to Kostya & Andrei
*/


/* Copyright (C) 2000-2005 Drew F. Parsons
 *
 *     This program is free software; you can redistribute it and/or modify
 *     it under the terms of the GNU General Public License as published by
 *     the Free Software Foundation; ( version 2 of the License at time of 
 *     writing).
 *
 *     This program is distributed in the hope that it will be useful,
 *     but WITHOUT ANY WARRANTY; without even the implied warranty of
 *     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *     GNU General Public License for more details.
 *
 *     You should have received a copy of the GNU General Public License
 *     along with this program; if not, write to the Free Software
 *     Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <config.h>
#include <errno.h>
#include <gtk/gtk.h>
#include <stdlib.h>
#include <locale.h>
#include <libintl.h>
#define _(A) gettext(A)

#include "main.h"
#include "options.h"
#include "zones.h"
#include "resize.h"
#include "timer.h"

/* so many global variables....how naughty! 
collect them in a structure to be passed around? */
gint changed=0;
gint OKtoQuit=1;
GString *configfile;
GString *optionFile;

/* id of second timer.
   Set to -1 during synchronisation.
*/
gint timer;

GtkBox *syncBox;

const gchar* translate_func (const gchar * s, gpointer p)
{
  /* 
     translation function used for itemfactory
     added by dancerj, Junichi Uekawa, 26 Jul 2003.
   */
  return gettext(s);
}


void AboutDialog( GtkWidget *w, gpointer clocklist )
{
    GtkWidget *dialog;

    GString *copyright, *comments, *licence;

    /* set up strings */
    copyright = g_string_new( "Drew Parsons <dparsons@emerall.com>  " );
#ifdef RELEASE_DATE
    g_string_append(copyright, RELEASE_DATE);
#endif

    licence = g_string_new( _("Released under the General Public License (GPL) v2.0") );

    comments = g_string_new(  _( "gworldclock allows you to keep track of the current time"
				 " in time zones all round the world.  You may also \"rendezvous\""
				 " your time zones to a specified time and date.") );
    

    /* build the about dialog */
    
    dialog = gtk_about_dialog_new();
    
#ifdef PACKAGE_VERSION
    gtk_about_dialog_set_version( GTK_ABOUT_DIALOG(dialog), PACKAGE_VERSION );
#endif
    gtk_about_dialog_set_copyright( GTK_ABOUT_DIALOG(dialog), copyright->str );
    gtk_about_dialog_set_comments( GTK_ABOUT_DIALOG(dialog), comments->str );
    gtk_about_dialog_set_license( GTK_ABOUT_DIALOG(dialog) , licence->str );
    gtk_about_dialog_set_translator_credits( GTK_ABOUT_DIALOG(dialog), _("translator-credits") );

    gtk_show_about_dialog( NULL,
       "copyright", gtk_about_dialog_get_copyright( GTK_ABOUT_DIALOG(dialog) ),
       "license", gtk_about_dialog_get_license( GTK_ABOUT_DIALOG(dialog) ),
       "comments", gtk_about_dialog_get_comments( GTK_ABOUT_DIALOG(dialog) ),
		 NULL );

    g_string_free(copyright,TRUE);
    g_string_free(comments,TRUE);
    g_string_free(licence,TRUE);

}


/* ensure we go through the same save routine when quitting from the menu (C-Q)
   or by pressing the window's 'close' button */
void send_clock_quit( GtkWidget *w, gpointer clocklist )
{
  GdkEvent event;
  gint return_val;
  extern gint OKtoQuit;

  g_signal_emit_by_name((GObject *) gtk_widget_get_toplevel(
			     (GtkWidget *)g_object_get_data(clocklist, MAIN_WINDOW)), 
			  "delete_event", &event, &return_val);

  /* why didn't the above send "delete", which then sends "destroy"??
     Have to "destroy" by hand...   */
  if(OKtoQuit)
    gtk_main_quit();
  else
    OKtoQuit=1;
}

/* save the config data when quitting, if necessary */
gint worldclock_quit(GtkWidget *widget,
		     GdkEvent  *event,
		     gpointer   clocklist )
{
  extern gint changed;
  extern gint OKtoQuit;
  extern GString *configfile;
  gint choice;
  GtkWidget *dialog;

  gchar *title="Save Zones";
  GString *msg;
  gchar *buttons3[] =  { "Yes", "No", "Cancel" };
  gchar *buttons2[] =  { "Yes", "No" };

  SaveOptions();

  if(changed) 
  {
     msg = g_string_new(NULL);
     g_string_sprintf(msg," Do you want to save your modified zone list? ");
     dialog = gtk_message_dialog_new( NULL,
				      0,
				      GTK_MESSAGE_QUESTION,
				      GTK_BUTTONS_YES_NO,
				      "%s", msg->str);
     gtk_dialog_add_buttons( GTK_DIALOG(dialog), 
			     "Cancel", GTK_RESPONSE_CANCEL,
			     NULL);
     choice = gtk_dialog_run( GTK_DIALOG(dialog) );
     gtk_widget_destroy (dialog);
     g_string_free(msg,TRUE);
     if(choice==GTK_RESPONSE_YES)   /* yes, save */
     {
	if (!SaveZones(widget, clocklist)) 
	{
	   msg = g_string_new(NULL);
	   g_string_sprintf(msg," Error saving zone file \"%s\": \n %s \n\n Continue quitting? ",
			    configfile->str, g_strerror(errno));
	   dialog = gtk_message_dialog_new( NULL,
					    0,
					    GTK_MESSAGE_QUESTION,
					    GTK_BUTTONS_YES_NO,
					    "%s", msg->str);

	   choice = gtk_dialog_run (GTK_DIALOG (dialog));
	   gtk_widget_destroy (dialog);

	   g_string_free(msg,TRUE);
	   if(choice==GTK_RESPONSE_NO) /* no, don't quit if there was an error */
	   {
	      OKtoQuit=0;    
	      return TRUE;
	   }
	}
	OKtoQuit=1;
	return FALSE;
     }
     else if (choice==GTK_RESPONSE_NO) /* no, don't save */
     {
	OKtoQuit=1;
	return FALSE;
     }
     else /* cancel from quitting */
     {
	OKtoQuit=0;    
	return TRUE;
     }
  }

  return FALSE;
}


/* Process mouse click when a zone in the list is selected.
   Double-click with left mouse button (button 1) pops up the 
   "Change Description" dialog box.
   Single-click with right button (button 3) pops up the menu of actions.
*/
static gboolean ButtonPressedInList (GtkWidget *clocklist, 
				     GdkEventButton *event,
				     gpointer selection)
{
   GtkTreeModel *clocklistModel;

   static GtkWidget *popup;

   gboolean returnVal = FALSE;

   if( (event->button==1) && ( event->type==GDK_2BUTTON_PRESS))
   {
      ChangeZoneDescription(clocklist, (gpointer)clocklist);
      returnVal = TRUE;
   }
   /* attempting to identify when reorder has occurred
      Will probably have to wipe this code 
      else if ( (event->type==GDK_BUTTON_RELEASE) && (event->button==1)) 
      {
      g_print("released button 1 for row %d\n", selectedRow);
      returnVal = TRUE;
      }    
      else if (event->button==1) 
      {
      g_print("pressed button 1 for row %d\n", selectedRow);
      returnVal = FALSE;
      }
   */	
   else if (event->button==3) 
   {
      if(!popup) { /* create only once! */
	  popup = gtk_ui_manager_get_widget (g_object_get_data(G_OBJECT(clocklist), UI_MANAGER), "/PopupMenu");
      }
      gtk_menu_popup (GTK_MENU(popup), NULL, NULL, NULL, NULL,
		      event->button, event->time);   
      returnVal = TRUE;
   }

   return returnVal;
}


int main( int argc, char *argv[] )
{
  GtkWidget *window, *scrolled_window;
  GtkWidget *main_vbox;
  GtkWidget *menubar;
  GtkListStore *clocklistModel;
  GtkWidget *clocklist;
  gchar *titles[2] = { N_("Time Zone"), N_("Time&Date") };
  GtkCellRenderer *renderer;
  GtkTreeViewColumn *column;
  GtkTreeSelection *select;
  GdkRectangle cell;
  gint cellHeight;

  GtkActionGroup *action_group;
  GtkUIManager *ui_manager;
  GtkAccelGroup *accel_group;
  GError *error;

  gint xoff, yoff, h, w;
  GSignalQuery query;

  gtk_init (&argc, &argv);
  GetOptions(argc,argv);
  bindtextdomain(PACKAGE_NAME, LOCALEDIR);
  bind_textdomain_codeset (PACKAGE_NAME, "UTF-8");
  textdomain(PACKAGE_NAME);

  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  g_signal_connect (G_OBJECT (window), "destroy", 
		      G_CALLBACK (gtk_main_quit), 
		      "WM destroy");
  gtk_window_set_title (GTK_WINDOW(window), "gworldclock");

  main_vbox = gtk_vbox_new (FALSE, 5);
  gtk_container_set_border_width (GTK_CONTAINER (main_vbox), 1);
  gtk_container_add (GTK_CONTAINER (window), main_vbox);
  gtk_widget_show (main_vbox);

  /* Create a scrolled window to pack the list widget into */
  scrolled_window = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window),
				  GTK_POLICY_AUTOMATIC, GTK_POLICY_ALWAYS);
  
  gtk_box_pack_end(GTK_BOX(main_vbox), scrolled_window, TRUE, TRUE, 0);
  gtk_widget_show (scrolled_window);


  /* the main clock object is created here */
  clocklistModel = gtk_list_store_new (LIST_COLUMNS, 
				  G_TYPE_STRING, /* TZ name */
				  G_TYPE_STRING, /* TZ description */
				  G_TYPE_STRING  /* time/date */
     );

  clocklist = gtk_tree_view_new_with_model (GTK_TREE_MODEL (clocklistModel));  
  gtk_tree_view_set_reorderable( GTK_TREE_VIEW(clocklist), TRUE );
  g_signal_connect(G_OBJECT(clocklistModel), "rows_reordered", 
		   G_CALLBACK(registerReorderedRows), NULL);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes (gettext(titles[0]),
						     renderer,
						     "text", TZ_DESCRIPTION,
						     NULL);
  gtk_tree_view_column_set_sizing( column, GTK_TREE_VIEW_COLUMN_AUTOSIZE );

  g_signal_connect(G_OBJECT(column), "notify::width", 
		   G_CALLBACK(updateColumnWidth), clocklist);
  gtk_tree_view_append_column (GTK_TREE_VIEW (clocklist), column);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes (gettext(titles[1]),
						     renderer,
						     "text", TZ_TIMEDATE,
						     NULL);
  gtk_tree_view_column_set_sizing( column, GTK_TREE_VIEW_COLUMN_AUTOSIZE );
  gtk_tree_view_append_column (GTK_TREE_VIEW (clocklist), column);

  select = gtk_tree_view_get_selection (GTK_TREE_VIEW (clocklist));
  gtk_tree_selection_set_mode (select, GTK_SELECTION_SINGLE);
  
  g_signal_connect(G_OBJECT(clocklist),
		   "button_press_event",
		   G_CALLBACK(ButtonPressedInList),
		   (gpointer)select);

  gtk_container_add(GTK_CONTAINER(scrolled_window), clocklist);
  start_clocks((gpointer)clocklist);
  gtk_widget_show (clocklist);

  g_object_set_data(G_OBJECT(clocklist), MAIN_WINDOW, window);
  g_signal_connect (G_OBJECT (window), "delete_event", 
		      G_CALLBACK (worldclock_quit), (gpointer)clocklist);

  /* set up menu */
  action_group = gtk_action_group_new ("UIActions");
  gtk_action_group_set_translation_domain (action_group, PACKAGE_NAME);
  gtk_action_group_add_actions (action_group, entries, G_N_ELEMENTS (entries), clocklist);  
  gtk_action_group_add_toggle_actions (action_group, toggle_entries, G_N_ELEMENTS (toggle_entries), clocklist);
  g_object_set_data(G_OBJECT(clocklist), ACTIONS, action_group);


  ui_manager = gtk_ui_manager_new ();
  gtk_ui_manager_insert_action_group (ui_manager, action_group, 0);

  accel_group = gtk_ui_manager_get_accel_group (ui_manager);
  gtk_window_add_accel_group (GTK_WINDOW (window), accel_group);

  error = NULL;
  if (!gtk_ui_manager_add_ui_from_string (ui_manager, ui_description, -1, &error))
  {
      g_message ("building menus failed: %s", error->message);
      g_error_free (error);
      exit (EXIT_FAILURE);
  }

  menubar = gtk_ui_manager_get_widget (ui_manager, "/MainMenu");
  gtk_box_pack_start (GTK_BOX (main_vbox), menubar, FALSE, FALSE, 0);

  g_object_set_data(G_OBJECT(clocklist), UI_MANAGER, ui_manager);


  /* stick the synchronise control panel at the top and hide it */
  syncBox = constructSyncBox((gpointer)clocklist);
  gtk_box_pack_start(GTK_BOX(main_vbox), (GtkWidget *)syncBox, FALSE, FALSE, 0);
  gtk_widget_hide((GtkWidget *) syncBox );

  /* calculate the time each second */
  timer = start_timer( (gpointer)clocklist );

  gtk_widget_show (window);
  resizeWindow( window, GTK_TREE_VIEW(clocklist) );

  gtk_main ();
         
  return(0);
}

