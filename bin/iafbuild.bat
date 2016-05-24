@echo off
:-  Build iAF application
:-
:-  set model=lite|fulldev|full
:-  set dabatase=odbc|mssql
:-
if %apname%.==. goto end

if %1.==.     goto full
if %1.==dfl.  goto dfl
if %1.==ofl.  goto ofl
if %1.==pfl.  goto pfl
if %1.==one.  goto pflone
if %1.==doc.  goto dfldoc
if %1.==prod. goto prod
if %1.==lite. goto lite
if %1.==dev.  goto dev
if %1.==inc.  goto inc
if %1.==chk.  goto pflchk

goto end

:-  Default behaviour - build full application
:full
    if exist rebuild.?fl del rebuild.?fl
    echo "Starting build" > ________.___
    for %%a in (*.wfl) do gslgen -q -unicode:%unicode% %%a
    gslgen -q global
    if errorlevel 1 goto failed
    gslgen -q -dfl:%apname% -database:%database%  -unicode:%unicode% builddfl
    if errorlevel 1 goto failed
    gslgen -q -dfl:%apname% -database:none -doc:1 -unicode:%unicode% builddfl
    if errorlevel 1 goto failed
    gslgen -q -ofl:%apname% -model:%model%        -unicode:%unicode% buildofl
    if errorlevel 1 goto failed
    gslgen -q -pfl:%apname%                       -unicode:%unicode% buildpfl
    if errorlevel 1 goto failed
    if %model%==lite goto end

    echo Recompiling VB components...
    if %vbpath%.==. set vbpath="c:\Program Files\Microsoft Visual Studio\VB98\vb6"

    :-  Shut down web server so we can recompile the components
    if %OS%.==Windows_NT. net stop "World Wide Web Publishing Service"
    if %OS%.==Windows_NT. net stop "FTP Publishing Service"
    if %OS%.==Windows_NT. net stop "IIS Admin Service"
    if not %OS%.==Windows_NT. kill /f inetinfo
    
    %vbpath% /m %apname%.vbg

    :-  Restart the web server
    if %OS%.==Windows_NT. net start "IIS Admin Service"
    if %OS%.==Windows_NT. net start "FTP Publishing Service"
    if %OS%.==Windows_NT. net start "World Wide Web Publishing Service"
    if not %OS%.==Windows_NT. c:\windows\system\inetsrv\pws.exe

    :echo Saving existing database...
    :- not correct if Unicode...
    :iafodbc -u sa -d %odbcname% -e -en >dbsave_log.xml
    :echo Recreating the database...
    :iafodbc -u sa -d %odbcname% -x %apname%crt.sql>dbsave_log.xml
    :echo Reloading the database...
    :iafodbc -u sa -d %odbcname% -i -en >dbload_log.xml
    :echo Loading stored procedures...
    :gsl -q -database:%odbcname% storeproc
    call storeproc fast
    goto end

:-  Incremental build
:inc
    if not exist rebuild.dfl goto inc_1
        gslgen -q -dfl:%apname% -database:%database% -unicode:%unicode% builddfl
        if errorlevel 1 goto failed
        echo Saving existing database...
        iafodbc -u sa -d %odbcname% -e request.xml -en 
        echo Recreating the database...
        iafodbc -u sa -d %odbcname% -x %apname%crt.sql>dbsave_log.xml
        echo Reloading the database...
        iafodbc -u sa -d %odbcname% -i request.xml -en 
    :inc_1

    if not exist rebuild.ofl goto inc_2
        gslgen -q -ofl:%apname% -model:%model% -unicode:%unicode% buildofl
        if errorlevel 1 goto failed
        if %model%==lite goto inc_2
        echo Loading stored procedures...
        gsl -q -database:%odbcname% storeproc
        call storeproc fast
        echo Recompiling VB components...
        if %vbpath%.==. set vbpath="c:\Program Files\Microsoft Visual Studio\VB98\vb6"

        :-  Shut down web server so we can recompile the components
        if %OS%.==Windows_NT. net stop "World Wide Web Publishing Service"
        if %OS%.==Windows_NT. net stop "FTP Publishing Service"
        if %OS%.==Windows_NT. net stop "IIS Admin Service"
        if not %OS%.==Windows_NT. kill /f inetinfo

        %vbpath% /m %apname%.vbg

        :-  Restart the web server
        if %OS%.==Windows_NT. net start "IIS Admin Service"
        if %OS%.==Windows_NT. net start "FTP Publishing Service"
        if %OS%.==Windows_NT. net start "World Wide Web Publishing Service"
        if not %OS%.==Windows_NT. c:\windows\system\inetsrv\pws.exe
    :inc_2

    if not exist rebuild.pfl goto inc_3
        gslgen -q -pfl:%apname% -unicode:%unicode% buildpfl
        if errorlevel 1 goto failed
    :inc_3
    del rebuild.?fl
    goto end

:pfl
    if exist rebuild.?fl del rebuild.?fl
    gslgen -q -pfl:%apname% -unicode:%unicode% buildpfl
    if errorlevel 1 goto failed
    goto end
    
:ofl
    if exist rebuild.?fl del rebuild.?fl
    gslgen -q -ofl:%apname% -model:%model% -unicode:%unicode% buildofl
    if errorlevel 1 goto failed
    gsl -q -database:%odbcname% storeproc
    goto end

:dfl
    if exist rebuild.?fl del rebuild.?fl
    gslgen -q -dfl:%apname% -database:%database% -unicode:%unicode% builddfl
    if errorlevel 1 goto failed
    goto end

:pflone
    gslgen -q -pfl:%apname% -match:%2 -unicode:%unicode% buildpfl
    if errorlevel 1 goto failed
    goto end
    
:pflchk
    gslgen -q -pfl:%apname% -verify:1 buildpfl
    goto end
    
:prod
    set model=full
    set database=mssql
    goto full

:dev
    set model=fulldev
    set database=mssql
    goto full

:lite
    set model=lite
    set database=odbc
    goto full

    
:dfldoc
    gslgen -q -dfl:%apname% -database:none -doc:1 -unicode:%unicode% builddfl
    if errorlevel 1 goto failed
    goto end

:failed
    echo Build process halted with errors

:end
    :-  Skin any SPF files
    for %%a in (*.spf) do perl %iafbin%\skinner %%a
    if exist *.spf del *.spf

    set vbpath=
    set model=
    set database=
