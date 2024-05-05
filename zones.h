/* functions related to selecting time zones */

#ifndef GWORLDCLOCK_ZONES
#define GWORLDCLOCK_ZONES

#include <gtk/gtk.h>

static const gchar *ZONE_TABLE="/usr/share/zoneinfo/zone.tab";
static const gchar *COUNTRY_TABLE="/usr/share/zoneinfo/iso3166.tab";

#define ZONE_DATA "ZoneData"

/* entries in tree holding continent-region-city information */
enum
{
   CONTINENT_NAME,
   COUNTRY_NAME,
   REGION_NAME,
   CODE,
   N_COLUMNS
};

static const gchar *defaultContinent = "Australia";
static const gchar *defaultCountry = "Australia";
static const gchar *defaultRegion = "Sydney";

static const gchar *continents[] = 
{ "Africa",
  "Americas",
  "Antarctica",
  "Arctic Ocean",
  "Asia",
  "Atlantic Ocean",
  "Australia",
  "Europe",
  "Indian Ocean",
  "Pacific Ocean" };

/* Keep this number up to date with the number of continents in the
   array above.  Not that the number of continents on Earth is going
   to change, but you know.  */ 

static const gint Ncontinents = 10;  


typedef struct NameCodeType {
  gchar *name;
  gchar *code;  /* actually only 2 characters needed, but 3 would be better */
} NameCodeType;

typedef struct AddZoneStruct 
{
    GObject *clocklist;
    GtkTreeView *countryTreeView;
    GtkTreeView *regionTreeView;
    GtkWidget *DescriptionEntry;
    GtkWidget *TZEntry;
} AddZoneStruct;



void DeleteZone( GtkWidget *w, gpointer clocklist );

/*  Save list of time zones to configfile */
gint SaveZones(GtkWidget *w, gpointer clocklist);

/* Handle "rows_reordered" signal, indicating the rows in the clock have been
   moved */
/* Note this callback function does not get  called for some reason,
   I don't know why.  A bug in GTK+ ? 
   You will have to save the reordered list by hand for the time being.
*/
void registerReorderedRows( GtkTreeModel* clocklistModel,
			     GtkTreePath *arg1,
			     GtkTreeIter *arg2,
			     gpointer new_order,
			    gpointer user_data);

gint CodeInList(gchar *code, GSList *List);

GSList* AddNameCodeEntry(gchar *code, gchar *name, GSList *List);

void  ClearNameCodeList(GSList **List);

/* for given continent, find corresponding countries as identified in ZONE_TABLE
   and prepare list of country name using COUNTRY_TABLE
*/
GSList* FetchCountries(gchar *continent);

/* from given country code ("*country"), find list of regions in ZONE_TABLE */
/* input: country is the two-letter country code from ISO3166 */
GSList* FetchRegions(gchar *country);

void UpdateCountries(GtkTreeSelection *selection, gpointer ZoneNotes);
void UpdateRegions(GtkTreeSelection *selection, gpointer ZoneNotes);
void SelectRegion(GtkTreeSelection *selection, gpointer ZoneNotes);


void UpdateCountriesCL(GtkWidget *ContinentCList,
		     gint row,
		     gint column,
		     GdkEventButton *event,
		     gpointer ZoneData);


/* Implements GtkTreeModelFilterVisibleFunc, making continent entries
   only visible.
*/
gboolean FilterContinents(GtkTreeModel *model,
			  GtkTreeIter *iter,
			  gpointer data);

gboolean FilterCountries(GtkTreeModel *model,
			 GtkTreeIter *iter,
			 gpointer data);

gboolean FilterRegions(GtkTreeModel *model,
		       GtkTreeIter *iter,
		       gpointer data);




/* when left mouse button is double-clicked,
   send "key-pressed-event" to one of the Entry boxes 
   which will be handled by adding the given zone.
   We're assuming here that "select-row" preceded the double-click event */
gint ButtonPressedInRegionList(GtkWidget *regionlist, 
			       GdkEventButton *event, gpointer ZoneData);

/* zones are selected according to the method used in tzselect:
   First the continent is chosen, then, if necessary, the country is chosen,
   with countries being identified from the two-letter code in the
   entries of  [/usr/share/zoneinfo/]zone.tab (and country names taken from 
   iso3166.tab)  Then the region (or city) of that country is identified, from 
   zone.tab.
*/
void PrepareZoneNotes(GtkWidget **ZoneNotes, AddZoneStruct *Zone);

void AddZoneToList(GtkWidget *w, gpointer NewZone);

void AddZone( GtkWidget *w, gpointer clocklist );

void WriteZoneDescription(GtkDialog *dialog, gint responseId, gpointer Zone);

void ChangeZoneDescription(GtkWidget *w, gpointer clocklist);

#endif
