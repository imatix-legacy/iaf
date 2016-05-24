@echo off
gsl12 -schema:dbioodbc.sch emaillog.dbm
gsl12 -schema:dbioodbc.sch emailqueue.dbm
lr sflxmll
for %%1 in (*.c)   do call c %%1
for %%1 in (*.obj) do call c -r libapp.lib %%1
call c -l usemail
copy usemail.exe ..

