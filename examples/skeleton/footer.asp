<%
'   footer.asp - Standard footer file for iAF applications
'
%>
<!--#include file="sub_alert.asp" -->
</td></tr></table>
</td></tr>
  <% if cur_message <> "" then %>
  <tr><td align=center colspan=99><font size=+1 color="red"><em><%=cur_message%></em></font></td></tr>
  <% end if %>
  <tr>
    <td height=1 colspan=99 background="images\footerback.gif">
    <table border=0 cellpadding=0 border=0 cellspacing=0 width=100%>
    <tr><td>
    <font size=-2 color=<%=Session ("FOOTERFONTCOLOR")%>>
    <a class=tailbar href="http://www.imatix.com">Copyright &#169 2000-2001 iMatix Corporation</a>
    <% if cur_userid > 0 then %>
    <br><b><%=cur_usersurname%>, <%=cur_userforename%> logged in as: <%=Session ("usertype_name")%> (<%=Session ("cur_rolename")%>)
    <% else %>
    <br><b><%=APPLICATION_ID%>
    <% end if %>
    </td>
    <td><img src="images\footerback.gif"></td>
    <% if cur_userid > 0 then %>
    <td><font color=<%=Session ("FOOTERFONTCOLOR")%>><b><% = Session ("statusbar_text") %></b></td>
    <% end if %>
    <td align=right><font size=-2 color=<%=Session ("FOOTERFONTCOLOR")%>>
    <%
    if Session ("debug") = 1 then
        if Session ("cur_debug") = 1 then
            create_link "DEBUG", "0", 0, "", "Debug", "Disable OAL debugging", "tailbar"
        else
            create_link "DEBUG", "1", 0, "", "Debug", "Enable OAL debugging", "tailbar"
        end if
    end if
    if Session ("loggedin") > 0 then
        if cur_program <> "welcome" then
            create_link "^HRETURN", session ("homepage"), 0, "", "Home", "Back to Home Page", "tailbar"
        end if
        if Session ("showhints") = 1 then
            create_link "HINTS", "0", 0, "", "&lt;&gt;", "Full screen", "tailbar"
        else        
            create_link "HINTS", "1", 0, "", "&gt;&lt;", "Restore screen", "tailbar"
        end if    
        create_link "^LRETURN", "default", 0, "", "Logoff", "Logoff", "tailbar"
    end if %>
    <br><b><% = FormatDateTime(Now, vbLongDate) & " " & FormatDateTime(Time, vbLongTime) %>
    &nbsp;</td></tr>
    </table>
</td></tr>
</table>
</body>
</html>
