/* functions related to time zone synchronisation 
   (i.e. "Rendezvous")
  
   For simplicity, and because the phrase "synchronisation" made sense to me,
   I'll leave internal references to "sync" without changing them to 
   "rendezvous".
*/

#ifndef GWORLDCLOCK_SYNC
#define GWORLDCLOCK_SYNC

#include <gtk/gtk.h>
#include <time.h>

static const gint SYNCBOX_HEIGHT=145;

/* this data is used to construct the time when synchronising */
typedef struct SyncData {
  gchar *zoneName;
  gint hour;
  gint minute;
  gint date;
  gint month;
  gint year;
} SyncData;


/* returns name of month (as defined by locale, converted to UTF-8)
   corresponding to given index (between 1 and 12) */
gchar* getNameOfMonth( gint index );

/* returns the index (between 1 and 12 ) of the given month,
   identified by name (for current locale, but encoded in UTF-8) */
gint getMonthIndex( gchar *monthName );

/* set the display values for date and time in the SyncBox to the current time */
void setDefaultSyncValues(gchar *description, gchar *timezone);

/* Allows menu selection to turn Rendezvous on and off. */
void ToggleRendezvous(GtkToggleAction *action, gpointer clocklist);

void Rendezvous(GtkWidget *w, gpointer clocklist);

/* reads the values set in the syncBox fields and turns them into
   a time_t time.
   Note: this time_t crashes (mktime returns -1) at 6:45am, 14 Dec 1901
   At 6:46am, t=-2147483640 (2^31=2147483648) - 32bit int limitation */
time_t extractSyncTime();

/* callback function to be run if any of the sync fields are changed.
   Updates the times in each zone to the new sync time. 
   The changed field is referenced as field, but is not used as such.
*/
void synchroniseTimes( GtkWidget *field, gpointer clocklist);

/* after done synchronising zones, return back to current time */
void unsynchronise( GtkWidget *button, gpointer clocklist);

void initialiseMonthList(GObject *months);

/* construct the box in which the controls for synchronising dates will be 
   found */
GtkBox *constructSyncBox(gpointer clocklist);

#endif
