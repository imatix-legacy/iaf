<%
'   library.asp  - a small set of general-purpose ASP functions
'
'   makedate          Formats day, month, year into yyyymmdd long value
'   maketime          Formats hour, minute, seconds into hhmmsscc value
'   date_dd           Return day portion of date from yyyymmdd value
'   date_mm           Return month portion of date from yyyymmdd value
'   date_yy           Return year portion of date from yyyymmdd value
'   fulldate          Ensures that a year contains 4 digits and is Y2K-ok
'   btoi              Converts boolean values from checkbox on/off to 0 or 1
'   atod              Converts ASCII date components yy mm dd to date value
'   atot              Converts ASCII date components hh mm ss cc to time value
'   tvtod             Converts VB time value to date value
'   tvtot             Converts VB time value to time value
'   dtotv             Converts date value to VB time value
'   cutoff            Compute future cutoff date
'   showdate          Format date value as string using cur_language
'   showdate_compact  Format compact date value as string using cur_language
'   showtime          Format time value as string
'   showts            Format timestamp value as string using cur_datefmt
'   showmoney         Format currency value as string using cur_moneyfmt
'   shownumber        Format long number using thousands separators
'   astr              Put string inside apostophes and doubles ' chars
'   monthtext         Return month name for cur_language code
'   daytext           Return day name for cur_language code
'   booltext          Return textual value for Boolean field
'   choptext          Return text truncated to a certain length
'   open_select       Start select box/radio list
'   build_select      Build select box/radio list
'   close_select      End select box/radio list
'   build_date        Build date select field
'   build_time        Build time select field
'   translate         Translate value through database lookup
'   define_icon       Define image icon with rollover
'   show_icon         Display icon with rollover
'   create_link       Create link with status and mouse-over
'   open_link         Start link with status and mouse-over
'   create_button     Create submit button with status and mouse-over
'   create_link_text  Create link with status and mouse-over as text
'   open_link_text    Start link with status and mouse-over as text
'   format_datetime   Format date and time using LCID
'   response_redirect Safe implementation of response.redirect
%>

<% 
'   Standard ASP functions

'   Formats year, month, day as a Y2K-compliant date value: yyyymmdd
'
Function makedate (yy, mm, dd)
    if mm = 0 or dd = 0 then
        makedate = 0
    else
        makedate = fulldate (yy) * 10000 + mm * 100 + dd
    end if
End Function

'   Formats hour, minute, seconds into hhmmsscc value
'
Function maketime (hh, mm, ss)
    if hh = 0 and mm = 0 then
        maketime = 0
    else
        maketime = (hh) * 1000000 + mm * 10000 + ss * 100
    end if
End Function

'   Return day portion of date formatted as long yyyymmdd
'
Function date_dd (value)
    date_dd = value mod 100
End Function

'   Return month portion of date formatted as long yyyymmdd
'
Function date_mm (value)
    date_mm = (value / 100) mod 100
End Function

'   Return year portion of date formatted as long yyyymmdd
'
Function date_yy (value)
    date_yy = int (value / 10000)
End Function

'   Ensures that date contains full 4 digits
'
Function fulldate (yy)
    '   Ensure 4 digit year, using a sliding 50-year window
    if yy < 100 then
        cur_year = Year (Now ()) mod 100
        cur_cent = Year (Now ()) - cur_year
        if cur_year < 50 then
            cur_year = cur_year + 100
            cur_cent = cur_cent - 100
        end if
        if yy > cur_year - 50 then
            yy = yy + cur_cent
        else
            yy = yy + cur_cent + 100
        end if 
    end if
    fulldate = yy
End Function

'   Converts a Boolean values from a checkbox string to a binary value
'
Function btoi (string)
    if string = "on" then
        btoi = 1
    else
        btoi = 0
    end if
End Function

'   Converts ASCII date components yy mm dd to a date value.  Use this to
'   convert date components from the form into a long date value.
'
Function atod (yy, mm, dd)
    atod = makedate (CInt (yy), CInt (mm), CInt (dd))
End Function

Function atot (hh, mm, ss)
    atot = maketime (CInt (hh), CInt (mm), CInt (ss))
End Function

'   Converts time value to an 8-digit date value.  Use this to convert a 
'   date from VB's time value into a long date value. 
'
Function tvtod (timeval)
    if timeval = 0 then
        tvtod = 0
    else
        tvtod = makedate (year (timeval), month (timeval), day (timeval))
    end if
End Function

'   Converts VB time value to a long 8-digit time value.
'
Function tvtot (timeval)
    if timeval = 0 then
        tvtot = 0
    else
        tvtot = maketime (hour (timeval), minute (timeval), second (timeval))
    end if
End Function

'   Converts time value to a date value.  Use this to convert a date
'   from VB's time value into a long date value. 
'
Function dtotv (value)
    date_val = CInt (date_mm (value)) & "/" & CInt (date_dd (value)) & _
               "/" & CInt (date_yy (value))
    dtotv = CDate (date_val)
End Function

'   Compute future cutoff date.  Returns future or current date with
'   specified day of week.  ts argument should be date/time value.

Function cutoff (ts, day, time)
    if  WeekDay (ts) > day _
    or (WeekDay (ts) = day and tvtot (ts) > time) then
        cutoff = DateAdd ("d", day + 7 - WeekDay (ts), ts)
    else
        cutoff = DateAdd ("d", day - WeekDay (ts), ts)
    end if
End Function

'   Returns displayable date formatted as d Mmmmmmmm, yyyy in current language
'
Function showdate (value)
    if value > 0 then
        '  Japanese dates are formatted separately
        if Session ("current_LCID") = 1041 Then
            lcid = Session.LCID
            Session.LCID = Session ("current_LCID")
            showdate = FormatDateTime (dtotv (value), vbLongDate)
            Session.LCID = lcid
        else
            curmonth = monthtext (date_mm (value))
            select case cur_datefmt
                case "ymd"
                    showdate = date_yy (value) & " " & curmonth & " " & CInt (date_dd (value))
                case "mdy"
                    showdate = curmonth & " " & CInt (date_dd (value)) & ", " & date_yy (value)
                case else
                    showdate = CInt (date_dd (value)) & " " & curmonth & ", " & date_yy (value)
            end select
        end if
    else
        showdate = ""
    end if
End Function 

'   Returns date formatted as dd-Mmm-yyyy in current language
'
Function showdate_compact (value)
    if value > 0 then
        '  Japanese dates are formatted separately
        if Session ("current_LCID") = 1041 Then
            lcid = Session.LCID
            Session.LCID = Session ("current_LCID")
            showdate_compact = FormatDateTime (dtotv (value), vbLongDate)
            Session.LCID = lcid
        else
            curmonth = left (monthtext (date_mm (value)), 3)
            select case cur_datefmt
                case "ymd"
                    showdate_compact = date_yy (value) & " " & curmonth & " " & CInt (date_dd (value))
                case "mdy"
                    showdate_compact = curmonth & " " & CInt (date_dd (value)) & ", " & date_yy (value)
                case else
                    showdate_compact = CInt (date_dd (value)) & " " & curmonth & ", " & date_yy (value)
            end select
        end if
    else
        showdate_compact = ""
    end if
End Function 

'   Returns time formatted as HHhMM (time is hhmmsscc)
'
Function showtime (value, full)
    hh = int (value / 1000000)
    mm = int (value / 10000) mod 100
    ss = int (value / 100) mod 100
    cc = value mod 100

    if value > 0 then
        timestr = hh & ":"
        if mm < 10 then timestr = timestr & "0"
        timestr = timestr & mm
        if full then
            timestr = timestr & ":"
            if ss < 10 then timestr = timestr & "0"
            timestr = timestr & ss & ":"
            if cc < 10 then timestr = timestr & "0"
            timestr = timestr & cc
        end if
        showtime = timestr
    else
        showtime = ""
    end if
End Function 

'   Format timestamp value as string using cur_datefmt
'
Function showts (value)
    if value > 0 then
        showts = showdate_compact (pDate.ts2date (value)) & " " & showtime (pDate.ts2time (value), FALSE)
    else
        showts = ""
    end if
End Function

'   Format currency value using cur_moneyfmt and cur_showdecs
'   cur_moneyfmt is '.' or ',', indicating the decimal point to use.
'   This function can/should be replaced by the SFL sflcvns module.
'
Function showmoney (value)
    wholepart = Int (value / 100)
    fraction  = value - wholepart * 100
    result    = cstr (wholepart)
    '   Group by thousands
    if cur_moneyfmt = "," then
        separator = "."
    else
        separator = ","
    end if
    position = len (result) - 2
    do while position > 1
        result = left (result, position - 1) & separator & mid (result, position)
        position = position - 3
    loop
    if cur_showdecs then
        result = result & cur_moneyfmt
        if fraction < 10 then fraction = "0" & fraction
        result = result & fraction
    end if
    showmoney = result
End Function

'   Format number value using cur_moneyfmt.
'   cur_moneyfmt is '.' or ',', indicating the decimal point to use.
'   This function can/should be replaced by the SFL sflcvns module.
'
Function shownumber (value)
    result = CStr (CLng (value))
    '   Group by thousands
    if cur_moneyfmt = "," then
        separator = "."
    else
        separator = ","
    end if
    position = len (result) - 2
    do while position > 1
        result = left (result, position - 1) & separator & mid (result, position)
        position = position - 3
    loop
    shownumber = result
End Function

'   Formats the value as a string inside apostrophes.  Puts an apostrophe 
'   before and after the value, and doubles any apostrophe inside the value,
'   to escape it.   
'   "a" -> "'a'"   
'   "it's" -> "'it''s'"
'
Function astr (string)
    if isnull (string) then
        astr = "''"
    else
        astr = "'"
        For position = 1 To len (string)
            If Mid (string, position, 1) = "'" Then
                astr = astr & "''"
            Else
                astr = astr & Mid (string, position, 1)
            End If
        Next 
        astr = astr & "'"
    end if
End Function

'   Returns month name for cur_language
Function monthtext (month)
    dim months (12)
    months  (1) = "!January!"
    months  (2) = "!February!"
    months  (3) = "!March!"
    months  (4) = "!April!"
    months  (5) = "!May!"
    months  (6) = "!June!"
    months  (7) = "!July!"
    months  (8) = "!August!"
    months  (9) = "!September!"
    months (10) = "!October!"
    months (11) = "!November!"
    months (12) = "!December!"

    if month < 1 or month > 12 then
        monthtext = "!(Out of range)!"
    else
        if isnull (month) then
            monthtext = "!(null)!"
        else
            monthtext = months (month)
        end if
    end if
    monthtext = replace (monthtext, "!", "")
End Function

'   Returns day name for cur_language
Function daytext (day)
    dim days (7)
    days (1) = "!Sunday!"
    days (2) = "!Monday!"
    days (3) = "!Tuesday!"
    days (4) = "!Wednesday!"
    days (5) = "!Thursday!"
    days (6) = "!Friday!"
    days (7) = "!Saturday!"

    if day < 1 or day > 7 then
        daytext = "!(Out of range)!"
    else
        if isnull (month) then
            daytext = "!(null)!"
        else
            daytext = days (day)
        end if
    end if
    daytext = replace (daytext, "!", "")
End Function

'   Returns day name for cur_language
'
Function booltext (value)
    if value then
        booltext = "!Yes!"
    else
        booltext = "!No!"
    end if
    booltext = replace (booltext, "!", "")
End Function

'   Return text truncated to a certain length

Function choptext (value, maxlength)
    if len (value) > maxlength then
        choptext = left (value, maxlength - 3) & "..."
    else
        choptext = value
    end if
End Function

'   Builds select/radio header
'   Styles are: "drop down", "radio", "radio down"
'
select_style = ""
select_name  = ""

sub open_select (style, name)
    select_style = style
    select_name  = name
    if select_style = "drop down" then
        %><select name="<% = select_name %>"><%
    end if
end sub

'   Builds select/radio item
'
sub build_select (value, key, label, item_nbr)
    if select_style = "radio down" and item_nbr > 1 then
        %><br><%
    end if
    if select_style = "drop down" then
        %><option value="<%=key%>"<%
        If CStr (value) = key then
            %> selected<%
        end if
        %>><%=label%></option><%
    else
        %><input type="radio" value="<%=key%>" name="<%=select_name%>"<%
        If CStr (value) = key then
            %> checked<%
        end if
        %>>&nbsp;<%=label%>&nbsp;<%
    end if
    item_nbr = item_nbr + 1
end sub

'   Builds select/radio footer
'
sub close_select 
    if select_style = "drop down" then
        %></select><%
        if cursor_field = "" then
            cursor_field = select_name
        end if
    end if
end sub

'   Builds date select field
'
sub build_date (value, name, startyear, endyear)
    select case cur_datefmt
        case "ymd"
            build_date_year  value, name, startyear, endyear
            build_date_month value, name
            build_date_day   value, name
        case "mdy"
            build_date_month value, name
            build_date_day   value, name
            build_date_year  value, name, startyear, endyear
        case "dmy"
            build_date_day   value, name
            build_date_month value, name
            build_date_year  value, name, startyear, endyear
        case else
            build_date_day   value, name
            build_date_month value, name
            build_date_year  value, name, startyear, endyear
    end select
end sub

sub build_date_day (value, name)
    dd = date_dd (value)
    default_value = "Day"
    response.write "<select name=""" & name & "_dd""><option value=""0"">" & _
                    default_value & "</option>"
    For i = 1 To 31
        response.write "<option value=" & i
        if i = dd then response.write " selected"
        response.write ">" & i & "</option>"
    Next
    if cursor_field = "" then cursor_field = name & "_dd"
    response.write "</select>"
end sub

sub build_date_month (value, name)
    mm = date_mm (value)
    default_value = "Month"
    response.write "<select name=""" & name & "_mm""><option value=""0"">" & _
                   default_value & "</option>"
    For i = 1 To 12
        response.write "<option value=" & i
        if i = mm then response.write " selected"
        response.write ">" & monthtext (i) & "</option>"
    Next
    response.write "</select>"
end sub

sub build_date_year (value, name, startyear, endyear)
    yy = date_yy (value)
    default_value = "Year"
    response.write "<select name=""" & name & "_yy""><option value=""0"">" & _
                   default_value & "</option>"
    if startyear > endyear then
        For i = Year(Now()) + startyear To Year(Now()) + endyear step -1
            response.write "<option value=" & i
            if i = yy then response.write " selected"
            response.write ">" & i & "</option>"
        Next
    else
        For i = Year(Now()) + startyear To Year(Now()) + endyear
            response.write "<option value=" & i
            if i = yy then response.write " selected"
            response.write ">" & i & "</option>"
        Next
    end if
    response.write "</select>"
end sub

'   Builds time select field
'
sub build_time (value, name, starthour, endhour, interval)
    hh = int (value / 1000000)
    mm = int (value / 10000) mod 100

    response.write "<select name=""" & name & "_hh"">"
    For i = starthour To endhour
        if i <> hh then
            response.write "<option value=" & i & ">" & i & "</option>"
        else
            response.write "<option value=" & i & " selected>" & i & "</option>"
        end if
    Next
    if cursor_field = "" then cursor_field = name & "_hh"

    response.write "</select>h<select name=""" & name & "_mm"">"
    For i = 0 To 59 step interval
        if i <> mm then
            response.write "<option value=" & i & ">" & i & "</option>"
        else
            response.write "<option value=" & i & " selected>" & i & "</option>"
        end if
    Next
    response.write "</select>"
end sub

'   Translate value through APDB database lookup
'
'   table    name of table
'   key      name of key field
'   label    name of label field
'   value    current key value, passed through astr if textual
'   where    optional where clause, e.g. "language = 'EN'"
'
'   Returns label value if translation was found, else returns ""
'
function translate (table, key, label, value, where)
    sql = "SELECT " & label _
        & " FROM "  & table _
        & " WHERE " & key & " = " & value

    if where <> "" then
        sql = sql & " AND " & where
    end if

    set Rs = APDB.Execute (sql)
    if Rs.Eof then
        translate = ""
    else
        translate = Rs (label)
    end if
    Rs.Close
end function

sub define_icon (name, source)
%><script language="JavaScript"><!--
<%=name%>h = new Image; <%=name%>h.src = 'images/<%=source%>h.gif';
<%=name%>l = new Image; <%=name%>l.src = 'images/<%=source%>l.gif';
//-->
</script><%
end sub

sub show_icon (name, source, action, argval, active, hint)
    if active then
        %>&nbsp;<a href="javascript:formaction('<%=action%>',0,'<%=argval%>');"
   onMouseOver="hilite('ic_<%=name%>','<%=name%>h');window.status='';return true;"
   onMouseOut="hilite('ic_<%=name%>','<%=name%>l');window.status=''; return true;"><img name="ic_<%=name%>"
src="images/<%=source%>l.gif" alt="<%=hint%>" border=0 align=absmiddle></a><%
    else
        %><img src="images/<%=source%>l.gif" alt="<%=source%>" border=0 align=absbottom><%
    end if
end sub

'   This set of functions generates active hyperlinks
'
'   action  - passed back to ASP program
'   argval  - passed back to ASP program
'   fields  - if 1, mandatory fields are checked
'   confirm - if not "", shown as confirmation message
'   text    - actual text shown
'   hint    - popup or status bar hint
'
sub create_link (action, argval, fields, confirm, text, hint, styleclass)
    response.write create_link_text (action, argval, fields, confirm, text, hint, styleclass)
end sub

sub open_link (action, argval, fields, confirm, text, hint, styleclass)
    response.write open_link_text (action, argval, fields, confirm, text, hint, styleclass)
end sub

function create_link_text (action, argval, fields, confirm, text, hint, styleclass)
    create_link_text = open_link_text (action, argval, fields, confirm, text, hint, styleclass) & text & "</a>" & chr (13)
end function

function open_link_text (action, argval, fields, confirm, text, hint, styleclass)
    '   If action starts with '^', use MSIE accesskey attribute
    if left (action, 1) = "^" then
        accesskey = mid (action, 2, 1)
        action    = mid (action, 3)
        hint      = hint & " (Alt + " & accesskey & ")"
    end if
    '   Escape apostrophes if not using Japanese character set
    if Session.CodePage <> 932 then
        hint_value    = replace (hint, "'", "\'")
        confirm_value = replace (confirm, "'", "\'")
    end if

    if confirm = "" then
        open_link_text = "<a href=""javascript:formaction('" & action & "'," & fields & ",'" & argval & "')"""
    else
        open_link_text = "<a href=""javascript:confaction('" & action & "'," & fields & ",'" & argval & "','" & confirm_value & "')"""
    end if
    if accesskey <> "" then
        open_link_text = open_link_text & " ACCESSKEY=""" & accesskey & """"
    end if
    open_link_text = open_link_text & " class=" & styleclass & " title=""" & hint & """ OnMouseOver=""window.status='" & _
                   hint_value & "'; return true"" onmouseout=""window.status=''; return true;"">"
end function

sub create_button (action, argval, fields, confirm, text, hint)
    ' Handle single quote in arguments - this otherwise cause JavaScript errors
    hint_value    = replace (hint, "'", "\'")
    confirm_value = replace (confirm, "'", "\'")

    response.write "<input class=button type=button value="" " & text & " "" name=""" & action & """ onClick="""
    if confirm = "" then
        response.write "javascript:formaction('" & action & "'," & fields & ",'" & argval & "')"""
    else
        response.write "javascript:confaction('" & action & "'," & fields & ",'" & argval & "', '" & confirm_value & "')"""
    end if
    response.write " title=""" & hint & """ OnMouseOver=""window.status='" & hint_value & "'; return true"">"
end sub

function format_datetime (date_value, format_value)
    lcid = Session.LCID
    If Session ("current_LCID") > 0 And Session ("current_LCID") <> lcid then
        Session.LCID = Session ("current_LCID")
    End If
    format_datetime = FormatDateTime (date_value, format_value)
    Session.LCID = lcid
end function

'   response_redirect Safe implementation of response.redirect
'   Works with earlier browsers that do not support the 302 code.
'
sub response_redirect (url)
    Response.Buffer = TRUE
    Response.Clear
    Response.Status ="301 Moved"
    Response.AddHeader "Location", url
    Response.End
end sub
%>

