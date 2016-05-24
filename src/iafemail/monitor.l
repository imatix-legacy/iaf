!===========================================================================
!  monitor.l
!
!  Copyright (c) 2004 iMatix Corporation all rights reserved.
!  This source code is proprietary and may not be used or distributed
!  without explicit permission.
!===========================================================================

-idle
-nosort
-schema:lrservice.pl

After-Init:
    (--) Ok                                 -> Checking-Process
          + Check-If-Process-Running
    (--) Error                              ->
          + Terminate-The-Program

Checking-Process:
    (--) Ok                                 -> Checking-Logfile
          + Reload-Configuration-File
          + Check-Logfile-Progress
    (--) Missing                            -> Checking-Process
          + Sleep-For-A-Wink
          + Check-If-Process-Running

Checking-Logfile:
    (--) Ignore                             -> Checking-Process
          + Sleep-For-A-Wink
          + Check-If-Process-Running
    (--) Growing                            -> Checking-Process
          + Sleep-For-A-Wink
          + Check-If-Process-Running
    (--) Unchanged                          -> Checking-Process
          + Kill-Process
          + Sleep-For-A-Wink
          + Check-If-Process-Running

