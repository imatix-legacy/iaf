<%@ Language=VBScript %>
<% 
cur_message = "Your previous Session has timed out... Please login again..."
session.abandon
Response.Redirect ("login.asp?op=I")
%>
