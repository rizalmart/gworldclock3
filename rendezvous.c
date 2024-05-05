/* functions related to time zone synchronisation
   (i.e. "Rendezvous")
  
   For simplicity, and because the phrase "synchronisation" made sense to me,
   I'll leave internal references to "sync" without changing them to 
   "rendezvous".
*/

#define _GNU_SOURCE

#include <stdlib.h>
#include <string.h>
#include <libintl.h>
#include <stdio.h>
#include <time.h>
#define _(A) gettext(A)

#include "main.h"
#include "rendezvous.h"
#include "timer.h"
#include "misc.h"

extern GtkBox *syncBox;
/* id of second timer.
   Set to -1 during synchronisation.
*/
extern gint timer;

/* Normal UI format is to press the "Done" button when finished with 
   Rendezvous.  Certain users (see Debian bug #213368) seem, however,
   not to understand that "Done" means, ahem, "done".  To accommodate
   such users I will allow the "Done" label to be set by the user:
   in $XDG_CONFIG_HOME/gworldclock/gworldclock.xml, e.g.:
   <gworldclock>
       <rendezvous>
           <doneLabel>Return</doneLabel>
       </rendezvous>
   </gworldclock>
*/

/* should be left as NULL unless explicitly set by user in config file */
GString *rendezvousDoneLabel = NULL;

/* keeps track of whether or not Rendezvous is currently running */
static gboolean rendezvousIsActive = FALSE;
static gulong leftClickHandlerForRendezvous=0;

/* returns name of month (as defined by locale, converted to UTF-8)
   corresponding to given index (between 1 and 12) */
gchar* getNameOfMonth( gint index )
{
  gchar *number, *monthName, localeMonthName[50];
  struct tm tmonth;
  time_t t;

  if ( (index < 1) || (index > 12) ) {
    return NULL;
  }

  asprintf(&number, "%i", index );
  strptime(number, "%m", &tmonth); 
  strftime(localeMonthName, sizeof(localeMonthName), "%B", &tmonth);
  monthName = g_locale_to_utf8 ( localeMonthName,
				   -1, NULL, NULL, NULL );

  free(number);

  return monthName;
}

/* returns the index (between 1 and 12 ) of the given month,
   identified by name (for current locale, but encoded in UTF-8) */
gint getMonthIndex( gchar *monthName )
{
  struct tm tmonth;
  time_t t;
  gchar * localeMonthName;

  localeMonthName = g_locale_from_utf8 ( monthName,
  			   -1, NULL, NULL, NULL );
  strptime(localeMonthName, "%B", &tmonth); 
  g_free(localeMonthName);

  /* don't forget to add 1, since struct tm counts months 0 to 11 */
  return ( tmonth.tm_mon + 1 );
}



/* set the display values for date and time in the SyncBox to the current time */
void setDefaultSyncValues(gchar *description, gchar *timezone)
{
  GtkWidget *label;
  GtkWidget *hour;
  GtkWidget *minute;
  GtkWidget *day;
  GtkWidget *month;
  GtkWidget *year;

  gint *dayHandler, *yearHandler;

  struct tm *timeSet, timeSetStruct;
  time_t t;
  gchar *TZdefault;
  gchar *s, *oldtimezone;
  gint yearChangeSignal;

  timeSet = &timeSetStruct;

  label = g_object_get_data( G_OBJECT(syncBox), "label" );
  gtk_label_set_text( (GtkLabel *) label, description);

  /* initialise time from sync time if syncing has started */
  if ( timer == -1) {
    oldtimezone = g_object_get_data( G_OBJECT(syncBox), "timezone"  );
    t = extractSyncTime();
  }

  g_object_set_data( G_OBJECT(syncBox), "timezone", (gpointer) timezone );

  day = g_object_get_data( G_OBJECT(syncBox), "day" );
  month = g_object_get_data( G_OBJECT(syncBox), "month" );
  year = g_object_get_data( G_OBJECT(syncBox), "year" );
  hour = g_object_get_data( G_OBJECT(syncBox), "hour" );
  minute = g_object_get_data( G_OBJECT(syncBox), "minute" );

  TZdefault = getenv("TZ");

  /* assign time zone in which time is to be given, if not "Local". */
  if(strcasecmp(timezone,"Local"))    
    setenv("TZ",timezone,1);


  /* initialise time from current time if syncing hasn't started yet */
  if ( timer != -1 ) {
    time(&t);
  }

  /* must read in time zone here, if we don't want to use localtime(&t) */
  tzset();

  /* initialise timeSet, setting time zone info */
  timeSet = localtime_r( &t, timeSet ); 

  /* ensure user's default time zone is not lost */
  if (TZdefault)
    setenv("TZ",TZdefault,1);
  else
    unsetenv("TZ");

  /* place the various bits of data into their fields */

  dayHandler =(gint *) g_object_get_data( G_OBJECT(day), "dayHandler" );
  g_signal_handler_block( (GObject *)gtk_spin_button_get_adjustment((GtkSpinButton *) day ), 
			    *dayHandler );
  asprintf( &s, "%i", timeSet->tm_mday );
  gtk_spin_button_set_value( (GtkSpinButton *)day, atoi(s) );
  free( s );
  g_signal_handler_unblock(  
     (GObject *)gtk_spin_button_get_adjustment((GtkSpinButton *) day ), 
     *dayHandler );

  gtk_combo_box_set_active( GTK_COMBO_BOX(month), timeSet->tm_mon);

  /* keep the year spin button from handling value_changed,
     or the clock will try to update before it's ready! */
  yearHandler = (gint *)g_object_get_data( G_OBJECT(year), "yearHandler" );
  g_signal_handler_block( (GObject *)gtk_spin_button_get_adjustment((GtkSpinButton *) year ), 
			    *yearHandler );
  asprintf( &s, "%i", (1900 + timeSet->tm_year) );
  gtk_spin_button_set_value( (GtkSpinButton *)year, atoi(s) );
  free( s );
  g_signal_handler_unblock(  (GObject *)gtk_spin_button_get_adjustment((GtkSpinButton *) year ), 
			       *yearHandler );

  asprintf( &s, "%.2i", timeSet->tm_hour);
  gtk_entry_set_text( GTK_ENTRY(hour), strdup(s) );
  free( s );

  asprintf( &s, "%.2i", timeSet->tm_min);
  gtk_entry_set_text( GTK_ENTRY(minute), strdup(s) );
  free( s );


}

/* Allows menu selection to turn Rendezvous on and off. */
void ToggleRendezvous(GtkToggleAction *action, gpointer clocklist)
{
    if ( gtk_toggle_action_get_active( action ) )
    {
	Rendezvous( NULL, clocklist );
    }
    else
    {
	unsynchronise( NULL, clocklist );
    }		
}

/* "Synchronise" is labelled "Rendezvous" in the user interface, to help
   users understand the meaning of the function. 
   Correspondingly this function is likewise relabelled */
void Rendezvous(GtkWidget *w, gpointer clocklist)
{
   extern gint changed;
   
   GString *title, *msg;
   gchar *button[]={"OK"};
   GdkWindow *window;
   gint width, height;
   gint win_x, win_y;
   gint selectedRow = 0; //temp  
   gchar *description, *timezone;
   GtkTreeModel *clocklistModel;
   GtkTreeIter iter;
   GtkTreeSelection *select;
   GtkToggleAction *toggleAction;
   
   if ( gtk_tree_selection_get_selected( 
	   gtk_tree_view_get_selection( clocklist ),
	   &clocklistModel,
	   &iter ) )
   {
       gtk_tree_model_get( clocklistModel, &iter,
			   TZ_NAME, &timezone,
			   TZ_DESCRIPTION, &description,
			   -1);
      
       /* set sync values to current time */
       setDefaultSyncValues(description, timezone);

      if ( timer != -1 ) {

	 /* freeze timer */
	 stop_timer();

	 /* adjust size of window to fit syncbox */
	 window = gtk_widget_get_parent_window( (GtkWidget *) syncBox );
	 
	 gdk_window_get_geometry(window, &win_x, &win_y, &width, &height);
	 //gdk_window_get_size(window, &width, &height);
	 
	 
	 gdk_window_resize( window, width, height + SYNCBOX_HEIGHT );
      }

      /* enable single clicking on a zone in the list to rendezvous
	 against that zone */
      if ( ! rendezvousIsActive )
      {
	  if ( leftClickHandlerForRendezvous )
	  {
	      g_signal_handler_unblock( 
		  G_OBJECT(gtk_tree_view_get_selection( clocklist )), 
		  leftClickHandlerForRendezvous );
	  }
	  else
	  {
	      leftClickHandlerForRendezvous =
		  g_signal_connect(G_OBJECT(gtk_tree_view_get_selection( clocklist )),
				   "changed",
				   G_CALLBACK(Rendezvous),
				   (gpointer)clocklist );
	  }
      }

      synchroniseTimes( NULL, (gpointer) clocklist);
     
      /* and change display to correspond to given time */
      gtk_widget_show( (GtkWidget *) syncBox );

      rendezvousIsActive = TRUE;

      /* Check the Rendezvous main menu item is toggled on, if we got
	 here via the popup menu */
      toggleAction = GTK_TOGGLE_ACTION( gtk_action_group_get_action(
	  GTK_ACTION_GROUP( g_object_get_data(G_OBJECT(clocklist), ACTIONS) ),
	  "ToggleRendezvous" ) );
      gtk_toggle_action_set_active( toggleAction, TRUE );


   }
   else 
   {
      title = g_string_new("Delete Zone");
      msg = g_string_new("No zone chosen for synchronising.");
      showMessageDialog( msg->str, GTK_MESSAGE_WARNING );
      g_string_free(msg,TRUE);
      g_string_free(title,TRUE);
   }
}


/* reads the values set in the syncBox fields and turns them into
   a time_t time.
   Note: this time_t crashes (mktime returns -1) at 6:45am, 14 Dec 1901
   At 6:46am, t=-2147483640 (2^31=2147483648) - 32bit int limitation :(  
   useless! */
time_t extractSyncTime()
{
  int newDay, newMonth, newYear, newHour, newMinute;
  
  GtkWidget *hour;
  GtkWidget *minute;
  GtkWidget *day;
  GtkWidget *month;
  GtkWidget *year;

  gchar *timezone;

  struct tm *timeSet, timeSetStruct;
  time_t t;
  gchar *TZdefault;
  gchar *s;

  timeSet = &timeSetStruct;

  day = g_object_get_data( G_OBJECT(syncBox), "day" );
  month = g_object_get_data( G_OBJECT(syncBox), "month" );
  year = g_object_get_data( G_OBJECT(syncBox), "year" );
  hour = g_object_get_data( G_OBJECT(syncBox), "hour" );
  minute = g_object_get_data( G_OBJECT(syncBox), "minute" );

  newDay = gtk_spin_button_get_value_as_int( (GtkSpinButton *) day );
  newMonth = gtk_combo_box_get_active(GTK_COMBO_BOX(month));
  newYear = gtk_spin_button_get_value_as_int( (GtkSpinButton *) year ) - 1900;
  newHour = atoi( gtk_entry_get_text( GTK_ENTRY(hour) ) );
  newMinute = atoi( gtk_entry_get_text( GTK_ENTRY(minute) ) );

  TZdefault = getenv("TZ");
    
  /* assign time zone in which time is to be given, if not "Local". */
  timezone = g_object_get_data( G_OBJECT(syncBox), "timezone" );
  if(strcasecmp(timezone,"Local"))    
    setenv("TZ",timezone,1);

  /* initialise timeSet, setting time zone info */
  time(&t);
  tzset();
  timeSet = localtime_r( &t, timeSet ); 

  /* set new day to get proper daylight savings calculations */  
  timeSet->tm_mday = newDay;
  timeSet->tm_mon = newMonth;
  timeSet->tm_year = newYear;
  timeSet->tm_hour = newHour;
  timeSet->tm_min = newMinute;

  /* reset timeSet to time on new day, to make sure daylight savings is set properly */
  t = mktime( timeSet );

  timeSet = localtime_r( &t, timeSet ); 

  /* and set new time (set everything just to be sure) */
  timeSet->tm_mday = newDay;
  timeSet->tm_mon = newMonth;
  timeSet->tm_year = newYear;
  timeSet->tm_hour = newHour;
  timeSet->tm_min = newMinute;

  /* set seconds to zero for neatness */
  timeSet->tm_sec = 0;

  t = mktime( timeSet );

  /* ensure user's default time zone is not lost */
  if (TZdefault)
    setenv("TZ",TZdefault,1);
  else
    unsetenv("TZ");

  return t;
}


/* callback function to be run if any of the sync fields are changed.
   Updates the times in each zone to the new sync time. 
   The changed field is referenced as field, but is not used as such.
*/
void synchroniseTimes( GtkWidget *field, gpointer clocklist)
{
  time_t t;

  /* grab sync time */
  t = extractSyncTime();

  /* and display it */
  gtk_tree_model_foreach( gtk_tree_view_get_model( GTK_TREE_VIEW(clocklist)),
			   (GtkTreeModelForeachFunc) SetToGivenTime,
			  (gpointer) t );
  
}


/* after done synchronising zones, return back to current time */
void unsynchronise( GtkWidget *w, gpointer clocklist)
{
  GdkWindow *window;
  gint width, height;
  gint win_x, win_y;

  if ( leftClickHandlerForRendezvous )
  {
      g_signal_handler_block( 
	  G_OBJECT(gtk_tree_view_get_selection( clocklist )), 
	  leftClickHandlerForRendezvous );
  }

  SetTime( clocklist );
  gtk_widget_hide( (GtkWidget *) syncBox );
  
  /* adjust window to normal size */
  window = gtk_widget_get_parent_window( (GtkWidget *) syncBox );
  
  //gdk_window_get_size(window, &width, &height);
  gdk_window_get_geometry(window, &win_x, &win_y, &width, &height);
  
  
  
  gdk_window_resize( window, width, height - SYNCBOX_HEIGHT );

  timer = start_timer( clocklist );

  rendezvousIsActive = FALSE;
}

/* after done synchronising zones and the Done button has been
  clicked, return back to current time. Goes through the Rendezvous
  toggle action, to make sure it is switched off as well.  */
void unsynchroniseFromDone( GtkWidget *w, gpointer clocklist)
{
  GtkToggleAction *toggleAction;

  /* Check the Rendezvous main menu item is toggled off, if we got
     here via the "Done" button */
  toggleAction = GTK_TOGGLE_ACTION( gtk_action_group_get_action(
		   GTK_ACTION_GROUP( g_object_get_data(G_OBJECT(clocklist), 
		   ACTIONS) ),
	  "ToggleRendezvous" ) );

  // go to unsynchronise by toggle the Rendezvous button off
  gtk_toggle_action_set_active( toggleAction, FALSE );

}

void initialiseMonthList(GObject *months)
{
   gint i;
   
    /* months in year by name (as set by locale) */
   for ( i=1; i<=12; i++ )  
   {
      gtk_combo_box_text_append_text(GTK_COMBO_BOX(months),
				getNameOfMonth( i ) );
   }
}

/* construct the box in which the controls for synchronising dates will be 
   found */
GtkBox *constructSyncBox(gpointer clocklist)
{
   GtkWidget *box, *line, *set;
   GtkWidget *button;

   GString *doneLabel;
   
   GtkWidget *label;
   GtkWidget *hour;
   GtkWidget *date, *time, *colon;
   GtkWidget *minute;
   GtkWidget *day;
   GtkWidget *month;
   GtkWidget *year;
   GObject *adj;
   gint *dayHandler, *yearHandler;
   GList *monthList = NULL;
   gint charWidth;
   
   PangoFontMetrics *metrics;
   PangoContext *context;
   
   GtkStyleContext *style_context = gtk_widget_get_style_context(GTK_WIDGET(clocklist));
   PangoFontDescription *desc = gtk_style_context_get_font(style_context,GTK_STATE_FLAG_NORMAL);
   
   context = gtk_widget_get_pango_context (clocklist);
   metrics = pango_context_get_metrics (context,
					desc,
					pango_context_get_language (context));
   charWidth = PANGO_PIXELS( pango_font_metrics_get_approximate_char_width( metrics ) );

   box  = gtk_vbox_new (FALSE, 2);
   
   line = gtk_hbox_new (FALSE, 1);
   
   /* Zone name */
   label = gtk_label_new("");
   gtk_box_pack_start(GTK_BOX(line), label, TRUE, FALSE, 0);  
   gtk_widget_show( label );

   gtk_box_pack_start(GTK_BOX(box), line, FALSE, FALSE, 0);  
   gtk_widget_show( line );
   
   
   line = gtk_hbox_new (FALSE, 1);
   set = gtk_hbox_new (FALSE, 1);
   date = gtk_label_new(_("Date:"));
   gtk_box_pack_start(GTK_BOX(set), date, FALSE, FALSE, 0);  
   gtk_widget_show( date );
   
   /* just use days 1 to 31 for now */
   adj = gtk_adjustment_new (0, 1, 31, 1, 5, 0);
   day = gtk_spin_button_new ((GtkAdjustment *)adj, 0.1, 0);
   gtk_spin_button_set_numeric( (GtkSpinButton *)day, TRUE );
   gtk_spin_button_set_update_policy( (GtkSpinButton *)day, GTK_UPDATE_IF_VALID );
   //  gtk_spin_button_set_shadow_type (GTK_SPIN_BUTTON (day), GTK_SHADOW_ETCHED_IN);
   gtk_entry_set_width_chars( GTK_ENTRY(day), 2 );
   gtk_box_pack_start(GTK_BOX(set), day, FALSE, FALSE, 0);  
   /* must keep record of day change handler to block it
      while the syncBox is being populated with data */
   dayHandler = (gint *) g_malloc( sizeof(gint) );
   *dayHandler = 
       g_signal_connect (G_OBJECT(adj), "value_changed",
			 G_CALLBACK (synchroniseTimes),  (gpointer)clocklist);
   g_object_set_data( G_OBJECT(day), "dayHandler", (gpointer) dayHandler);
   gtk_widget_show( day );
  
  
   month = gtk_combo_box_text_new();
   initialiseMonthList(G_OBJECT(month));
   gtk_box_pack_start(GTK_BOX(set), month, FALSE, FALSE, 2);
   g_signal_connect (G_OBJECT (month), "changed",
		       G_CALLBACK (synchroniseTimes),  (gpointer)clocklist);
   gtk_widget_show( month );
   
   
   /* The time functions only handle years up to 9999, I believe
      so limit the year spin button accordingly */
   adj = gtk_adjustment_new (0, -9999, 9999, 1, 5, 0);
   year = gtk_spin_button_new ((GtkAdjustment *)adj, 0.1, 0);
   gtk_spin_button_set_numeric( (GtkSpinButton *)year, TRUE );
   /*  gtk_spin_button_set_wrap (GTK_SPIN_BUTTON (spinner), TRUE); */
   gtk_spin_button_set_update_policy( (GtkSpinButton *)year, GTK_UPDATE_IF_VALID );
   //  gtk_spin_button_set_shadow_type (GTK_SPIN_BUTTON (year), GTK_SHADOW_ETCHED_IN);
   gtk_entry_set_width_chars( GTK_ENTRY(year), 4 );
   gtk_box_pack_start(GTK_BOX(set), year, FALSE, FALSE, 0);  
   /* must keep record of year change handler to block it
      while the syncBox is being populated with data */
   yearHandler = (gint *) g_malloc( sizeof(gint) );
   *yearHandler = 
      g_signal_connect (G_OBJECT(adj), "value_changed",
			  G_CALLBACK (synchroniseTimes),  (gpointer)clocklist);
   g_object_set_data( G_OBJECT(year), "yearHandler", (gpointer) yearHandler);
   gtk_widget_show( year );
   
   gtk_box_pack_start(GTK_BOX(line), set, TRUE, FALSE, 0);  
   gtk_widget_show( set );
   
   
   gtk_box_pack_start(GTK_BOX(box), line, FALSE, FALSE, 0);  
   gtk_widget_show( line );
   
   
   line = gtk_hbox_new (FALSE, 1);
   set = gtk_hbox_new (FALSE, 1);
   time = gtk_label_new(_("Time:"));
   gtk_box_pack_start(GTK_BOX(set), time, FALSE, FALSE, 0);  
   gtk_widget_show( time );
   
   hour = gtk_entry_new();
   gtk_entry_set_max_length(GTK_ENTRY(hour), 2);
   
   gtk_entry_set_width_chars( GTK_ENTRY(hour), 2 );
   gtk_box_pack_start(GTK_BOX(set), hour, FALSE, FALSE, 0);  
   g_signal_connect (G_OBJECT (hour), "activate",
		       G_CALLBACK (synchroniseTimes),  (gpointer)clocklist);
   gtk_widget_show( hour );
   
   colon = gtk_label_new(":");
   gtk_box_pack_start(GTK_BOX(set), colon, FALSE, FALSE, 0);  
   gtk_widget_show( colon );
      
   minute = gtk_entry_new();
   gtk_entry_set_max_length(GTK_ENTRY(minute), 2);
   
   gtk_entry_set_width_chars( GTK_ENTRY(minute), 2 );
   gtk_box_pack_start(GTK_BOX(set), minute, FALSE, FALSE, 0);  
   g_signal_connect (G_OBJECT (minute), "activate",
		       G_CALLBACK (synchroniseTimes),  (gpointer)clocklist);
   gtk_widget_show( minute );
   
   gtk_box_pack_start(GTK_BOX(line), set, TRUE, FALSE, 0);  
   gtk_widget_show( set );
   
   gtk_box_pack_start(GTK_BOX(box), line, FALSE, FALSE, 0);  
   gtk_widget_show( line );
   
   line = gtk_hbox_new (TRUE, 1);
   /* button to set to new time (for those who dislike the return button */
   button = gtk_button_new_with_label(_("Update View"));
   gtk_box_pack_start(GTK_BOX(line), button, TRUE, FALSE, 0);  
   g_signal_connect (G_OBJECT (button), "clicked",
		       G_CALLBACK (synchroniseTimes),  (gpointer)clocklist);
   gtk_widget_show (button);
   
   /* button to close synchronisation, return to current time */
   if ( rendezvousDoneLabel != NULL )
       doneLabel = g_string_new( rendezvousDoneLabel->str );
   else /* normal behaviour */
       doneLabel = g_string_new( _("Done") );
   button = gtk_button_new_with_label( doneLabel->str );
   g_string_free( doneLabel, TRUE );

   gtk_box_pack_start(GTK_BOX(line), button, TRUE, FALSE, 0);  
   g_signal_connect (G_OBJECT (button), "clicked",
		       G_CALLBACK(unsynchroniseFromDone),  (gpointer)clocklist);
   gtk_widget_show (button);
   
   gtk_box_pack_start(GTK_BOX(box), line, FALSE, FALSE, 0);  
   gtk_widget_show( line );
   
   g_object_set_data( G_OBJECT(box), "label", (gpointer) label);
   g_object_set_data( G_OBJECT(box), "day", (gpointer) day);
   g_object_set_data( G_OBJECT(box), "month", (gpointer) month);
   g_object_set_data( G_OBJECT(box), "year", (gpointer) year);
   g_object_set_data( G_OBJECT(box), "hour", (gpointer) hour);
   g_object_set_data( G_OBJECT(box), "minute", (gpointer) minute);
   
   return (GtkBox *)box;
}

