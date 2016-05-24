<%
'-------------------------------------------------------------------------
'   sub_formio.asp - Form i/o routines
'
'   These subroutines implement the form display and accept routines.
'   
'   These routines are used to add dynamic forms to a screen.  Data is
'   captured as an XML string, which can be stored in the database as-is
'   or broken into fields, depending on the application's needs.
'   The form routines provide for all iAF form data types.
'   The data definitions required by this module are in stdform.dfl.
'
'   Form id must be provided in Session ("form_id").
'   If form id is zero, form is not displayed.
'   The form data is provided and updated in form_data.
'   After form_accept, this variable is updated with new contents.
'
'   Global form variables

if not IsObject (Session ("obj_form")) then
    set Session ("obj_form")     = Server.CreateObject ("scl.XMLstore")
end if
if not IsObject (Session ("obj_fields")) then
    set Session ("obj_fields")   = Server.CreateObject ("scl.XMLstore")
end if
if not IsObject (Session ("obj_values")) then
    set Session ("obj_values")   = Server.CreateObject ("scl.XMLstore")
end if
set pForm   = Session ("obj_form")
set pFields = Session ("obj_fields")
set pValues = Session ("obj_values")

field_id         = 0 
field_name       = ""
field_type       = 0
field_label      = ""
field_join       = 0
field_break      = 0
field_rule       = 0
field_required   = 0
field_showsize   = 0
field_maxsize    = 0
field_default    = ""
field_lockable   = 0
field_cursor     = 0
field_uppercase  = 0
field_blankzero  = 0
field_value      = ""
field_match_mask = 0
field_mask       = ""
field_filtered   = 0
%>
<!--#include file="form.asp"      -->
<!--#include file="field.asp"     -->
<!--#include file="fieldvalue.asp"-->
<!--#include file="fieldrole.asp"-->
<!--#include file="uservalue.asp"-->
<%

'   These constants are copied from field_update.asp
if FIELDTYPE_TEXT = 0 then
    FIELDTYPE_TEXT      = 1
    FIELDTYPE_TEXTBOX   = 2
    FIELDTYPE_DATE      = 3
    FIELDTYPE_TIME      = 4
    FIELDTYPE_NUMBER    = 5
    FIELDTYPE_SELECT    = 6
    FIELDTYPE_RADIO     = 7
    FIELDTYPE_CHECKBOX  = 8
    FIELDTYPE_LABEL     = 9

'   These constants are copied from fieldrule_update.asp
    FIELDRULE_BOTH            = 1
    FIELDRULE_EITHER          = 2
    FIELDRULE_FORMATTED       = 3
    FIELDRULE_DEPENDENT       = 4
    FIELDRULE_FORMATDEP       = 5
    FIELDRULE_FORVALUE        = 6
    FIELDRULE_FORVALUE_EMPTY  = 7
end if

'-------------------------------------------------------------------------
'   Fetches Session ("form_id") as specified

sub form_fetch (parentname, parentid, objectname, fieldrole)
    oalstring = _
        "<oal do=""query"" user=""" & cur_userid & """ query=""byobject"" limit = ""1"" control=""eq"">" & _
        "<form>"                                        & _
        "<parentname>" & parentname & "</parentname>"   & _
        "<parentid>"   & parentid   & "</parentid>"     & _
        "<objectname>" & objectname & "</objectname>"   & _
        "</form></oal>"
    pform.value = oa_do_form (oalstring)
    'response.write pstring.htmlencode (oalstring) & "<hr>"
    'response.write pstring.htmlencode (pform.value) & "<hr>"
    if pForm.attr ("done") = "ok" and CInt (pForm.attr ("count", 0)) > 0 then 
        pForm.item_first_child
        Session ("form_id") = CInt (pForm.item_child_value ("id", 0))
        pForm.item_root
        form_fetch_fields fieldrole
    else
        Session ("form_id") = 0
    end if
end sub

sub form_fetch_fields (fieldrole)
    If fieldrole = "" Then
        oalstring = _
            "<oal do=""query"" user=""" & cur_userid & """ query=""byform"" limit=""500"">" & _
            "<field><formid match=""1"">" & Session ("form_id") & "</formid></field></oal>"
        pFields.value = oa_do_field (oalstring)
    Else
        ' Make query string for field role 
        oalstring = _
            "<oal do=""query"" user=""" & cur_userid & """ query=""byrole"" limit=""500"">" & _
            "<fieldrole><formid match=""1"">" & Session ("form_id") & "</formid>"           & _
            "<role match = ""1"">" & fieldrole & "</role></fieldrole></oal>"
        ' Create required SCL component
        Set xml_fieldrole = Server.CreateObject ("scl.xmlstore")
        Set xml_field     = Server.CreateObject ("scl.xmlstore")
        ' Reset field xml tree
        fields = "<oal>"

        ' Execute query
        xml_fieldrole.value = oa_do_fieldrole (oalstring)
        'response.write pstring.htmlencode (oalstring) & "<hr>"
        'response.write pstring.htmlencode (xml_fieldrole.value) & "<hr>"

        if xml_fieldrole.attr ("done") = "ok" and CInt (xml_fieldrole.attr ("count", 0)) > 0 then 
            xml_fieldrole.item_first_child
            do while xml_fieldrole.valid
                oalstring = "<oal do=""query"" query=""detail"" control = ""eq"" view=""detail"" " & _
                            "limit = ""1""user = """ & cur_userid & _
                            """><field><id match=""1"">" & xml_fieldrole.item_child_value ("fieldid")  & _
                            "</id></field></oal>"
                xml_field.value = oa_do_field (oalstring)
                'response.write pstring.htmlencode (oalstring) & "<hr>"
                'response.write pstring.htmlencode (xml_field.value) & "<hr>"
                if xml_field.attr ("done") = "ok" then
                    xml_field.item_first_child
                    fields = fields & xml_field.xmlstring
                end if
                xml_fieldrole.item_next
            loop
            pFields.value = fields & "</oal>"
        end if
        'response.write pstring.htmlencode (pfields.value) & "<hr>"
        Set xml_field     = Nothing
        Set xml_fieldrole = Nothing
    End If
end sub

sub form_display (mode, user_id, fieldrole)
    if Session ("form_id") = 0 then exit sub

    '   Fetch form only if necessary
    pForm.item_root
    pForm.item_first_child

    if Session ("form_id") <> CInt (pForm.item_child_value ("id", 0)) then
        pForm.value = oa_do_form ( _
            "<oal do=""fetch"" user=""" & cur_userid & """ view=""detail"">" & _
            "<form><id>" & Session ("form_id") & "</id></form></oal>")
        form_fetch_fields fieldrole
    end if

    '   Save context
    Session ("form_value")   = pForm.value
    Session ("fields_value") = pFields.value
    pValues.value            = form_data

    '   Now display the form
    field_open = 0
    pFields.item_first_child
    do while pFields.valid
        field_id        = CInt (pFields.item_child_value ("id",        0))
        field_type      = CInt (pFields.item_child_value ("type",      0))
        field_label     =       pFields.item_child_value ("label",     "No label")
        field_join      = CInt (pFields.item_child_value ("join",      0))
        field_break     = CInt (pFields.item_child_value ("break",     0))
        field_rule      = CInt (pFields.item_child_value ("rule",      0))
        field_required  = CInt (pFields.item_child_value ("required",  0))
        field_showsize  = CInt (pFields.item_child_value ("showsize", 20))
        field_maxsize   = CInt (pFields.item_child_value ("maxsize",  50))
        field_lockable  = CInt (pFields.item_child_value ("lockable",  0))
        field_cursor    = CInt (pFields.item_child_value ("cursor",    0))
        field_uppercase = CInt (pFields.item_child_value ("uppercase", 0))
        field_blankzero = CInt (pFields.item_child_value ("blankzero", 0))
        field_filtered  = CInt (pFields.item_child_value ("filtered",  0))
        field_value     = pValues.attr (pFields.item_child_value ("name"), "")
        if field_value = "" then
            field_value   = pFields.item_child_value ("default", "")
            field_default = field_value
        end if

        if mode = "input" and field_required = 1 then
            field_label = "<b>" & field_label & "</b>"
        else        
            field_label = field_label
        end if

        if field_join = 1 and field_open = 1 then
            response.write "&nbsp;"
            if mode = "design" then
                create_link "DO_EDIT", field_id, 0, "", field_label, "Click to edit field", "action"
            else
                response.write "</b>" & field_label
            end if
            response.write "&nbsp;"
        else
            if field_open = 1 then
                response.write "</td></tr>"
            end if
            response.write "<tr><td height=20 valign=top>"
            if mode = "design" then
                create_link "DO_EDIT", field_id, 0, "", field_label, "Click to edit field", "action"
            else
                response.write field_label
            end if
            response.write "&nbsp;</td><td valign=top"
            if mode = "design" then
                response.write ">"
            else
                response.write " bgcolor=" & Session ("TABLEROWCOLOR1") & ">"
            end if
        end if
        if field_required then
            response.write "<b>"
        end if
        field_open = 1
        field_name = "x" & field_id

        if mode = "input" or mode = "design" then
            select case field_type
                case FIELDTYPE_TEXT
                    form_textual_input  
                case FIELDTYPE_TEXTBOX 
                    form_textbox_input  
                case FIELDTYPE_DATE    
                    form_date_input     
                case FIELDTYPE_TIME    
                    form_time_input     
                case FIELDTYPE_NUMBER  
                    form_number_input   
                case FIELDTYPE_SELECT  
                    form_select_input "drop down", user_id
                case FIELDTYPE_RADIO   
                    form_select_input "radio", user_id
                case FIELDTYPE_CHECKBOX
                    form_checkbox_input 
            end select
        elseif mode = "search" then
            select case field_type
                case FIELDTYPE_TEXT
                    form_textual_input  
                case FIELDTYPE_TEXTBOX 
                    form_textbox_input  
                case FIELDTYPE_DATE    
                    form_date_input     
                case FIELDTYPE_TIME    
                    form_time_input     
                case FIELDTYPE_NUMBER  
                    form_number_search_input   
                case FIELDTYPE_SELECT  
                    form_select_checkbox_input user_id
                case FIELDTYPE_RADIO   
                    form_select_checkbox_input user_id
                case FIELDTYPE_CHECKBOX
                    form_checkbox_input 
            end select
        else
            select case field_type
                case FIELDTYPE_TEXT
                    form_textual_output 
                case FIELDTYPE_TEXTBOX 
                    form_textbox_output 
                case FIELDTYPE_DATE    
                    form_date_output    
                case FIELDTYPE_TIME    
                    form_time_output    
                case FIELDTYPE_NUMBER  
                    form_number_output  
                case FIELDTYPE_SELECT, FIELDTYPE_RADIO
                    form_select_output user_id
                case FIELDTYPE_CHECKBOX
                    form_checkbox_output
            end select
            response.write "</b>"
        end if
        if field_break = 1 then
            if field_open = 1 then
                response.write "</td></tr>"
            end if
            response.write "<tr><td colspan=99>&nbsp;</td></tr>"
            field_open = 0
        end if
        if field_rule = 1 then
            if field_open = 1 then
                response.write "</td></tr>"
            end if
            response.write "<tr><td colspan=99 bgcolor=" & Session ("TABLEROWCOLOR1") & ">&nbsp;</td></tr>"
            field_open = 0
        end if
        pFields.item_next
    loop
end sub

'-------------------------------------------------------------------------

sub form_textual_input 
    response.write "<input size="      & field_showsize & _
                         " maxlength=" & field_maxsize & _
                         " value="""   & pstring.htmlencode (field_value) & """" & _
                         " name="""    & field_name & """ onFocus=""this.form." & field_name & ".select();"""
    if field_required = 1 then response.write " class=force"
    response.write ">"
    if cur_cursor = "" then cur_cursor = field_name
end sub

sub form_textbox_input
    response.write "<textarea rows=5 cols=50" & _
                         " maxlength=" & field_maxsize & _
                         " name="""    & field_name & """ onFocus=""this.form." & field_name & ".select();"""
    if field_required = 1 then response.write " class=force"
    response.write ">" & pstring.htmlencode (field_value) & "</textarea>"
    if cur_cursor = "" then cur_cursor = field_name
end sub

sub form_date_input
    if field_value = "" then field_value = tvtod (Now)
    build_date field_value, field_name, +10, -70
end sub

sub form_time_input
    if field_value = "" then field_value = 0
    build_time field_value, field_name, 0, 23, 15
    if cur_cursor = "" then cur_cursor = field_name
end sub

sub form_number_input
    if field_value = "" or not IsNumeric (field_value) then
        field_value = 0
    end if
    if field_blankzero = 1 and field_value = 0 then
        field_value = ""
    end if
    response.write "<input size="      & field_showsize & _
                         " maxlength=" & field_maxsize & _
                         " value="""   & field_value & """" & _
                         " name="""    & field_name & """ onFocus=""this.form." & field_name & ".select();"""
    if field_required = 1 then response.write " class=force"
    response.write ">"
    if cur_cursor = "" then cur_cursor = field_name
end sub

sub form_number_search_input
    response.write "<input size="      & field_showsize & _
                         " maxlength=" & field_maxsize * 4 & _
                         " value="""   & field_value & """" & _
                         " name="""    & field_name & """ onFocus=""this.form." & field_name & ".select();"""
    if field_required = 1 then response.write " class=force"
    response.write ">"
    if cur_cursor = "" then cur_cursor = field_name
end sub

sub form_select_input (style, user_id)
    item_nbr = 1
    open_select style, field_name
    if style = "drop down" then
        build_select field_value, "", "No selection", item_nbr
    end if
    set selectvalues = Server.CreateObject ("scl.XMLstore")
    selectvalues.value = get_select_value (0, user_id)
    selectvalues.item_first_child
    do while selectvalues.valid
        build_select field_value, _
                     selectvalues.item_child_value ("encoding"), _
                     selectvalues.item_child_value ("shownvalue"), _
                     item_nbr
        selectvalues.item_next
    loop
    close_select
    set selectvalues = Nothing
end sub

sub form_checkbox_input
    if not IsNumeric (field_value) then field_value = 0
    response.write "<input type=checkbox name=""" & field_name & """"
    if field_value = 1 then response.write " checked"
    response.write ">"
    if cur_cursor = "" then cur_cursor = field_name
end sub

sub form_select_checkbox_input (user_id)
    item_nbr = 1
    set selectvalues = Server.CreateObject ("scl.XMLstore")
    selectvalues.value = get_select_value (0, user_id)
    selectvalues.item_first_child
    do while selectvalues.valid
        response.write selectvalues.item_child_value ("shownvalue")
        full_value = selectvalues.item_child_value ("encoding")
        If    InStr (field_value, " " & full_value & " ") > 0 _
           Or Left (field_value, len (full_value)) = full_value Then
             check_value = 1
        else
             check_value = 0
        end if
        response.write "<input type=checkbox name=""" & field_name & _
                        "_" & selectvalues.item_child_value ("encoding") & """"
        if check_value = 1 then response.write " checked"
        response.write ">"
        selectvalues.item_next
    loop
    set selectvalues = Nothing
end sub

'-------------------------------------------------------------------------

sub form_textual_output
    response.write pstring.htmlencode (field_value)
end sub

sub form_textbox_output
    response.write pstring.htmlencode (field_value)
end sub

sub form_date_output
    if not IsNumeric (field_value) then field_value = 0
    response.write showdate (field_value)
end sub

sub form_time_output
    if not IsNumeric (field_value) then field_value = 0
    response.write showtime (field_value,true)
end sub

sub form_number_output
    if field_value = "" then
        field_value = 0
    elseif field_blankzero = 1 and field_value = 0 then
        field_value = ""
    end if
    response.write field_value
end sub

sub form_select_output (user_id)
    set selectvalues = Server.CreateObject ("scl.XMLstore")
    selectvalues.value = get_select_value (1, user_id)
    selectvalues.item_first_child
    do while selectvalues.valid
        if field_value = selectvalues.item_child_value ("encoding") then
            response.write selectvalues.item_child_value ("shownvalue")
            exit do
        end if
        selectvalues.item_next
    loop
    set selectvalues = Nothing
end sub

sub form_checkbox_output
    if not IsNumeric (field_value) then field_value = 0
    response.write booltext (field_value)
end sub

'-------------------------------------------------------------------------

sub form_accept (check_required_fields)
    if Session ("form_id") = 0 then exit sub

    '   Restore context
    pForm.value   = Session ("form_value")
    pFields.value = Session ("fields_value")
    form_data     = "<data "
    pFields.item_first_child
    do while pFields.valid
        field_id         = CInt (pFields.item_child_value ("id", 0))
        field_label      =       pFields.item_child_value ("label")
        field_type       = CInt (pFields.item_child_value ("type"))
        field_uppercase  = CInt (pFields.item_child_value ("uppercase"))
        field_mask       =       pFields.item_child_value ("mask")
        field_name       = "x" & field_id
        field_value      = ""
        field_match_mask = 0

        if check_required_fields = 0 then
            field_required = 0
        else
            field_required = CInt (pFields.item_child_value ("required"))
        end if

        get_field_value ()

        if field_required = 1 and field_value = "" and exception_raised = FALSE then
            cur_message = field_label & " is a required field - please supply it"
            cur_error   = cur_error_field
            exception_raised = TRUE
        end if
        if field_match_mask > 0 Then
            symbols.clear
            symbols.add "name",     field_name
            symbols.add "mask",     field_mask
            symbols.add "position", field_match_mask
            message_string = "$(name) does not match $(mask) in position $(position)"
            cur_message = symbols.substitute (message_string)
            cur_error   = cur_error_field
            exception_raised = TRUE
        end if

        verify_field_rule (pFields)

        field_value = pstring.htmlencode (field_value)
        form_data = form_data & pFields.item_child_value ("name") & " = """ & field_value & """ "
        pFields.item_next
    loop
    form_data = form_data & "/>"
end sub

'-------------------------------------------------------------------------

sub form_accept_search (check_required_fields, user_id)
    if Session ("form_id") = 0 then exit sub

    '   Restore context
    pForm.value   = Session ("form_value")
    pFields.value = Session ("fields_value")
    form_data     = "<data "
    pFields.item_first_child
    do while pFields.valid
        field_id        = CInt (pFields.item_child_value ("id", 0))
        field_label     =       pFields.item_child_value ("label")
        field_type      = CInt (pFields.item_child_value ("type"))
        field_uppercase = CInt (pFields.item_child_value ("uppercase"))
        field_mask      = ""
        field_name      = "x" & field_id
        field_value     = ""

        get_field_search_value (user_id)

        if field_value <> "" then
            field_value = pstring.htmlencode (field_value)
            form_data = form_data & pFields.item_child_value ("name") & " = """ & field_value & """ "
        end if
        pFields.item_next
    loop
    form_data = form_data & "/>"
end sub

'-------------------------------------------------------------------------

sub get_field_value
    select case field_type
        case FIELDTYPE_TEXT
            form_textual_accept                
            cur_error_field = field_name
        case FIELDTYPE_TEXTBOX 
            form_textbox_accept  
            cur_error_field = field_name
        case FIELDTYPE_DATE    
            form_date_accept     
        case FIELDTYPE_TIME    
            form_time_accept     
        case FIELDTYPE_NUMBER  
            form_number_accept   
            cur_error_field = field_name
        case FIELDTYPE_SELECT, FIELDTYPE_RADIO
            form_select_accept
        case FIELDTYPE_CHECKBOX
            form_checkbox_accept 
            cur_error_field = field_name
    end select
end sub

sub get_field_search_value (user_id)
    select case field_type
        case FIELDTYPE_TEXT
            form_textual_accept                
            cur_error_field = field_name
        case FIELDTYPE_TEXTBOX 
            form_textbox_accept  
            cur_error_field = field_name
        case FIELDTYPE_DATE    
            form_date_accept     
        case FIELDTYPE_TIME    
            form_time_accept     
        case FIELDTYPE_NUMBER  
            form_textual_accept                
            cur_error_field = field_name
        case FIELDTYPE_SELECT, FIELDTYPE_RADIO
            form_select_search_accept user_id
        case FIELDTYPE_CHECKBOX
            form_checkbox_accept 
            cur_error_field = field_name
            if field_value = 0 then field_value = ""
    end select
end sub

sub form_textual_accept 
    if have_attachments then
        field_value = trim (Upload.Form (field_name))
    else
        field_value = trim (Request.Form (field_name))
    end if
    if field_uppercase then field_value = ucase (field_value)
    check_mask_format ()
end sub

sub form_textbox_accept
    if have_attachments then
        field_value = trim (Upload.Form (field_name))
    else
        field_value = trim (Request.Form (field_name))
    end if
    check_mask_format ()
end sub

sub form_date_accept
    if have_attachments then
        field_value = atod (Upload.Form (field_name & "_yy"), _
                            Upload.Form (field_name & "_mm"), _
                            Upload.Form (field_name & "_dd"))
    else
        field_value = atod (Request.Form (field_name & "_yy"), _
                            Request.Form (field_name & "_mm"), _
                            Request.Form (field_name & "_dd"))
    end if
end sub

sub form_time_accept
    if have_attachments then
        field_value = atot (Upload.Form (field_name & "_hh"), _
                            Upload.Form (field_name & "_mm"), 00)
    else
        field_value = atot (Request.Form (field_name & "_hh"), _
                            Request.Form (field_name & "_mm"), 00)
    end if
end sub

sub form_number_accept
    if have_attachments then
        field_value = Upload.Form (field_name)
    else
        field_value = Request.Form (field_name)
    end if
    if field_value <> "" and not IsNumeric (field_value) then
        cur_message = "Please enter a valid number"
        cur_error   = field_name
        exception_raised = TRUE
    else
        check_mask_format ()
    end if
end sub

sub form_select_accept
    if have_attachments then
        field_value = Upload.Form (field_name)
    else
        field_value = Request.Form (field_name)
    end if
end sub

sub form_select_search_accept (user_id)
    set selectvalues = Server.CreateObject ("scl.XMLstore")
    selectvalues.value = get_select_value (0, user_id)
    selectvalues.item_first_child
    do while selectvalues.valid
        temp_field_name = field_name & "_" & _
                         selectvalues.item_child_value ("encoding")
        if have_attachments then
            temp_field_value = btoi (Upload.Form (temp_field_name))
        else
            temp_field_value = btoi (Request.Form (temp_field_name))
        end if
        if temp_field_value = 1 then
            field_value = field_value                                & _
                          selectvalues.item_child_value ("encoding") & _
                          " "
        end if
        selectvalues.item_next
    loop
    set selectvalues = Nothing
end sub

sub form_checkbox_accept
    if have_attachments then
        field_value = btoi (Upload.Form (field_name))
    else
        field_value = btoi (Request.Form (field_name))
    end if
end sub

'-------------------------------------------------------------------------

sub check_mask_format
  If not IsNull (field_mask) and field_value <> ""  Then
      field_match_mask = pString.mask (field_value, field_mask)
  End If
end sub

'-------------------------------------------------------------------------

function is_empty_field ()
    is_empty_field = FALSE
    select case field_type
        case FIELDTYPE_TEXT, FIELDTYPE_TEXTBOX 
            if field_value = "" then
                is_empty_field = TRUE
            end if
        case FIELDTYPE_NUMBER, FIELDTYPE_CHECKBOX, FIELDTYPE_DATE, FIELDTYPE_TIME
            if field_value = "0" then
                is_empty_field = TRUE
            end if            
        case FIELDTYPE_SELECT, FIELDTYPE_RADIO
            if field_value = "" or field_value = "space" then
                is_empty_field = TRUE
            end if
    end select
end function

'-------------------------------------------------------------------------

sub verify_field_rule (pfields)
    'Get target label
    target_name = pfields.item_child_value ("label")
    ' Save affected global variable
    pfields.push_position
    old_field_value = field_value
    old_field_type  = field_type
    old_field_name  = field_name    
    old_field_empty = is_empty_field ()

    source_name     = ""

    ' Search fieldrule record, child of field id item
    pfields.item_set_current ("id")
    pfields.item_first_child
    do while pfields.valid And exception_raised = FALSE
        source_id = pfields.item_child_value ("sourceid")
        field_name = "x" & source_id
        'Get type of source field
        pfields.push_position
        pfields.item_root
        pfields.item_first_child
        do while pfields.valid
            If pfields.item_child_value ("id") = source_id Then
                field_type = CInt (pfields.item_child_value ("type"))
                source_name = pfields.item_child_value ("label")
                Exit do
            End If
            pfields.item_next
        loop
        pfields.pop_position
        
        'Get source field value
        get_field_value ()
        field_empty = is_empty_field ()
        'Check type of rule
        select case CLng (pfields.item_child_value ("rule"))
            case FIELDRULE_BOTH
                ' If source populated, target must be populated
                If field_empty = FALSE And old_field_empty Then
                    'symbols is used for translation of message
                    symbols.clear
                    symbols.add "source", source_name
                    symbols.add "target", target_name
                    message_string = "If $(source) is entered, $(target) is required too"
                    cur_message = symbols.substitute (message_string)
                    cur_error   = old_field_name
                    exception_raised = TRUE
                End If

            case FIELDRULE_EITHER
                ' If source populated, target must be empty
                If field_empty = FALSE And old_field_empty = FALSE Then
                    'symbols is used for translation of message
                    symbols.clear
                    symbols.add "source", source_name
                    symbols.add "target", target_name
                    message_string = "If $(source) is entered, $(target)  must be empty"
                    cur_message = symbols.substitute (message_string)
                    cur_error   = old_field_name
                    exception_raised = TRUE
                End If

            case FIELDRULE_FORMATTED
               ' Source follows format
               If field_empty = FALSE Then
                   field_mask = pfields.item_child_value ("sourceparam")
                   check_mask_format ()
                   if field_match_mask > 0 Then
                        symbols.clear
                        symbols.add "name",     field_name
                        symbols.add "mask",     field_mask
                        symbols.add "position", field_match_mask
                        message_string = "$(name) does not match $(mask) in position $(position)"
                        cur_message = symbols.substitute (message_string)
                       cur_error   = source_name
                       exception_raised = TRUE
                   end if
               End If

            case FIELDRULE_DEPENDENT
               ' If source populated, target follow format
               If field_empty = FALSE Then
                   field_mask  = pfields.item_child_value ("targetparam")
                   field_value = old_field_value
                   check_mask_format ()
                   if field_match_mask > 0 Then
                        symbols.clear
                        symbols.add "name",     field_name
                        symbols.add "mask",     field_mask
                        symbols.add "position", field_match_mask
                        message_string = "$(name) does not match $(mask) in position $(position)"
                        cur_message = symbols.substitute (message_string)
                       cur_error   = old_field_name
                       exception_raised = TRUE
                   end if
               End If

            case FIELDRULE_FORMATDEP
              ' If source follow source format, target follow target format
               field_mask  = pfields.item_child_value ("sourceparam")
               check_mask_format ()
               If field_match_mask = 0 Then
                   field_mask  = pfields.item_child_value ("targetparam")
                   field_value = old_field_value
                   check_mask_format ()
                   if field_match_mask > 0 Then
                        symbols.clear
                        symbols.add "name",     field_name
                        symbols.add "mask",     field_mask
                        symbols.add "position", field_match_mask
                        message_string = "$(name) does not match $(mask) in position $(position)"
                        cur_message = symbols.substitute (message_string)
                        cur_error   = old_field_name
                        exception_raised = TRUE
                   end if
               End If               
            case FIELDRULE_FORVALUE
               fieldvalue_id  = pfields.item_child_value ("sourceparam")
               if field_value = fieldvalue_id and old_field_empty Then
                    symbols.clear
                    symbols.add "source", source_name
                    symbols.add "target", target_name
                    message_string = "If $(source) have this value, $(target) is required"
                    cur_message = symbols.substitute (message_string)
                    cur_error       = old_field_name
                   exception_raised = TRUE
               end if
            case FIELDRULE_FORVALUE_EMPTY
               fieldvalue_id  = pfields.item_child_value ("sourceparam")
               if field_value = fieldvalue_id and old_field_empty = FALSE Then
                    symbols.clear
                    symbols.add "source", source_name
                    symbols.add "target", target_name
                    message_string = "If $(source) have this value, $(target) must be empty"
                    cur_message = symbols.substitute (message_string)
                    cur_error       = old_field_name
                   exception_raised = TRUE
               end if
        end select
        pfields.item_next
    loop   

    'Restore affected global variable
    field_name  = old_field_name
    field_type  = old_field_type
    field_value = old_field_value
    pfields.pop_position
end sub

'------------------------------------------------------------------------------

function get_form_display_text (user_id)
    dim stroutput
    if Session ("form_id") = 0 then exit function

    '   Fetch form only if necessary
    pForm.item_root
    pForm.item_first_child
    if Session ("form_id") <> CInt (pForm.item_child_value ("id", 0)) then
        pForm.value = oa_do_form ( _
            "<oal do=""fetch"" user=""" & cur_userid & """ view=""detail"">" & _
            "<form id=form1 name=form1><id>" & Session ("form_id") & "</id></form></oal>")
        pFields.value = oa_do_field ( _
            "<oal do=""query"" user=""" & cur_userid & """ query=""byform"" limit=""1000"">" & _
            "<field><formid match=""1"">" & Session ("form_id") & "</formid></field></oal>")
    end if
    'Response.Write pstring.htmlencode (pForm.value ) & "<HR>"
    'Response.Write pFields.value & "<HR>"
    'Response.Write pValues.value  & "<HR>"
    'Response.Write fld_submission_formdata  & "<HR>"
    'Response.Write Session("form_data")  & "<HR>"
    'Response.Write form_data  & "<HR>"
    
    '   Save context
    Session ("form_value")   = pForm.value
    Session ("fields_value") = pFields.value
    pValues.value = Session("form_data")  'fld_submission_formdata 'form_data

    '   Now display the form
    field_open = 0
    pFields.item_first_child
    do while pFields.valid
        field_id        = CInt (pFields.item_child_value ("id",        0))
        field_type      = CInt (pFields.item_child_value ("type",      0))
        field_label     =       pFields.item_child_value ("label",     "No label") & ":"
        field_join      = CInt (pFields.item_child_value ("join",      0))
        field_break     = CInt (pFields.item_child_value ("break",     0))
        field_rule      = CInt (pFields.item_child_value ("rule",      0))
        field_required  = CInt (pFields.item_child_value ("required",  0))
        field_showsize  = CInt (pFields.item_child_value ("showsize", 20))
        field_maxsize   = CInt (pFields.item_child_value ("maxsize",  50))
        field_lockable  = CInt (pFields.item_child_value ("lockable",  0))
        field_cursor    = CInt (pFields.item_child_value ("cursor",    0))
        field_uppercase = CInt (pFields.item_child_value ("uppercase", 0))
        field_blankzero = CInt (pFields.item_child_value ("blankzero", 0))
        field_value     = pValues.attr (pFields.item_child_value ("name"), "")
        if field_value = "" then
            field_value = pFields.item_child_value ("default", "")
        end if

        if field_join = 1 and field_open = 1 then
            stroutput = stroutput & " " & field_label
        else
            if field_open = 1 then
                stroutput = stroutput & vbcrlf
            end if
            stroutput = stroutput & field_label & " "
        end if
        field_open = 1
        field_name = "x" & field_id

        select case field_type
             case FIELDTYPE_TEXT,FIELDTYPE_TEXTBOX
                 stroutput = stroutput & field_value 
             case FIELDTYPE_DATE    
                if not IsNumeric (field_value) then field_value = 0
                stroutput = stroutput & showdate (field_value)    
             case FIELDTYPE_TIME    
                if not IsNumeric (field_value) then field_value = 0
                stroutput = stroutput & showtime (field_value,true)    
             case FIELDTYPE_NUMBER  
                if field_value = "" then
                    field_value = 0
                elseif field_blankzero = 1 and field_value = 0 then
                    field_value = ""
                end if
                stroutput = stroutput & field_value
             case FIELDTYPE_SELECT, FIELDTYPE_RADIO
                set selectvalues = Server.CreateObject ("scl.XMLstore")
                selectvalues.value = get_select_value (1, user_id)
                selectvalues.item_first_child
                do while selectvalues.valid
                    if field_value = selectvalues.item_child_value ("encoding") then
                        stroutput = stroutput & selectvalues.item_child_value ("shownvalue")
                        exit do
                    end if
                    selectvalues.item_next
                loop
                set selectvalues = Nothing
             case FIELDTYPE_CHECKBOX
                if not IsNumeric (field_value) then field_value = 0
                stroutput = stroutput & booltext (field_value)
        end select

        if field_break = 1 or field_rule = 1 then
            stroutput = stroutput & vbcrlf
            field_open = 0
        end if
        pFields.item_next
    loop
    if stroutput <> "" then
        get_form_display_text = vbcrlf & stroutput
    else
        get_form_display_text = ""
    end if
end function

'------------------------------------------------------------------------------

function get_select_value (output_mode, user_id)
    if field_filtered = 0 or user_id = 0 Then
        get_select_value = oa_do_fieldvalue ( _
                                "<oal do=""query"" user=""" & cur_userid & _
                                """ control = ""first"" query=""byfield"" limit=""500"">" & _
                                "<fieldvalue><fieldid match=""1"">" & field_id & _
                                "</fieldid></fieldvalue></oal>")
    else
        get_select_value = oa_do_uservalue ("<oal do=""query"" user=""" & user_id & _
                                """ query=""byuser"" limit=""100"" control = ""first"" >" & _
                                "<uservalue><fieldid match=""1"">"   & field_id   & _
                                "</fieldid><userid match=""1"">"     & user_id & _
                                "</userid></uservalue></oal>")
        ' set the default value
        if (field_value = "" or field_value = field_default) and output_mode = 0 then
           set data_value = Server.CreateObject ("scl.XMLstore")
           data_value.value = get_select_value
           data_value.item_first_child
           do while data_value.valid
               if data_value.item_child_value ("defaultval", "0") = "1" then
                    field_value = data_value.item_child_value ("encoding")
               end if
               data_value.item_next
           loop
           set data_value = Nothing           
       end if
    end if
end function
%>
