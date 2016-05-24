<%
'-------------------------------------------------------------------------
'   sub_inherit_form.asp - Form inheritence
'
'   Written: 2000/04/02  Pieter Hintjens
'   Revised: 2000/08/01  Pieter Hintjens
'
'   Inherits a form as specified in
'       Session ("form_parent_name")
'       Session ("form_parent_id")
'       Session ("form_object_name")
'
'   From the form, if present, attached to the object:
'       Session ("form_inherit_name")
'       Session ("form_inherit_id")
'

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

sub form_inherit
    form_fetch Session ("form_inherit_name"),_
               Session ("form_inherit_id"),_
               Session ("form_object_name"),_
               ""

    if Session ("form_id") <> 0 then
        fld_form_id = create_form_object ()
        
        '   Create fields in form
        pFields.item_first_child
        do while pFields.valid
            '   Create new field item
            pxml.value = "<oal/>"
            pxml.attr ("do") = "create"
            pxml.item_new         "field"
            pxml.item_set_current "field"
            source_field = pFields.item_child_value ("id")
            pFields.push_position
            pFields.item_first_child
            do while pFields.valid
                pxml.item_new pFields.item_name, pFields.item
                pFields.item_next
            loop
            pFields.item_parent
            pxml.item_set_current "formid"
            pxml.item = fld_form_id
            reply = oa_do_field (pxml.value)
            pxml.value = reply
            pxml.item_first_child
            fld_field_id = CLng (pxml.item_child_value ("id", 0))
            
            'If field has values, copy those as well
            oalstring = _
                "<oal do=""query"" user=""" & cur_userid & """ query=""byfield"" limit=""100"">" & _
                "<fieldvalue><fieldid match=""1"">" & source_field & "</fieldid></fieldvalue></oal>"
            pValues.value = oa_do_fieldvalue (oalstring)
           
            pValues.item_first_child
            do while pValues.valid
                '   Create new field value item
                pxml.value = "<oal/>"
                pxml.attr ("do") = "create"
                pxml.item_new         "fieldvalue"
                pxml.item_set_current "fieldvalue"
                pxml.item_new "encoding",   pValues.item_child_value ("encoding")
                pxml.item_new "shownvalue", pValues.item_child_value ("shownvalue")
                pxml.item_new "order",      pValues.item_child_value ("order")
                pxml.item_new "fieldid",    fld_field_id
                reply = oa_do_fieldvalue (pxml.value)
                pValues.item_next
            loop
            pFields.pop_position
            pFields.item_next
        loop
        cur_message = "New form created by copying from " & Session ("form_inherit_name") & " level"
    end if
end sub

function create_form_object
    ' Create new form item
    pForm.item_root
    pForm.attr ("do") = "create"
    pForm.item_first_child
    pForm.item_set_current "parentname"
    pForm.item = Session ("form_parent_name")
    pForm.item_parent
    pForm.item_set_current "parentid"
    pForm.item = Session ("form_parent_id")
    reply = oa_do_form (pForm.value)
    pForm.value = reply
    pForm.item_first_child
    form_revised = pForm.attr ("revised")
    create_form_object = CLng (pForm.item_child_value ("id", 0))
end function
%>
