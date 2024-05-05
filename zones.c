/* functions related to selecting time zones */

#include <stdio.h>
#include <errno.h>
#include <gtk/gtk.h>
#include <libintl.h>
#include <string.h>

#include "gworldclock.h"
#include "zones.h"
#include "misc.h"
#include "resize.h"
#include "timer.h"

#define _(A) gettext(A)


void DeleteZone( GtkWidget *w, gpointer clocklist )
{
   extern gint changed;
   GString *title, *msg;
   gchar *button[]={"OK"};
   GtkTreeModel *clocklistModel;
   GtkTreeIter iter;

   if ( gtk_tree_selection_get_selected( 
	   gtk_tree_view_get_selection( clocklist ),
	   &clocklistModel,
	   &iter ) )
   {
      gtk_list_store_remove( GTK_LIST_STORE(clocklistModel), &iter );
      changed = 1;
      resizeWindow( gtk_widget_get_toplevel( GTK_WIDGET(clocklist) ), 
		    clocklist );
   }
   else 
   {
      title = g_string_new(_("Delete Zone"));
      msg = g_string_new(_("No zone chosen for deleting."));
      showMessageDialog( msg->str, GTK_MESSAGE_WARNING );
      g_string_free(msg,TRUE);
      g_string_free(title,TRUE);
   }
}


/*  Save list of time zones to configfile */
gint SaveZones(GtkWidget *w, gpointer clocklist)
{
  FILE *cf;
  extern GString *configfile;
  extern gint changed;
  gint N,i;
  gchar *description, *timezone;
  GtkTreeModel *clocklistModel;
  GtkTreeIter iter;
  gboolean gotIter = FALSE;

  if ( !(cf=fopen(configfile->str,"w")) ) {
    return 0;
  }

  clocklistModel = gtk_tree_view_get_model( clocklist );
  gotIter =  gtk_tree_model_get_iter_first(clocklistModel, &iter);

  while( gotIter )
  {
     gtk_tree_model_get( clocklistModel, &iter,
			 TZ_NAME, &timezone,
			 TZ_DESCRIPTION, &description,
			 -1);

    /* only write description if there is one! */
    if(strlen(description)==0)
      fprintf(cf,"%s\n", timezone );
    else
       fprintf(cf,"%s    \"%s\"\n", timezone, description );

    g_free(timezone);
    g_free(description);
    gotIter = gtk_tree_model_iter_next( clocklistModel, &iter );
  }

  changed=0;
  return ( ! fclose( cf ) );
}

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
			     gpointer user_data)
{
  extern gint changed;

  /* mark clocklist as "changed" */
  changed = 1;
}

gint CodeInList(gchar *code, GSList *List)
{
  /* can't use g_slist_find, unfortunately, since the data in the list is both the
     country code and (unknown) country name
  */

  GSList *item;

  item = List;
  while (item) {
    if ( ! strcmp(code,((NameCodeType *)item->data)->code) )
      return TRUE;  /* found match */
    item = item->next;
  }
  return FALSE;
}

/* GList GCompareFunc to sort list after name */
gint alphabetical_GCompareFunc(gconstpointer a, gconstpointer b)
{
  NameCodeType *aentry = ((NameCodeType *) a);
  NameCodeType *bentry = ((NameCodeType *) b);

  return strcmp(aentry->name, bentry->name);
}

GSList* AddNameCodeEntry(gchar *code, gchar *name, GSList *List)
{
  NameCodeType *entry;
 
  entry = g_malloc0(sizeof(NameCodeType));
  if(!entry)
    g_print(_("Could not create list: %s"),g_strerror(errno));
  entry->name = g_strdup(name);
  entry->code = g_strdup(code);
  List = g_slist_insert_sorted(List, (gpointer)entry, alphabetical_GCompareFunc);
  /* we don't free entry here do we?  It's on record and is only to be freed when
     the item is released from the list */
  return List;
}

/* the documentation is not too clear about allocating and free lists */
/* does g_slist_free deallocate all the links in the list, or just the first? */
/* I will assume it does the entire list */
void  ClearNameCodeList(GSList **List) 
{
  GSList *item;

  if(*List) {
    item = *List;
    while (item) {
      g_free( ((NameCodeType *)item->data)->name );
      g_free( ((NameCodeType *)item->data)->code );
      item = item->next;
    }
    g_slist_free(*List);
  }
  *List=NULL;
}

/* for given continent, find corresponding countries as identified in ZONE_TABLE
   and prepare list of country name using COUNTRY_TABLE
*/

GSList* FetchCountries(gchar *continent)
{
  FILE *fpc, *fpz;
  GString *title, *msg;
  gchar *button[]={"OK"};
  gchar line[500];
  gchar *codec, *codez, *name;
  GSList *Countries;
  
  if (strlen(continent)==0 )
    return NULL;

  if ( !(fpz=fopen(ZONE_TABLE,"r")) ) {  
    title = g_string_new(_("Read Zone Table"));
    msg = g_string_new(NULL);
    g_string_sprintf(msg,_(" Error reading zone table \"%s\": \n %s \nHow very sad.\n"),
		     ZONE_TABLE , g_strerror(errno) );
    showMessageDialog( msg->str, GTK_MESSAGE_ERROR );
    g_string_free(msg,TRUE);
    g_string_free(title,TRUE);
  }
  if ( !(fpc=fopen(COUNTRY_TABLE,"r")) ) {  
    title = g_string_new(_("Read Zone Table"));
    msg = g_string_new(NULL);
    g_string_sprintf(msg,_(" Error reading country table \"%s\": \n %s \nHow very sad.\n"),
		     COUNTRY_TABLE , g_strerror(errno) );
    showMessageDialog( msg->str, GTK_MESSAGE_ERROR );
    g_string_free(msg,TRUE);
    g_string_free(title,TRUE);
  }

  Countries=NULL;
  while(fgets(line,500,fpz)) {
    if (line[0] != '#') {
      
      /* check for continent in TZ value (third item on the line in ZONE_TABLE)
	 Also read country code at beginning of zone table entry.
	 Strictly this is only 2 characters, but I will allow for a whole string
	 (in my opinion 3 character would be more meaningful.  The standard sux */
      sscanf(line,"%ms %*s %ms",&codez,&name);
      if(name && strstr(name,continent)) {
	if(!CodeInList(codez,Countries)) {
	  g_free(name);
	  rewind(fpc);
	  while(fgets(line,500,fpc)) {
	    if (line[0] != '#') {

	      /* first, identify country */
	      if(sscanf(line,"%ms",&codec)==1) {
		if (!strcmp(codez,codec)) {

		  /* then extract name as entire string to \0 after tab */
		  /* (first make sure \n in line is reset to \0) */
		  name = (gchar *) strchr(line,'\n');
		  *name = '\0';
		  name = (gchar *) strchr(line,'\t');
		  name++;

		  Countries = AddNameCodeEntry(codec,name,Countries);
		}
		g_free(codec);
	      }	
	    }
	  }  
	}
      }
      else
	g_free(name);
      g_free(codez);
    }
  }
  fclose(fpc);
  fclose(fpz);

  return Countries;
}



/* from given country code ("*country"), find list of regions in ZONE_TABLE */
/* input: country is the two-letter country code from ISO3166 */
GSList* FetchRegions(gchar *country)
{
  FILE *fp;
  GString *title, *msg;
  gchar *button[]={"OK"};
  gchar line[500];
  gchar *code, *TZvalue, *region, *ptr;
  GSList *Regions;

  if (strlen(country)==0 )
    return NULL;

  if ( !(fp=fopen(ZONE_TABLE,"r")) ) {  
    title = g_string_new(_("Read Zone Table"));
    msg = g_string_new(NULL);
    g_string_sprintf(msg,_(" Error reading zone table \"%s\": \n %s \nHow very sad.\n"),
		     ZONE_TABLE , g_strerror(errno) );
    showMessageDialog( msg->str, GTK_MESSAGE_ERROR );
    g_string_free(msg,TRUE);
    g_string_free(title,TRUE);
  }

  Regions=NULL;
  while(fgets(line,500,fp)) {
    if (line[0] != '#') {
      /* check for entries corresponding to country code value 
	 (first item on the line in ZONE_TABLE)
	 Get name of region from third item on the line. */
      /* alternatively we may want to get the description from the optional
	 fourth item, where available.  Worry about that some other time */
      sscanf(line,"%ms %*s %ms",&code,&TZvalue);
      if(!strcmp(code,country)) {
	/* region name is in format:  continent/region
	   Extract the region part from the continent */
	ptr = (gchar *) strchr(TZvalue,'/');
	if(ptr)
	  region = g_strdup((gchar*)(ptr+1));
	else
	  region = g_strdup(TZvalue);

	/* Some regions have an underscore '_' in place of space */
	/* convert these to a real space */
	while( (ptr=(gchar*)strchr(region,'_')) ) 
	  *ptr = ' ';

	if(!CodeInList(TZvalue,Regions))
	  Regions = AddNameCodeEntry(TZvalue,region,Regions);
	g_free(region);
      }
      g_free(TZvalue);
      g_free(code);
    }
  }
  fclose(fp);

  return Regions;
}

void UpdateCountries(GtkTreeSelection *selection, gpointer ZoneNotes)
{
    GtkTreeIter iter ;
    GtkTreeModel *model;
    gchar *continent;

    GtkTreeModel *store;
    GtkTreeModel *filteredStore;
    GtkTreePath *path;
    GtkWidget *tree;
    GtkWidget *scrolled_window;
    GtkTreeSelection *countrySelection;
    
    AddZoneStruct *zoneData;

    if (gtk_tree_selection_get_selected (selection, &model, &iter))
    {
	gtk_tree_model_get (model, &iter, CONTINENT_NAME, &continent, -1);

	filteredStore = gtk_tree_view_get_model( gtk_tree_selection_get_tree_view(selection) );
	store = gtk_tree_model_filter_get_model( GTK_TREE_MODEL_FILTER(filteredStore) );
	path = gtk_tree_model_get_path( GTK_TREE_MODEL(filteredStore), &iter );	    
	path = gtk_tree_model_filter_convert_path_to_child_path( GTK_TREE_MODEL_FILTER(filteredStore), path );


	filteredStore = gtk_tree_model_filter_new( GTK_TREE_MODEL(store), path );
	if ( path ) gtk_tree_path_free( path );
	
	gtk_tree_model_filter_set_visible_func(GTK_TREE_MODEL_FILTER(filteredStore),
					       FilterCountries,
					       NULL, NULL );
	
	GtkWidget *label = gtk_label_new( _("Countries") );
	tree = gtk_tree_view_new_with_model (GTK_TREE_MODEL(filteredStore));
	gtk_tree_view_set_headers_visible( GTK_TREE_VIEW(tree), FALSE );
	gtk_widget_show(tree);

	GtkCellRenderer *renderer = gtk_cell_renderer_text_new ();
	GtkTreeViewColumn *column = gtk_tree_view_column_new_with_attributes ("Name",
							   renderer,
							   "text", COUNTRY_NAME,
							   NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW (tree), column);

	scrolled_window = gtk_scrolled_window_new (NULL, NULL);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window),
                                  GTK_POLICY_NEVER, GTK_POLICY_ALWAYS);
	gtk_widget_show (scrolled_window);
	gtk_container_add(GTK_CONTAINER(scrolled_window), tree);

	gtk_notebook_remove_page( (GtkNotebook*)ZoneNotes, COUNTRY_NAME );
	gtk_notebook_insert_page ( (GtkNotebook*)ZoneNotes, scrolled_window, 
				  label, COUNTRY_NAME );

	countrySelection = gtk_tree_view_get_selection( GTK_TREE_VIEW(tree) );
	gtk_tree_selection_set_mode (countrySelection, GTK_SELECTION_SINGLE);
	g_signal_connect (G_OBJECT (countrySelection), "changed",
			  G_CALLBACK (UpdateRegions),
			  ZoneNotes);

	zoneData = (AddZoneStruct*) g_object_get_data(G_OBJECT(ZoneNotes),ZONE_DATA);
	zoneData->countryTreeView = GTK_TREE_VIEW(tree);

	/* update region list straight away to match first country on
	  this continent  */
	gtk_tree_model_get_iter_first(GTK_TREE_MODEL(filteredStore),
				      &iter);
	gtk_tree_selection_select_iter(countrySelection, &iter);

	g_free (continent);
    }
}

void UpdateRegions(GtkTreeSelection *selection, gpointer ZoneNotes)
{
    GtkTreeIter iter ;
    GtkTreeModel *model;
    gchar *country;

    GtkTreeModel *store;
    GtkTreeModel *filteredStore;
    GtkTreePath *path;
    GtkWidget *tree;
    GtkTreeSelection *regionSelection;
    GtkWidget *scrolled_window;

    AddZoneStruct *zoneData;

    if (gtk_tree_selection_get_selected (selection, &model, &iter))
    {
	gtk_tree_model_get (model, &iter, COUNTRY_NAME, &country, -1);

	filteredStore = gtk_tree_view_get_model( gtk_tree_selection_get_tree_view(selection) );
	store = gtk_tree_model_filter_get_model( GTK_TREE_MODEL_FILTER(filteredStore) );
	path = gtk_tree_model_get_path( GTK_TREE_MODEL(filteredStore), &iter );	    
	path = gtk_tree_model_filter_convert_path_to_child_path( GTK_TREE_MODEL_FILTER(filteredStore), path );


	filteredStore = gtk_tree_model_filter_new( GTK_TREE_MODEL(store), path );
	if ( path ) gtk_tree_path_free( path );
	
	gtk_tree_model_filter_set_visible_func(GTK_TREE_MODEL_FILTER(filteredStore),
					       FilterRegions,
					       NULL, NULL );
	
	GtkWidget *label = gtk_label_new( _("Regions") );
	tree = gtk_tree_view_new_with_model (GTK_TREE_MODEL(filteredStore));
	gtk_tree_view_set_headers_visible( GTK_TREE_VIEW(tree), FALSE );
	gtk_widget_show(tree);

	GtkCellRenderer *renderer = gtk_cell_renderer_text_new ();
	GtkTreeViewColumn *column = gtk_tree_view_column_new_with_attributes ("Name",
							   renderer,
							   "text", REGION_NAME,
							   NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW (tree), column);

	scrolled_window = gtk_scrolled_window_new (NULL, NULL);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window),
                                  GTK_POLICY_NEVER, GTK_POLICY_ALWAYS);
	gtk_widget_show (scrolled_window);
	gtk_container_add(GTK_CONTAINER(scrolled_window), tree);

	gtk_notebook_remove_page( (GtkNotebook*)ZoneNotes, REGION_NAME );
	gtk_notebook_insert_page( (GtkNotebook*)ZoneNotes, scrolled_window, 
				  label, REGION_NAME );

	regionSelection = gtk_tree_view_get_selection(GTK_TREE_VIEW (tree));
	gtk_tree_selection_set_mode (regionSelection, GTK_SELECTION_SINGLE);
	g_signal_connect( G_OBJECT(regionSelection), "changed",
			  G_CALLBACK(SelectRegion),
			  g_object_get_data(G_OBJECT(ZoneNotes),ZONE_DATA));

	g_signal_connect(G_OBJECT(tree),
			 "button_press_event",
			 G_CALLBACK(ButtonPressedInRegionList),
			 (gpointer)g_object_get_data(G_OBJECT(ZoneNotes),ZONE_DATA));

	zoneData = (AddZoneStruct*) g_object_get_data(G_OBJECT(ZoneNotes),ZONE_DATA);
	zoneData->regionTreeView = GTK_TREE_VIEW(tree);

	g_free (country);
    }
}

void SelectRegion(GtkTreeSelection *selection, gpointer ZoneData)
{
    GtkTreeIter iter ;
    GtkTreeModel *model;
    gchar *description, *TZ;

    if (gtk_tree_selection_get_selected (selection, &model, &iter))
    {
	gtk_tree_model_get (model, &iter, REGION_NAME, &description, 
			    CODE, &TZ,
			    -1);
    
	gtk_entry_set_text( (GtkEntry *)((AddZoneStruct*)ZoneData)->DescriptionEntry, 
			    description);
	gtk_entry_set_text( (GtkEntry *)((AddZoneStruct*)ZoneData)->TZEntry, TZ);

	g_free( description );
	g_free( TZ );
    }
}

/* when left mouse button is double-clicked,
   send "key-pressed-event" to one of the Entry boxes 
   which will be handled by adding the given zone.
   We're assuming here that "select-row" preceded the double-click event */
gboolean ButtonPressedInRegionList (GtkWidget *regionTreeView, 
				     GdkEventButton *event,
				     gpointer ZoneData)
{
    GdkEventKey *KeyEvent;
    
    static GtkWidget *popup;
    
    gboolean returnVal = FALSE;
    
    if( (event->button==1) && ( event->type==GDK_2BUTTON_PRESS))
    {
       KeyEvent = (GdkEventKey*)g_malloc(sizeof(GdkEventKey));
       KeyEvent->keyval=GDK_KEY_Return;
       gint return_val;
       g_signal_emit_by_name( G_OBJECT( ((AddZoneStruct*)ZoneData)->TZEntry ),
				"key-press-event", (GdkEvent*)KeyEvent, &return_val);
       g_free(KeyEvent);
       returnVal = TRUE;
    }

    return returnVal;
}


/* Initialises (preexisting) iter to the node for the given column
   (whose value is identified by name) and returns TRUE.  Iter is
   invalid and FALSE returned, if there is no such name in the column.
*/ 
gboolean findRow( GtkTreeModel *model, const gint column, const gchar *nameToMatch, GtkTreeIter *iter )
{
    if( ! gtk_tree_model_get_iter_first( model, iter ) )
	return FALSE;

    gchar* name;
    gtk_tree_model_get( model, iter, column, &name, -1 );
    if ( name && !strcmp( nameToMatch, name ) )
    {
	g_free( name );
	return TRUE;
    }
    g_free(name);

    while( gtk_tree_model_iter_next( model, iter) )
    {
	gtk_tree_model_get( model, iter, column, &name, -1 );
	if ( name && !strcmp( nameToMatch, name ) )
	{
	    g_free(name);
	    return TRUE;
	}
    }
    g_free(name);

    return FALSE;
}


/* Implements GtkTreeModelFilterVisibleFunc, making continent entries
   only visible.
*/
gboolean FilterContinents(GtkTreeModel *model,
			  GtkTreeIter *iter,
			  gpointer data)
{
    gboolean visible;

    gchar *continentName;
    gchar *countryName;
    gchar *regionName;

    gtk_tree_model_get( model, iter, 
			CONTINENT_NAME, &continentName, 
			COUNTRY_NAME, &countryName, 
			REGION_NAME, &regionName, 
			-1 );

    visible = ( continentName && strlen(continentName)>0 ) && 
	( (!countryName) || strlen(countryName)==0 ) &&
	( (!regionName) || strlen(regionName)==0 ) ;

    g_free( continentName );
    g_free( countryName );
    g_free( regionName );

    return visible;
}

gboolean FilterCountries(GtkTreeModel *model,
			 GtkTreeIter *iter,
			 gpointer data)
{
    gboolean visible;
    gchar *countryName;
    gtk_tree_model_get( model, iter, 
			COUNTRY_NAME, &countryName, 
			-1 );

    visible = ( countryName && strlen(countryName)>0 );

    g_free( countryName );

    return visible;
}

gboolean FilterRegions(GtkTreeModel *model,
		       GtkTreeIter *iter,
		       gpointer data)
{
    gboolean visible;
    gchar *regionName;
    gtk_tree_model_get( model, iter, 
			REGION_NAME, &regionName, 
			-1 );

    visible = ( regionName && strlen(regionName)>0 );
    g_free( regionName );

    return visible;
}


/* zones are selected according to the method used in tzselect:
   First the continent is chosen, then, if necessary, the country is chosen,
   with countries being identified from the two-letter code in the
   entries of  [/usr/share/zoneinfo/]zone.tab (and country names taken from 
   iso3166.tab)  Then the region (or city) of that country is identified, from 
   zone.tab.
*/

void  PrepareZoneNotes(GtkWidget **ZoneNotes, AddZoneStruct *Zone)
{
    gint i;
    GtkTreeIter continentIter, countryIter, regionIter, iter;
    GtkTreePath *path;
    GtkTreeSelection *selection;
    GSList *countryList, *countryItem;
    GSList *regionList, *regionItem;
    gchar *name, *code;
    gchar *continentName, *ptr;
    gchar *ContinentLabel=_("Continents");
    gchar *CountryLabel=_("Countries");
    gchar *RegionLabel=_("Regions");
    gint columnId;

    /* collate date into a three-level tree.
       The first level represents continents,
       the second is regions/countries,
       the third is cities.
    */
   GtkTreeStore *store = gtk_tree_store_new (N_COLUMNS, 
					     G_TYPE_STRING,
					     G_TYPE_STRING,
					     G_TYPE_STRING,
					     G_TYPE_STRING ); 

    for( i=0; i<Ncontinents; i++) 
    {
	gtk_tree_store_append (store, &continentIter, NULL);
	gtk_tree_store_set (store, &continentIter,
			    CONTINENT_NAME, continents[i],
			    -1);

	/* add countries within that continent */
	continentName = g_strdup(continents[i]);
	/* change "Americas" to "America" by wiping out the 's' */
	if(!strcmp(continentName,"Americas"))
	    continentName[7]='\0';

	/* similarly remove " Ocean" in "Pacific Ocean", etc
	   by making the ' ' a null character */
	ptr = (gchar *) strchr(continentName,' ');
	if(ptr)
	    *ptr='\0';

	countryList=FetchCountries(continentName);
	g_free(continentName);
	countryItem = countryList;
	while( countryItem ) 
	{
	    name = ((NameCodeType *)countryItem->data)->name;
	    code = ((NameCodeType *)countryItem->data)->code;
	    gtk_tree_store_append (store, &countryIter, &continentIter);
	    gtk_tree_store_set (store, &countryIter,
				COUNTRY_NAME, name,
				CODE, code,
				-1);


	    /* add regions and cities within that country */
	    regionList=FetchRegions(code);
	    regionItem = regionList;
	    while( regionItem ) 
	    {
		gtk_tree_store_append (store, &regionIter, &countryIter);
		name = ((NameCodeType *)regionItem->data)->name;
		code = ((NameCodeType *)regionItem->data)->code;

		gtk_tree_store_set (store, &regionIter,
				    REGION_NAME, name,
				    CODE, code,
				    -1);
		regionItem = regionItem->next;
	    }
	    ClearNameCodeList(&regionList);

	    countryItem = countryItem->next;
	}
	ClearNameCodeList(&countryList);
    }


    /* form the view of the tree */

    // continents
    GtkTreeModel* filteredStore = gtk_tree_model_filter_new( GTK_TREE_MODEL(store), NULL );
    
    gtk_tree_model_filter_set_visible_func(GTK_TREE_MODEL_FILTER(filteredStore),
					   FilterContinents,
					   NULL, NULL );


    *ZoneNotes = gtk_notebook_new ();
    gtk_widget_show(*ZoneNotes);
    g_object_set_data(G_OBJECT(*ZoneNotes), ZONE_DATA, Zone );

    GtkWidget *label;
    label = gtk_label_new (ContinentLabel);
    GtkWidget *tree;

    tree = gtk_tree_view_new_with_model (GTK_TREE_MODEL(filteredStore));
    gtk_tree_view_set_headers_visible( GTK_TREE_VIEW(tree), FALSE );
    gtk_widget_show(tree);

    GtkCellRenderer *renderer;
    GtkTreeViewColumn *column;

    renderer = gtk_cell_renderer_text_new ();
    column = gtk_tree_view_column_new_with_attributes ("Name",
						       renderer,
						       "text", CONTINENT_NAME,
						       NULL);
    gtk_tree_view_append_column (GTK_TREE_VIEW (tree), column);
    gtk_notebook_insert_page(GTK_NOTEBOOK(*ZoneNotes), tree, 
			     label, CONTINENT_NAME );

    selection = gtk_tree_view_get_selection(GTK_TREE_VIEW (tree));
    gtk_tree_selection_set_mode (selection, GTK_SELECTION_SINGLE);
    g_signal_connect (G_OBJECT (selection), "changed",
		      G_CALLBACK (UpdateCountries),
		      (void*)*ZoneNotes);


    // countries, in default continent
    gboolean gotIt = findRow( GTK_TREE_MODEL(filteredStore),
			      CONTINENT_NAME, defaultContinent, &continentIter );
    path = NULL;
    if ( gotIt )
	gtk_tree_selection_select_iter( selection, &continentIter);

    filteredStore = gtk_tree_view_get_model( Zone->countryTreeView );
    selection = gtk_tree_view_get_selection( Zone->countryTreeView );
    gtk_tree_selection_set_mode (selection, GTK_SELECTION_SINGLE);
    g_signal_connect (G_OBJECT (selection), "changed",
		      G_CALLBACK (UpdateRegions),
		      (void*)*ZoneNotes);



    // regions, in default country
    gotIt = findRow( GTK_TREE_MODEL(filteredStore), COUNTRY_NAME, defaultCountry, &countryIter );
    if ( gotIt )
	gtk_tree_selection_select_iter( selection, &countryIter);

    filteredStore = gtk_tree_view_get_model( Zone->regionTreeView );
    selection = gtk_tree_view_get_selection( Zone->regionTreeView );
    gtk_tree_selection_set_mode (selection, GTK_SELECTION_SINGLE);
    g_signal_connect (G_OBJECT(selection), "changed",
		      G_CALLBACK (SelectRegion),
		      g_object_get_data(G_OBJECT(*ZoneNotes),ZONE_DATA));

    g_signal_connect(G_OBJECT(Zone->regionTreeView),
		     "button_press_event",
		     G_CALLBACK(ButtonPressedInRegionList),
		     (gpointer)Zone);

    /* default region/city  */
    gotIt = findRow( GTK_TREE_MODEL(filteredStore), REGION_NAME, defaultRegion, &regionIter );
    if ( gotIt )
    {
	gtk_tree_selection_select_iter( selection, &regionIter);
    }


}


void AddZoneToList(GtkWidget *w, gpointer NewZone)
{
  gchar *rowdata[2];
  gint row;
  extern gint changed;
  GtkTreeView *clocklist;
  GtkListStore *clocklistModel;
  GString *description;
  GString *TZ;
  GtkTreeIter iter;

  description = g_string_new(
     gtk_entry_get_text((GtkEntry *)((AddZoneStruct *)NewZone)->DescriptionEntry));
  TZ = g_string_new(
     gtk_entry_get_text((GtkEntry *)((AddZoneStruct *)NewZone)->TZEntry));

  /* check a time zone was given, if not, set to GMT */
  /* GST-0 is the "formal" TZ value for GMT */
  if ( TZ->len == 0 )
     g_string_assign(TZ,"GST-0");
  
  clocklist =  (GtkTreeView *)((AddZoneStruct *)NewZone)->clocklist;
  clocklistModel = GTK_LIST_STORE(gtk_tree_view_get_model( clocklist ));
  gtk_list_store_append ( clocklistModel, &iter);
  gtk_list_store_set ( clocklistModel, &iter,
		       TZ_NAME, TZ->str,
		       TZ_DESCRIPTION, description->str,
		       -1);     

  /* make new zone visible */
  gtk_tree_view_scroll_to_cell (clocklist,    
				gtk_tree_model_get_path(GTK_TREE_MODEL(clocklistModel), &iter),
				NULL, 
				/* attn: non use_align not implemented yet in GTK+
				   FALSE, 0, 0); */
				TRUE, 0, 1);
  gtk_tree_selection_select_iter( 
     gtk_tree_view_get_selection( clocklist ),
     &iter );

  SetTime((gpointer)clocklist);
  changed=1;

  g_string_free(description,TRUE);
  g_string_free(TZ,TRUE);
}

void AddZone( GtkWidget *w, gpointer clocklist )
{
  GtkWidget *window; 
  GtkWidget *vbox, *hbox;
  GtkWidget *AddButton, *DoneButton;
  GtkWidget *ZoneNotes;
  GtkWidget *Frame;
  GtkWidget *EntryDescription, *EntryTZ;
  static AddZoneStruct NewZone;

  NewZone.clocklist = (GObject *) clocklist;

  EntryDescription = gtk_entry_new();
  NewZone.DescriptionEntry = (GtkWidget *)EntryDescription;
  EntryTZ = gtk_entry_new();
  NewZone.TZEntry = (GtkWidget *) EntryTZ;

  window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title ((GtkWindow *)window, _("Add Time Zone"));
  gtk_window_set_position((GtkWindow *)window,GTK_WIN_POS_CENTER);
  gtk_container_set_border_width (GTK_CONTAINER (window), 5);
  
  vbox = gtk_vbox_new (FALSE, 0);
  gtk_container_add (GTK_CONTAINER (window), vbox);
  gtk_widget_show (vbox);


  /* display zone choices as notebook:
     Continents on one page, cities/countries on other
  */
  PrepareZoneNotes(&ZoneNotes,&NewZone);
  gtk_box_pack_start (GTK_BOX (vbox), ZoneNotes, TRUE, FALSE, 5);


  /* place zone in text entry box (allowing for manual entry if desired) */
  hbox = gtk_hbox_new (FALSE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, TRUE, FALSE, 5);
  gtk_widget_show (hbox);


  /* box for text description of zone */
  Frame = gtk_frame_new (_("Description"));
  gtk_frame_set_shadow_type( (GtkFrame *)Frame, GTK_SHADOW_NONE); 
  gtk_box_pack_start (GTK_BOX (hbox), Frame, TRUE, FALSE, 5);
  gtk_widget_show (Frame);
  gtk_container_add (GTK_CONTAINER (Frame), EntryDescription);
  /* where did the entry boxes get their default size from?
     Not setting the size here (and below for TZ) makes the dialog box too large 
     Arguably too large, anyway - this is a question of taste of course */
    
  gtk_widget_set_size_request(GTK_WIDGET(EntryDescription), 125, 0);  
  //gtk_widget_set_usize( GTK_WIDGET(EntryDescription),125,0);
  
  gtk_widget_show (EntryDescription);


  /* box for TZ value of zone */
  Frame = gtk_frame_new (_("TZ value"));
  gtk_frame_set_shadow_type( (GtkFrame *)Frame, GTK_SHADOW_NONE); 
  gtk_box_pack_start (GTK_BOX (hbox), Frame, TRUE, FALSE, 5);
  gtk_widget_show (Frame);
  gtk_container_add (GTK_CONTAINER (Frame), EntryTZ);
  
  gtk_widget_set_size_request(GTK_WIDGET(EntryTZ), 125, 0);  
  //gtk_widget_set_usize( GTK_WIDGET(EntryTZ),125,0);
  
  gtk_widget_show (EntryTZ);

  /* buttons to accept zone, or exit */
  hbox = gtk_hbox_new (FALSE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, TRUE, FALSE, 5);
  gtk_widget_show (hbox);

  AddButton = gtk_button_new_with_label (_("Add Zone"));
  g_signal_connect (G_OBJECT (AddButton), "clicked",
		      G_CALLBACK (AddZoneToList), (gpointer)&NewZone );
  gtk_box_pack_start (GTK_BOX (hbox), AddButton, TRUE, FALSE, 0);
  g_signal_connect (G_OBJECT (EntryDescription), "key-press-event",
		      G_CALLBACK (GotOK), (gpointer)AddButton);
  g_signal_connect (G_OBJECT (EntryTZ), "key-press-event",
		      G_CALLBACK (GotOK), (gpointer)AddButton);
  gtk_widget_show (AddButton);

  DoneButton = gtk_button_new_with_label (_("Done"));
  g_signal_connect (G_OBJECT (DoneButton), "clicked",
		      G_CALLBACK (DestroyWindow), (gpointer)window);
  gtk_box_pack_start (GTK_BOX (hbox), DoneButton, TRUE, FALSE, 0);
  gtk_widget_show (DoneButton);

  gtk_widget_show(window); 
}

void WriteZoneDescription(GtkDialog *dialog, gint responseId, gpointer Zone)
{
  extern gint changed;
  GString *description;
  GtkTreeView *clocklist;
  GtkTreeModel *clocklistModel;
  GtkTreeIter iter;

  if ( responseId == OK_BUTTON )
  {
     clocklist = GTK_TREE_VIEW(((AddZoneStruct *)Zone)->clocklist);
     if ( gtk_tree_selection_get_selected( 
	     gtk_tree_view_get_selection(clocklist),
	     &clocklistModel,
	     &iter) )
     {
	
	description = g_string_new(
	   gtk_entry_get_text((GtkEntry *)((AddZoneStruct *)Zone)->DescriptionEntry));
	
	gtk_list_store_set ( GTK_LIST_STORE(clocklistModel), 
			     &iter,
			     TZ_DESCRIPTION, description->str,
			     -1);     
	changed=1;

/*	gtk_tree_model_row_changed( clocklistModel,
				   gtk_tree_model_get_path( clocklistModel, &iter),
				    &iter);*/
//	resizeWindow( gtk_widget_get_toplevel( GTK_WIDGET(clocklist) ), 
	//clocklist );
     }
  }

  if ( responseId > 0 )
  {
     gtk_widget_destroy(GTK_WIDGET(dialog));
  }
}
  
void ChangeZoneDescription(GtkWidget *w, gpointer clocklist)
{
  gchar *description;
  GtkWidget *window, *vbox, *hbox;
  GtkWidget *DescriptionEntry, *OKButton, *CancelButton;
  static AddZoneStruct Zone;
  GString *title, *msg;
  gchar *button[]={"OK"};
  GtkTreeModel *clocklistModel;
  GtkTreeIter selectedRowIter;

  if ( ! gtk_tree_selection_get_selected( 
	  gtk_tree_view_get_selection( clocklist ),
	  &clocklistModel,
	  &selectedRowIter ) )
  {
    /* is this dialog box useful? */
     showMessageDialog( _("No zone chosen for changing."), GTK_MESSAGE_WARNING );
     return;
  }
  
  gtk_tree_model_get( GTK_TREE_MODEL(clocklistModel), &selectedRowIter,
		      TZ_DESCRIPTION, &description,
		      -1);

  Zone.clocklist = (GObject *) clocklist;

  window = gtk_dialog_new_with_buttons ( _("Change Zone Description"),
					 NULL,
					 GTK_UI_MANAGER_SEPARATOR,
					 "OK", OK_BUTTON,
					 "Cancel", CANCEL_BUTTON,
					 NULL);

  gtk_window_set_position((GtkWindow *)window,GTK_WIN_POS_CENTER);
  gtk_container_set_border_width (GTK_CONTAINER (window), 5);

  DescriptionEntry = gtk_entry_new();
  Zone.DescriptionEntry = DescriptionEntry;
  gtk_entry_set_text( (GtkEntry *) DescriptionEntry, description);
  g_signal_connect (G_OBJECT (DescriptionEntry), "key-press-event",
		      G_CALLBACK (GotOKInDialog), (gpointer)window);
  
  
  GtkWidget *content_area = gtk_dialog_get_content_area(GTK_DIALOG(window));
  gtk_container_add(GTK_CONTAINER(content_area), DescriptionEntry);
  
  //gtk_container_add (GTK_CONTAINER (GTK_DIALOG(window)->vbox), DescriptionEntry);
  gtk_widget_show (DescriptionEntry);

  g_signal_connect (G_OBJECT (window), "response",
			    G_CALLBACK(WriteZoneDescription), 
			    (gpointer)&Zone );

  gtk_widget_show(window);
}

