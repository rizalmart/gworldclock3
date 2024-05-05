/* global definitions needed by several gworldclock files */

#ifndef GWORLDCLOCK
#define GWORLDCLOCK

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


GString timeDisplayFormat;


#endif
