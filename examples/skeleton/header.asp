<%
'   header.asp - Sample header file
%>
<html>
<head><title>Skeleton Application</title>
<meta http-equiv="Content-Type" content="text/html; charset=iso-8859-1">
<meta http-equiv="Pragma" content="no-cache">
<script language="JavaScript" type="text/javascript">
<!--
//  Sets the form action and does a form submission; this can be used to attach
//  actions to hyperlinks, images, and buttons inside or outside forms.
//
function formaction (action_code, check, argument_val)
{
    var objDate = new Date ();
    document.forms[0].accode.value = action_code;
    document.forms[0].argval.value = argument_val;
    if ((check && check_form (document.forms[0]))
    ||  !check) {
        document.forms[0].submit();
    }
}
function confaction (action_code, check, argument_val, message)
{
    if (confirm (message))
        formaction (action_code, check, argument_val);
}
function hilite (imgDocID,imgObjName)
{
    document.images [imgDocID].src = eval(imgObjName + ".src");
}
function popdoc (url)
{
    window.open (url, "DocumentWindow", "menubar=1,scrollbars=1,resizable=1,width=700,height=500")
}
//-->
</script>
</head>
<body topmargin=0 leftmargin=0 rightmargin=0 bgcolor=<%=Session("BODYBGCOLOR")%>>
<style><!--
<%=Session("STYLESHEET")%>
--></style>
<table border=0 cellpadding=0 cellspacing=0 width=100% height=100%>
<tr height=1><td colspan=99>
  <table border=0 cellpadding=0 cellspacing=0 width=100%>
  <tr><td width=100%<% if Session("HEADERBACK") <> "" then response.write " background=" & Session("HEADERBACK")%>>
   <% if Session("HEADERLEFT") <> "" then %>
   <img src="<%=Session("HEADERLEFT")%>" align=left>
   <% end if%>
   <% if Session("HEADERRIGHT") <> "" then %>
   <img src="<%=Session("HEADERRIGHT")%>" align=right>
   <% end if%>
   </td></tr></table>
</td></tr>
<tr><td colspan=99 height=4>&nbsp;</td></tr>
<tr><td><table border=0 cellpadding=0 cellspacing=0 width=100% height=100%>
<tr>
<% if show_sidebars = 0 then %>
    <td width=3%>&nbsp;</td>
<% end if %>
<td valign=top>

