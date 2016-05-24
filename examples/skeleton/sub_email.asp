<%
'-------------------------------------------------------------------------
'   sub_email.asp - E-mail routine using SCL mail component
'
'   Written: 2000/04/02   iAF Development Team <iaf@imatix.com>
'   Revised: 2000/10/01   iAF Development Team <iaf@imatix.com>
'
'   Copyright:  Copyright (c) 1999-2000 iMatix Corporation
'   License:    This is free software; you can redistribute it and/or modify
'               it under the terms of the iAF License Agreement as provided
'               in the file LICENSE.TXT.  This software is distributed in
'               the hope that it will be useful, but without any warranty.
'
%>
<!--#include file="emaillog.asp"-->
<!--#include file="emailqueue.asp"-->
<!--#include file="emaildef.asp"-->
<%

function send_email_queued (context, recipient, sendat, orderid)
    '   Get email context
    fld_emaildef_context = context
    pxml.value = "<oal/>"
    pxml.attr ("view") = "detail"
    pxml.item_new         "emaildef"
    pxml.item_set_current "emaildef"
    pxml.item_new "context", fld_emaildef_context
    oal_emaildef "fetch"
    if exception_raised then
        send_email_queued = 1
        exit function
    end if
    pxml.item_first_child
    fld_emaildef_context = pxml.item_child_value ("context", context)
    fld_emaildef_name    = pxml.item_child_value ("name", context)
    fld_emaildef_subject = pxml.item_child_value ("subject", "(no subject)")
    fld_emaildef_body    = pxml.item_child_value ("body", "(no body)")

    '   Create entry in email log
    pxml.value = "<oal/>"
    pxml.item_new         "emaillog"
    pxml.item_set_current "emaillog"
    pxml.item_new "context",     context
    pxml.item_new "sender",      cur_userid
    pxml.item_new "recipients",  recipient
    pxml.item_new "tolist",      recipient
    pxml.item_new "subject",     symbols.substitute (fld_emaildef_subject)
    pxml.item_new "body",        symbols.substitute (fld_emaildef_body) & symbols.substitute (Session ("EMAILFOOTER"))
    pxml.item_new "status",      PENDING_QUEUED
    pxml.item_new "message",     ""
    pxml.item_new "sentat",      0
    oal_emaillog "create"
    pxml.item_first_child
    fld_emaillog_id = CLng (pxml.item_child_value ("id", 0))
    
    '   Finally, create entry in email queue
    if sendat = 0 then sendat = pDATE.Timestamp
    pxml.value = "<oal/>"
    pxml.item_new         "emailqueue"
    pxml.item_set_current "emailqueue"
    pxml.item_new "sendat",      sendat
    pxml.item_new "emaillogid",  fld_emaillog_id
    oal_emailqueue "create"
end function

sub oal_emaillog (operation)
    pxml.item_root
    pxml.attr ("do")   = operation
    pxml.attr ("user") = cur_userid
    reply = oa_do_emaillog (pxml.value)
    pxml.value = reply
    if pxml.attr ("done") = "ok" then
        exception_raised = FALSE
    else
        cur_message = "Error from emaillog: " & pxml.attr ("cause") & " " & pxml.attr ("message")
        exception_raised = TRUE
    end if
end sub

sub oal_emailqueue (operation)
    pxml.item_root
    pxml.attr ("do")   = operation
    pxml.attr ("user") = cur_userid
    reply = oa_do_emailqueue (pxml.value)
    pxml.value = reply
    if pxml.attr ("done") = "ok" then
        exception_raised = FALSE
    else
        cur_message = "Error from emailqueue: " & pxml.attr ("cause") & " " & pxml.attr ("message")
        exception_raised = TRUE
    end if
end sub

sub oal_emaildef (operation)
    pxml.item_root
    pxml.attr ("do")   = operation
    pxml.attr ("user") = cur_userid
    reply = oa_do_emaildef (pxml.value)
    pxml.value = reply
    if pxml.attr ("done") = "ok" then
        exception_raised = FALSE
    else
        cur_message = "Error from emaildef: " & pxml.attr ("cause") & " " & pxml.attr ("message")
        exception_raised = TRUE
    end if
end sub

%>
