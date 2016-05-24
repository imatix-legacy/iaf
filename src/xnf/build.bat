@echo off
gsl -xnf:pfl -parse:1 -code:1 buildxnf
copy/y pflparse.gsl %iafbin%
copy/y pflcode.gsl  %iafbin%

