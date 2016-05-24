@echo off
rem ******************************************
rem * regdll.bat Register All Component  DLL *
rem *                                        *
rem ******************************************
echo Unregistering all components, wait please...
if %OS%.==Windows_NT. goto winnt
for %%1 in (*.dll) do c:\windows\system\regsvr32 /u /s %%1
goto end

:winnt
for %%1 in (*.dll) do c:\winnt\system32\regsvr32 /u /s %%1
echo Done

:end
