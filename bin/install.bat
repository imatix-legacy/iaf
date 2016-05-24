@echo off
zip -q iafgsl *.gsl

:-  Transfer zip file to website using FTP
echo iaftools>response
echo GhettoBlaster>>response
echo bin>>response
echo put iafgsl.zip>>response
echo quit>>response
ftp -s:response imatix.net
del response
del iafgsl.zip

