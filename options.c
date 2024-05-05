/* Processes user options from the command line or from the option file
~/.gworldclock.

.gworldclock is an XML file, with default structure:

<?xml version="1.0"?>
<gworldclock>
  <timeDisplayFormat>%c</timeDisplayFormat>
  <rendezvous>
     <doneLabel>Return</doneLabel>
  </rendezvous>
</gworldclock>

<timeDisplayFormat> contains the date format string used to display
data&time.

<doneLabel> inside <rendezvous> provides a means to specify an
alternate label for the "Done" button (the button which closes the
Rendezvous interface and returns to normal clock operation).

In case you were wondering whether name/value pairs would have been simpler 
than XML for storing user options, my simple response is that I wanted to learn
how to use XML from within C.

*/



#include <stdlib.h>
#include <time.h>

#include <unistd.h> /* getopt */
#include <gtk/gtk.h>
#include <libxml/parser.h>
#include <libintl.h>
#include <string.h>
#include <strings.h>

#include "timer.h"
#include "resize.h"

#define _(A) gettext(A)

#define LOCALE_DEFAULT_DISPLAY "%c"
#define SHORT_DISPLAY "%x %l:%M%P"
#define SHORT_DISPLAY_24HOUR "%x %R"

gchar *defaultOptionFile="gworldclock";
gchar *defaultConfigFilename="tzlist";
gchar *defaultTimeDisplayFormat = LOCALE_DEFAULT_DISPLAY;
extern gchar *defaultRendezvousUIFormat;

gboolean changedPreferences = FALSE;
GtkEntry *rollYourOwnDisplayFormat=NULL;

GString *oldTimeDisplayFormat;

#define XMLVERSION  "1.0"
#define ROOT_NODE   "gworldclock"
#define TIME_DISPLAY_FORMAT_NODE "timeDisplayFormat"
#define RENDEZVOUS_NODE "rendezvous"
#define RENDEZVOUS_DONELABEL_NODE "doneLabel"

GString *getTimeDisplayFormat( xmlDocPtr optionXML )
{
   GString *format=NULL;
   xmlNodePtr cur;
   xmlChar *formatText;

   cur = xmlDocGetRootElement(optionXML);
   if (cur != NULL) 
   {
      if (xmlStrcmp(cur->name, ROOT_NODE)) {
	 fprintf(stderr,"Options file incorrectly formatted\n");
	 return NULL;
      }
      
      cur = cur->xmlChildrenNode;
      while ( (cur != NULL) && (format==NULL) ) 
      {
	 if ((!xmlStrcmp(cur->name, TIME_DISPLAY_FORMAT_NODE))) 
	 {
	    formatText = xmlNodeListGetString(optionXML, 
					      cur->xmlChildrenNode, 1);
	    format = g_string_new(formatText);
	    xmlFree(formatText);
	    return format;
	 }
	 cur = cur->next;
      }

   }   

   return format;
}

GString *getRendezvousDoneLabel( xmlDocPtr optionXML )
{
   GString *format=NULL;
   xmlNodePtr cur;
   xmlChar *formatText;

   cur = xmlDocGetRootElement(optionXML);
   if (cur != NULL) 
   {
      if (xmlStrcmp(cur->name, ROOT_NODE)) {
	 fprintf(stderr,"Options file incorrectly formatted\n");
	 return NULL;
      }
      
      cur = cur->xmlChildrenNode;
      while ( (cur != NULL) && (format==NULL) ) 
      {
	 if ((!xmlStrcmp(cur->name, RENDEZVOUS_NODE))) 
	 {

	     cur = cur->xmlChildrenNode;
	     while ( (cur != NULL) && (format==NULL) ) 
	     {
		 if ((!xmlStrcmp(cur->name, RENDEZVOUS_DONELABEL_NODE))) 
		 {
		     formatText = xmlNodeListGetString(optionXML, 
						       cur->xmlChildrenNode, 1);
		     format = g_string_new(formatText);
		     xmlFree(formatText);
		     return format;
		 }
		 cur = cur->next;
	     }
	 }
	 cur = cur->next;
      }

   }   

   return format;
}

void GetOptions( int argc, char **argv )
{
  extern GString *configfile, *displayFormat, *optionFile;
  extern GString *rendezvousDoneLabel;
  int c;
  xmlDocPtr optionXML;
  GString *temp;


  /* set defaults */
  configfile = g_string_new(g_strdup((gchar *) getenv("XDG_CONFIG_HOME")));  
  g_string_append(configfile,"/");
  g_string_append(configfile,defaultConfigFilename);

  displayFormat = g_string_new( g_strdup( defaultTimeDisplayFormat ) );

 /* read configuration options from file */
  optionFile = g_string_new(g_strdup((gchar *) getenv("XDG_CONFIG_HOME")));  
  g_string_append(optionFile,"/");
  g_string_append(optionFile,defaultOptionFile);
  optionXML = xmlParseFile( optionFile->str );
  if ( optionXML!=NULL )
  {
     temp = getTimeDisplayFormat(optionXML);
     if ( temp != NULL ) 
     {
	g_string_free( displayFormat, TRUE );
	displayFormat = temp;
     }

     temp = getRendezvousDoneLabel(optionXML);
     if ( temp != NULL ) 
     {
	 if ( rendezvousDoneLabel != NULL )
	     g_string_free( rendezvousDoneLabel, TRUE );
	rendezvousDoneLabel = temp;
     }

  }

  if ( optionXML!=NULL )
     xmlFreeDoc(optionXML);


  /* set command line options */
  while ((c = getopt (argc, argv, "f:")) != -1)
     switch (c) 
     {
	case 'f':
	   configfile = g_string_new(g_strdup((gchar *)optarg));  
	   break;
	   
	default:
	break;
     }

}

/* save options to options file (default ~/.gworldclock) 
   - creates the file if it does not already exist
*/
gint SaveOptions()
{
   extern GString *optionFile, *displayFormat;
   xmlDocPtr optionXML;
   xmlNodePtr root, cur, foundNode=NULL;
   xmlChar *formatText;
   gint save = FALSE;

   optionXML = xmlParseFile( optionFile->str );
   if ( optionXML==NULL ) /* create new document */
   {
      optionXML = xmlNewDoc(XMLVERSION);
      root = xmlNewDocRawNode( optionXML, NULL, ROOT_NODE, NULL ); 
      xmlDocSetRootElement( optionXML, root );
   }
   else
   {
      root = xmlDocGetRootElement(optionXML);
   }
   
   cur = root->xmlChildrenNode;
   while ( (cur != NULL) && (foundNode==NULL) ) 
   {
      if ((!xmlStrcmp(cur->name, TIME_DISPLAY_FORMAT_NODE))) 
      {
	 foundNode=cur;
      }
      cur = cur->next;
   }

   if (foundNode==NULL)
   {
      save = TRUE;
      foundNode = xmlNewTextChild( root, NULL, 
				   TIME_DISPLAY_FORMAT_NODE,
				   displayFormat->str );
   }
   else
   {
      formatText = xmlNodeListGetString(optionXML, 
					foundNode->xmlChildrenNode, 1);
      if(strcasecmp(formatText,displayFormat->str))
      {
	 save = TRUE;
	 xmlNodeSetContent( foundNode, displayFormat->str );
      }
      xmlFree(formatText);
   }

   if (save)
   {
      xmlSaveFormatFile( optionFile->str, optionXML, 1 );
   }

   xmlFreeDoc(optionXML);
   
   return 1;
}

void StoreOldPreferences()
{
   extern GString *displayFormat;
   if ( oldTimeDisplayFormat == NULL )
   {
      oldTimeDisplayFormat = g_string_new( displayFormat->str);
   }
   else
   {
      oldTimeDisplayFormat = g_string_assign( oldTimeDisplayFormat, 
					      displayFormat->str);
   }
}


void updateDisplayFormat( GString *newDisplayFormat, gpointer clocklist )
{
   extern GString *displayFormat;
   
   displayFormat = g_string_assign( displayFormat, 
				    newDisplayFormat->str);

   /* restart timer, in case displayFormat was changed to require or
      not require seconds */
   reset_timer(clocklist);

   // update display now, in case timer is set to 60 sec, or else
   // new display will not be seen for another minute
   SetTime(clocklist); 

   /* update column size to adapt to new date format */
   resizeWindow( gtk_widget_get_toplevel( GTK_WIDGET(clocklist) ), clocklist);
   

   changedPreferences = TRUE;

}

void RestoreOldPreferences( gpointer clocklist )
{
   extern GString *displayFormat;

   if ( ! g_string_equal( displayFormat, oldTimeDisplayFormat ) )
   {
       updateDisplayFormat( oldTimeDisplayFormat, clocklist );
   }
}

/* Updates the user's preferences. */ 
void ProcessPreferences( GObject *owner, gpointer clocklist )
{
   extern GString *displayFormat;

   GString *newDisplayFormat;
   GSList *radioGroup;
   GtkEntry *entry;

   radioGroup =  gtk_radio_button_get_group( 
      g_object_get_data(owner, "radioDisplayFormat") );

   while( radioGroup != NULL )
   {
      if ( gtk_toggle_button_get_active(
	      GTK_TOGGLE_BUTTON(radioGroup->data) ) )
      {
	 newDisplayFormat = g_string_new( 
	    g_object_get_data( G_OBJECT(radioGroup->data),
			       "displayFormat" ) );
	 break;
      }
      else
	 radioGroup = radioGroup->next;
   }

   if (!strcmp(newDisplayFormat->str, "rollYourOwn") )
   {
      entry = g_object_get_data(owner, "entryRollYourOwnDisplayFormat");
      newDisplayFormat = g_string_assign( newDisplayFormat, 
				        gtk_entry_get_text( entry ) );
   }

   if ( ! g_string_equal( displayFormat, newDisplayFormat ) )
   {
       updateDisplayFormat( newDisplayFormat, clocklist );
   }
   g_string_free( newDisplayFormat, TRUE );
}

void SavePreferences()
{
   extern GString *displayFormat;

   if (changedPreferences)
   {
      SaveOptions();
      changedPreferences = FALSE;
   }
}

void ProcessPreferenceResponse( GtkDialog *dialog,
			 gint response,
			 gpointer clocklist)
{
   switch (response)
   {
      case GTK_RESPONSE_OK:
	 /* process preferences and close */
         ProcessPreferences(G_OBJECT(dialog), clocklist);
	 SavePreferences();
	 gtk_widget_destroy(GTK_WIDGET(dialog));
         break;
      case GTK_RESPONSE_APPLY:
	 /* process preferences, keep dialog box open */
         ProcessPreferences(G_OBJECT(dialog), clocklist);
         break;
      case GTK_RESPONSE_CLOSE:
      case GTK_RESPONSE_DELETE_EVENT:
	 SavePreferences();
	 gtk_widget_destroy(GTK_WIDGET(dialog));
         break;
      case GTK_RESPONSE_CANCEL:
	 /* restore original preferences and close */
	 RestoreOldPreferences(clocklist);
	 SavePreferences();
	 gtk_widget_destroy (GTK_WIDGET(dialog));
         break;
      default:
         /* unknown response, do nothing */
         break;
   }
}

/* Lays out the various display options, 
   as radio buttons inside the container.
   A reference to the Display Format radio buttons is stored in owner with
   key "radioDisplayFormat".
   The user's format Entry Box is stored under "entryRollYourOwnDisplayFormat".
   The format string for each radio button is stored with it under key
   "displayFormat".
*/
void SetDisplayAlternatives( GtkContainer *container, GObject *owner )
{
   extern GString *displayFormat;

   GtkWidget *radio, *entry;
   gboolean foundFormat = FALSE;
   GString *text;
   time_t currenttime;
   struct tm *tm;
   char rawTimeDisplay[TIME_DISPLAY_SIZE];
   gchar *timeDisplay;

   time(&currenttime);
   tm = (struct tm *) localtime( (time_t *) &currenttime );

   strftime (rawTimeDisplay, TIME_DISPLAY_SIZE, 
	     LOCALE_DEFAULT_DISPLAY, 
	     tm      );
   timeDisplay = g_locale_to_utf8 ( rawTimeDisplay,
				    -1, NULL, NULL, NULL );   
   text = g_string_new( _("Locale default") );
   text = g_string_append_c( text, ' ' );
   text = g_string_append( text, _("e.g.") );
   text = g_string_append_c( text, ' ' );
   text = g_string_append( text, timeDisplay );
   radio = gtk_radio_button_new_with_label( NULL, text->str );
   g_object_set_data( G_OBJECT(radio), "displayFormat", 
		      LOCALE_DEFAULT_DISPLAY );
   gtk_container_add ( container, radio);
   if( ! strcmp(displayFormat->str,  LOCALE_DEFAULT_DISPLAY) )
   {
      gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON(radio), TRUE );
      foundFormat = TRUE;
   }
   g_free( timeDisplay );
   g_string_free(text, TRUE);
   g_object_set_data( owner, "radioDisplayFormat", (gpointer) radio );

   strftime (rawTimeDisplay, TIME_DISPLAY_SIZE, 
	     SHORT_DISPLAY, 
	     tm      );
   timeDisplay = g_locale_to_utf8 ( rawTimeDisplay,
				    -1, NULL, NULL, NULL );   
   text = g_string_new( _("Short Form") );
   text = g_string_append_c( text, ' ' );
   text = g_string_append( text, _("e.g.") );
   text = g_string_append_c( text, ' ' );
   text = g_string_append( text, timeDisplay );
   radio = gtk_radio_button_new_with_label_from_widget( GTK_RADIO_BUTTON(radio), text->str );
   g_object_set_data( G_OBJECT(radio), "displayFormat", 
		      SHORT_DISPLAY );
   gtk_container_add ( container, radio);
   if( ! strcmp(displayFormat->str,  SHORT_DISPLAY) )
   {
      gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON(radio), TRUE );
      foundFormat = TRUE;
   }
   g_free( timeDisplay );
   g_string_free(text, TRUE);

   strftime (rawTimeDisplay, TIME_DISPLAY_SIZE, 
	     SHORT_DISPLAY_24HOUR, 
	     tm      );
   timeDisplay = g_locale_to_utf8 ( rawTimeDisplay,
				    -1, NULL, NULL, NULL );   
   text = g_string_new( _("Short Form (24 Hour)") );
   text = g_string_append_c( text, ' ' );
   text = g_string_append( text, _("e.g.") );
   text = g_string_append_c( text, ' ' );
   text = g_string_append( text, timeDisplay );
   radio = gtk_radio_button_new_with_label_from_widget( GTK_RADIO_BUTTON(radio), text->str );
   g_object_set_data( G_OBJECT(radio), "displayFormat", 
		      SHORT_DISPLAY_24HOUR );
   gtk_container_add ( container, radio);
   if( ! strcmp(displayFormat->str,  SHORT_DISPLAY_24HOUR) )
   {
      gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON(radio), TRUE );
      foundFormat = TRUE;
   }
   g_free( timeDisplay );
   g_string_free(text, TRUE);


   text = g_string_new( _("Roll Your Own") );
   radio = gtk_radio_button_new_with_label_from_widget( GTK_RADIO_BUTTON(radio), text->str );
   g_object_set_data( G_OBJECT(radio), "displayFormat", 
		      "rollYourOwn" );
   gtk_container_add ( container, radio);
   g_string_free(text, TRUE);

   entry = gtk_entry_new();
   gtk_entry_set_width_chars( GTK_ENTRY(entry), 15 ); /* ignored!  why?! ;( */
   gtk_entry_set_text( GTK_ENTRY(entry), displayFormat->str );
   gtk_container_add ( container, entry);
   rollYourOwnDisplayFormat = GTK_ENTRY( entry );   
   gtk_widget_show (entry);
   g_object_set_data( owner, "entryRollYourOwnDisplayFormat", 
		      (gpointer) rollYourOwnDisplayFormat);
   if( ! foundFormat )
      gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON(radio), TRUE );
}


/* dialog box allows user to set preferences
   e.g. preferred display format for time and date */
void ChangePreferences(GtkWidget *w, gpointer clocklist)
{
   GtkWidget *dialog, *frame, *box;

   StoreOldPreferences();
   dialog = gtk_dialog_new_with_buttons (_("Preferences"),
                                         NULL,
                                         GTK_DIALOG_DESTROY_WITH_PARENT,
                                         GTK_STOCK_OK,
					 GTK_RESPONSE_OK,
                                         GTK_STOCK_APPLY,
					 GTK_RESPONSE_APPLY,
					 GTK_STOCK_CLOSE,
					 GTK_RESPONSE_CLOSE,
					 GTK_STOCK_REVERT_TO_SAVED,
					 GTK_RESPONSE_CANCEL,
                                         NULL);
   g_signal_connect (G_OBJECT (dialog),
		     "response",
		     G_CALLBACK (ProcessPreferenceResponse),
		     (gpointer)clocklist);
   
   frame = gtk_frame_new( _("Time Display Format") );
   box = gtk_vbox_new( FALSE, 0 );
   SetDisplayAlternatives( GTK_CONTAINER( box ), G_OBJECT(dialog) );
   gtk_container_add (GTK_CONTAINER(frame), box);

   GtkWidget *content_area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
   gtk_container_add(GTK_CONTAINER(content_area), frame);
  

//   gtk_container_add (GTK_CONTAINER (GTK_DIALOG(dialog)->vbox),
//                      frame);

   gtk_widget_show_all (dialog);
}
