@echo off
:-  Register SCL components, not under MTS
echo Registering SCL components, please wait...
if %OS%.==Windows_NT. goto winnt

for %%1 in (scl*.dll) do %windir%\system\regsvr32 /s %%1
goto done
:winnt

for %%1 in (scl*.dll) do %windir%\system32\regsvr32 /s %%1
:done
echo Done
