#include "sfl.h"

int
main (int argc, char *argv [])
{
    double
        timestamp;
    long
        date,
        time;

    if (argc < 2)
      {
        printf ("Error: missing iaf timestamp argument\n");
        printf ("ts2date display iAF timestamp value in readable date/time value\n");
        return (-1);
      }

    timestamp = atof (argv[1]);

    if (timestamp > 0)
      {
        printf ("Current timestamp is: %.06f\n", gmtimestamp_now ());
        date = timestamp_date (timestamp);
        time = timestamp_time (timestamp);

        printf ("GMT value    = %02d/%02d/%04d %02d:%02d:%02d\n",
                GET_DAY    (date),
                GET_MONTH  (date),
                GET_CCYEAR (date),
                GET_HOUR   (time),
                GET_MINUTE (time),
                GET_SECOND (time));

        gmt_to_local (date, time, &date, &time);

        printf ("Locale value =  %02d/%02d/%04d %02d:%02d:%02d\n",
                GET_DAY    (date),
                GET_MONTH  (date),
                GET_CCYEAR (date),
                GET_HOUR   (time),
                GET_MINUTE (time),
                GET_SECOND (time));

      }
    else
        printf ("Error: invalid timestamp value\n");
    return (0);
}