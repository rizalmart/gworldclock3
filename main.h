#ifndef GWORLDCLOCK_MAIN
#define GWORLDCLOCK_MAIN

#include "zones.h"
#include "rendezvous.h"
#include "options.h"

#define MAIN_WINDOW "main_window"
#define UI_MANAGER "ui_manager"
#define ACTIONS "actions"


/* two display columns: Name of zone, and time/date.
   This could be adjusted later to separate time from date.
   Also, store the TZ name here.
*/
enum
{
   TZ_NAME,
   TZ_DESCRIPTION,
   TZ_TIMEDATE,
   LIST_COLUMNS
};


void GetOptions( int argc, char **argv );

void AboutDialog( GtkWidget *w, gpointer clocklist );

/* ensure we go through the same save routine when quitting from the menu (C-Q)
   or by pressing the window's 'close' button */
void send_clock_quit(GtkWidget *w,  gpointer clocklist );

/* save the config data when quitting, if necessary */
gint worldclock_quit(GtkWidget *widget,
		     GdkEvent  *event,
		     gpointer   clocklist );

/* Process mouse click when a zone in the list is selected.
   Double-click with left mouse button (button 1) pops up the 
   "Change Description" dialog box.
   Single-click with right button (button 3) pops up the menu of actions.
*/
static gboolean ButtonPressedInList (GtkWidget *clocklist, 
				     GdkEventButton *event,
				     gpointer selection);

#define N_(A) A

static GtkActionEntry entries[] = {
  { "File", NULL, N_("_File") },
  /* don't allow zones to be saved via Ctrl-S, in order to keep accidental
     changes from being saved */
  { "SaveZones",  NULL, N_("_Save Zones"),     NULL, NULL, G_CALLBACK(SaveZones) },
  { "Preferences", NULL, N_("_Preferences"),  NULL,  NULL, G_CALLBACK(ChangePreferences) },
  { "Quit", NULL, N_("_Quit"),     "<control>Q", NULL, G_CALLBACK(send_clock_quit) },
  { "Options", NULL, N_("_Options") },
  { "AddTimezone", NULL, N_("_Add Timezone"),  "<control>A", NULL, G_CALLBACK(AddZone) },
  { "DeleteTimezone", NULL, N_("_Delete Timezone"),  "<control>D", NULL, G_CALLBACK(DeleteZone) },
  { "ChangeDescription", NULL, N_("_Change Description"),  NULL,  NULL, G_CALLBACK(ChangeZoneDescription) },
  { "Rendezvous", NULL, N_("_Rendezvous"),  NULL,  NULL, G_CALLBACK(Rendezvous) },
  { "Help", NULL, N_("_Help") },
  { "About", NULL, N_("_About"),   NULL,         NULL, G_CALLBACK(AboutDialog) }
};

static GtkToggleActionEntry toggle_entries[] = {
  { "ToggleRendezvous", NULL, N_("_Rendezvous"),  NULL,  NULL, G_CALLBACK(ToggleRendezvous), FALSE }
};



static const char *ui_description = 
"<ui>"
"  <menubar name='MainMenu'>"
"    <menu action='File'>"
/*"      <menuitem action='OpenZones' />"*/
"      <menuitem action='SaveZones' />"
/*"      <menuitem action='SaveZonesAs' />"*/
"      <menuitem action='Preferences' />"
"      <menuitem action='Quit' />"
"    </menu>"
"    <menu action='Options' >"
"      <menuitem action='AddTimezone' />"
"      <menuitem action='DeleteTimezone' />"
"      <menuitem action='ChangeDescription' />"
"      <menuitem action='ToggleRendezvous' />"
"    </menu>"
"    <menu action='Help' >"
"      <menuitem action='About' />"
"    </menu>"
"  </menubar>"
"  <popup name='PopupMenu'>"
"      <menuitem action='Rendezvous' />"
"      <menuitem action='AddTimezone' />"
"      <menuitem action='DeleteTimezone' />"
"      <menuitem action='ChangeDescription' />"
"  </popup>"
"</ui>";



#endif
