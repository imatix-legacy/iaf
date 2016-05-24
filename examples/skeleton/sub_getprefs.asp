<%
'-------------------------------------------------------------------------
'   sub_getprefs.asp - Load customer preferences into Session
'
%>
<!--#include file="prefdef.asp"   -->
<!--#include file="prefval.asp"   -->
<%

sub get_preferences
    set pValues = Server.CreateObject ("scl.XMLstore")
    pxml.value = oa_do_prefdef ("<oal do=""query"" query=""summary"" limit = ""100""/>")
    if pxml.attr ("done") = "ok" then
        pxml.item_first_child
        do while pxml.valid
            pref_name  = pxml.item_child_value ("name")
            pref_value = pxml.item_child_value ("default")
            pValues.value = oa_do_prefval ("<oal do=""query"" query=""byname"" control=""eq"">" _
                    & "<prefdefname>" & pref_name & "</prefdefname></prefval></oal>")
            if pValues.attr ("done") = "ok" and Cint (pValues.attr ("count", "0")) > 0 then
                pValues.item_first_child
                pref_value = pValues.item_child_value ("value", "")
            end if
            Session (pref_name) = pref_value
            pxml.item_next
        loop
    end if
    set pValues = Nothing
end sub
%>
