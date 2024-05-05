/* functions related to setting up configuration options */

void GetOptions( int argc, char **argv );

/* save options to options file (default ~/.gworldclock) 
   - creates the file if it does not already exist
*/
gint SaveOptions();

/* dialog box allows user to set preferences
   e.g. preferred display format for time and date */
void ChangePreferences(GtkWidget *w, gpointer clocklist);
