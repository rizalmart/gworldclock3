/* functions for automatically resizing the window */

#include <gtk/gtk.h>

#include "resize.h"

/* Resize window to appropriate width for name and time/date columns
   and default number of zones.
   The window must be already shown for this to work. 
 */
void resizeWindow( GtkWidget *window, GtkTreeView *clocklist )
{
   gint width=0, height=0, scrollbarWidth;
   GdkRectangle rect;
   gint cellWidth, cellHeight;
   GtkTreeViewColumn *column;
   GList *list;
   gint zonesToView=DEFAULT_ZONES_TO_VIEW; /* rows visible by default */

   /* segfaults here if the window is not already shown */
   gtk_widget_show(window);
   gtk_window_get_default_size( GTK_WINDOW(window), &scrollbarWidth, &height);
   if ( scrollbarWidth == -1 )
   {
      gtk_tree_view_get_visible_rect( clocklist, &rect );
      gtk_window_get_size( GTK_WINDOW(window), &width, &height);
      scrollbarWidth = width - rect.width;
      gtk_window_set_default_size( GTK_WINDOW(window), scrollbarWidth, height);
   }

   /* calculate width (of columns) */
   width = 0;
   list = gtk_tree_view_get_columns( clocklist );
   while ( list != NULL )
   {
      column = GTK_TREE_VIEW_COLUMN( list->data );
      gtk_tree_view_column_cell_get_size( column, NULL, NULL, NULL,
					  &cellWidth, &cellHeight );

      // gtk_tree_view_column_get_width gets a better spacing,
      // but can be 0 when gtk_tree_view_columns_autosize is used.
      if ( gtk_tree_view_column_get_width(column) == 0 )
	  width += cellWidth+2;  // needs +2 to get improve spacing
      else
	  width += gtk_tree_view_column_get_width(column);

      list = g_list_next(list);
   }
   g_list_free( list );
   /* add width of scroll bar, to get total width of window */
   width += scrollbarWidth;

   /* calculate height - initially shows one row,
      allow room for zonesToView rows.
      Adjustment of 1.5 rows is needed to avoid unsightly extra whitespace 
      at bottom of window.
   */
   if ( zonesToView > 
	gtk_tree_model_iter_n_children( gtk_tree_view_get_model(clocklist), NULL) )
   {
      zonesToView = gtk_tree_model_iter_n_children( gtk_tree_view_get_model(clocklist), NULL );
   }

   height += cellHeight * (zonesToView - 1.5) ;

   gtk_window_resize( GTK_WINDOW(window), width, height);
}

/* Act on "notify::width" signal from GtkTreeViewColumn,
   to dynamically adjust width of clock.
 */
void updateColumnWidth (GtkTreeViewColumn *column,  
			GType dummyType,
			gpointer clocklist)
{
   static int oldWidth=0;

   /* unfortunately the "notify::width" signal appears to be sent whenever
      the clock is changed, rather than whenever the description column is 
      changed.  So only update the window size when the width has really changed. */
   if ( oldWidth != gtk_tree_view_column_get_width(column) )
   {
      resizeWindow( gtk_widget_get_toplevel( GTK_WIDGET(clocklist) ), 
		    GTK_TREE_VIEW(clocklist) );
      oldWidth = gtk_tree_view_column_get_width(column);
   }

}
