/* functions for automatically resizing the window */

#ifndef GWORLDCLOCK_RESIZE
#define GWORLDCLOCK_RESIZE

/* default number of zones to view before scrollbar is needed */
#define DEFAULT_ZONES_TO_VIEW 6

/* Resize window to appropriate width for name and time/date columns
   and default number of zones.
   The window must be already shown for this to work. 
 */
void resizeWindow( GtkWidget *window, GtkTreeView *clocklist );

/* Act on "notify::width" signal from GtkTreeViewColumn,
   to dynamically adjust width of clock.
 */
void updateColumnWidth (GtkTreeViewColumn *column,  
			GType dummyType,
			gpointer clocklist);

#endif
