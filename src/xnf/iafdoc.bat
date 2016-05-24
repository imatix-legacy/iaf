@echo off
gsl iafdoc
perl \imatix\bin\mkgdl iafdoc.txt
gsl -page:4 -quiet -tpl:html_frameset -gdl:iafdoc gurudoc
del pfl.txt ofl.txt dfl.txt iafdoc.gdl
copy/y iafdoc*.htm ..\..\doc
