@echo off
:-  Build iAF Email application
:-

if %1.==.        goto error
    gslgen -q -filename:%1   eslservice.gsl
    goto end

:error
    echo Missing Email Service Layout (ESL) File name
:end
