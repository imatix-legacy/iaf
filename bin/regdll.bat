@echo off
rem ******************************************
rem * regdll.bat Register All Component  DLL *
rem *                                        *
rem ******************************************
echo Registering all components, wait please...
if %OS%.==Windows_NT. goto winnt
for %%1 in (*.dll) do regsvr32 /s %%1
goto end

:winnt
for %%1 in (*.dll) do regsvr32 /s %%1
echo Done

:end
