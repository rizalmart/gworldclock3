#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <gtk/gtk.h>
#include <time.h>
#include <string.h>

#include "main.h"
#include "timer.h"
#include "misc.h"

/*  display format for time&date  */
GString *displayFormat;


/* functions related to running the timer, displaying the times in each zone */
void  start_clocks(gpointer clocklist)
{
  FILE *cf;
  extern gint changed;
  extern GString *configfile;
  extern gchar *defaultConfigFilename;
  GtkTreeModel *clocklistModel;

  /* format of list:
     name of time zone (TZ variable), description, time in zone
  */
  gchar *rowdata[2], *localstr="Local";
  /* is there a way of reading the information without hardwiring '200' here?
     eg can flag 'a' in sscanf work with %[ as well as %s ? */
  gchar description[200], *timezone, line[200];
  int row;
  GString *title, *msg;
  gchar *button[]={"OK"};
  GtkTreeIter   iter;

  /* ??? associate configfile with list as widget data ?? */

  rowdata[1]=NULL; /* necessary to set to NULL, or segfault related
		      to List Reordering results */
  

  clocklistModel = gtk_tree_view_get_model( GTK_TREE_VIEW(clocklist) );
  cf=fopen(configfile->str,"r");
  if (cf) 
  {
    while(fgets(line,200,cf)) 
    {

      /* configfile is assumed to have two entries per line,
	 the TZ value followed by a description enclosed in quotation marks.
	 If the description is missing, the TZ string is used as a description instead */
      /* the second entry looks complicated, but just copies the whole set of
	 characters between two quotation marks
	 [finds the first '"', accounting for any spaces or tabs, 
	 then grabs everything up to the second '"'] */

      if ( sscanf(line,"%ms %*[ \t\"]%[^\"]",&timezone,description) < 2 )
	strncpy(description,timezone,200);

      gtk_list_store_append ( GTK_LIST_STORE(clocklistModel), &iter);
      gtk_list_store_set ( GTK_LIST_STORE(clocklistModel), &iter,
			   TZ_NAME, timezone,
			   TZ_DESCRIPTION, description,
			   -1);     
    }
    fclose(cf);
  } 
  else 
  {
     /* ignore error if it simply means the zone file does not (yet) exist,
      otherwise, report it to the user */
     if ( errno != ENOENT ) 
     {
	  title = g_string_new("Read Zone File");
	  msg = g_string_new(NULL);
	  g_string_sprintf(msg," Error reading zone file \"%s\": \n %s \n",
		     configfile->str,  g_strerror(errno) );
	  showMessageDialog( msg->str, GTK_MESSAGE_ERROR );
	  g_string_free(msg,TRUE);
	  g_string_free(title,TRUE);
     }
     
     gtk_list_store_append ( GTK_LIST_STORE(clocklistModel), &iter);
     gtk_list_store_set ( GTK_LIST_STORE(clocklistModel), &iter,
			  TZ_NAME, localstr,
			  TZ_DESCRIPTION, localstr,
			  -1);     
    changed=1;
  }
  SetTime(clocklist);
}

gint SetTime(gpointer clocklist)
{
   time_t currenttime;

   time(&currenttime);

   gtk_widget_freeze_child_notify( GTK_WIDGET(clocklist) );

   gtk_tree_model_foreach( gtk_tree_view_get_model(GTK_TREE_VIEW(clocklist)),
			   (GtkTreeModelForeachFunc) SetToGivenTime,
			   (gpointer) currenttime );

   gtk_widget_thaw_child_notify( GTK_WIDGET(clocklist) );

   return 1;
}

gboolean SetToGivenTime  (GtkTreeModel *clocklistModel,
			  GtkTreePath *path,
			  GtkTreeIter *iter,
			  gpointer timeToSet)
{
  gchar *timezone, *TZdefault;
  gint N,i;
  char rawTimeDisplay[TIME_DISPLAY_SIZE];
  gchar *timeDisplay;

  TZdefault = (gchar *)getenv("TZ");
  gtk_tree_model_get( clocklistModel, iter,
		      TZ_NAME, &timezone,
		      -1);

  if(strcasecmp(timezone,"Local"))
    setenv("TZ",timezone,1);
  else 
    {
    /* local time is set by the ordinary value of TZ */
    if (TZdefault)
      setenv("TZ",TZdefault,1);
    else
      unsetenv("TZ");
  }

  strftime (rawTimeDisplay, TIME_DISPLAY_SIZE, 
	    displayFormat->str, 
	    localtime( (time_t *) &timeToSet ) 
     );
  timeDisplay = g_locale_to_utf8 ( rawTimeDisplay,
				   -1, NULL, NULL, NULL );

  gtk_list_store_set ( GTK_LIST_STORE(clocklistModel), iter,
		       TZ_TIMEDATE, timeDisplay,
		       -1);     

  g_free( timeDisplay );

  if (TZdefault)
    setenv("TZ",TZdefault,1);
  else
    unsetenv("TZ");

  g_free(timezone);

  return FALSE;
}

gint start_timer( gpointer clocklist )
{
    /* by default update every minute = 60,000 ms */
    int beatInterval = 60000;

    /* 
       Change to seconds beat when needed.
       All the date formats which show seconds are listed here. 
    */
    if ( strstr( displayFormat->str, "%c" ) ||
	 strstr( displayFormat->str, "%N" ) ||
	 strstr( displayFormat->str, "%r" ) ||
	 strstr( displayFormat->str, "%s" ) ||
	 strstr( displayFormat->str, "%S" ) ||
	 strstr( displayFormat->str, "%T" ) ||
	 strstr( displayFormat->str, "%X" )   )
    {
	beatInterval = 1000;
    }

    return g_timeout_add( beatInterval, SetTime, clocklist);
}

void stop_timer()
{
   extern gint timer;
   g_source_remove( timer );

   /* set timer to -1 to show that there is no timer, and therefore
      synchronisation is taking place */
   timer = -1;
}

void reset_timer( gpointer clocklist )
{
    extern gint timer;
    stop_timer();
    timer = start_timer( clocklist );
}
