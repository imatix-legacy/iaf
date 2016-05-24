Building The iAF Standard Component Library

Prerequisites

    To build the SCL dynamic libraries, you need:
    
    1. iMatix GSLgen version 2.0
    2. Microsoft Visual C++ 6.0


Building The DLLs

    1.  gslgen scl

        This command generates the source code and a project workspace
        called scl.dsw.

    2.  Open this workspace with MSVC/C++ 6.0.

    3.  Build each scl_xxx project using the "Win32 Release MinDependencies"
        configuration.



