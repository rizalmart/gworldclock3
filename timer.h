/* functions related to running the timer, displaying the times in each zone */

#ifndef GWORLDCLOCK_TIMER
#define GWORLDCLOCK_TIMER

/* size of string buffer receiving the formatted time/date as a string in strftime */
#define TIME_DISPLAY_SIZE 50

void  start_clocks(gpointer clocklist);

gint SetTime(gpointer clocklist);

gboolean SetToGivenTime  (GtkTreeModel *clocklistModel,
			  GtkTreePath *path,
			  GtkTreeIter *iter,
			  gpointer timeToSet);

gint start_timer( gpointer clocklist );

void stop_timer();

void reset_timer( gpointer clocklist );

#endif
