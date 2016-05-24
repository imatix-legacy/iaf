<%@ Language=VBScript %>
<% 
'
'   Written:  1999/11/26  Pieter Hintjens <ph@imatix.com>
'   Modified: 2000/07/08  Pieter Hintjens <ph@imatix.com>
'
'   We invoke the login program with op=I to initialise it
'
Session.Abandon
Response.Redirect ("login.asp?op=I")
%>
