@echo off
:-  Unregister SCL components
echo Unregistering SCL components, please wait...
if %OS%.==Windows_NT. goto winnt

for %%1 in (scl*.dll) do %windir%\system\regsvr32 /u /s %%1
goto done
:winnt

for %%1 in (scl*.dll) do %windir%\system32\regsvr32 /u /s %%1
:done
echo Done
