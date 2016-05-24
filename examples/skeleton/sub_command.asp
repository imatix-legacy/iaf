<%
'-------------------------------------------------------------------------
'   sub_command.asp - Run command with 'Please wait' message
'
sub run_command (command, args)
    Session ("cmd_text") = command
    Session ("cmd_args") = args

    %><script language="JavaScript" type="text/javascript">
    <!--
    popup = window.open ("command.asp", "Command", "menubar=0,scrollbars=1,resizable=1,width=700,height=500,left=50,top=50")
    //-->
    </script><%
end sub
%>

