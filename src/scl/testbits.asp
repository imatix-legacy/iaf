<%@ LANGUAGE="VBSCRIPT" %>
<%
   Response.Expires = 0
   Response.Buffer  = FALSE
%>
<HTML>
<HEAD>
<TITLE>Test Bits Library (SCL)</TITLE>
</HEAD>
<BODY>
<h1>Test Bits Library (SCL)</h1>
<table border="2">
<tr><th><b>Value in bitstring 1</b></th><th><b>Value in bitstring 2</b></th></tr>
<tr><td align=right>5</td><td align=right>6</td></tr>
<tr><td align=right>123</td><td align=right>123</td></tr>
<tr><td align=right>568</td><td align=right>5689</td></tr>
<tr><td align=right>45312</td><td align=right>45312</td></tr>
<tr><td align=right>102789</td><td align=right>102789</td></tr>
<tr><td align=right>452123</td><td align=right>185548</td></tr>
<tr><td align=right>756321</td><td align=right>452123</td></tr>
<tr><td align=right>1256208</td><td align=right>1256208</td></tr>
<tr><td align=right>1285548</td><td align=right>7568321</td></tr>
<tr><td align=right>9853415</td><td align=right>9853415</td></tr>
</table>
<%
sub set_bits1
    bits1.create
    bits1.set 5
    bits1.set 123
    bits1.set 568
    bits1.set 45312
    bits1.set 102789
    bits1.set 452123
    bits1.set 756321
    bits1.set 1256208
    bits1.set 1285548
    bits1.set 9853415
end sub

sub set_bits2
    bits2.create
    bits2.set 6
    bits2.set 123
    bits2.set 5689
    bits2.set 45312
    bits2.set 102789
    bits2.set 185548
    bits2.set 452123
    bits2.set 1256208
    bits2.set 7568321
    bits2.set 9853415
end sub

sub display_value (title)
 Response.Write "<table border=""2""><tr><th><b>" & title & "(" & _
                bits1.count & ")</b></th></tr>"
 value = 0
 Do While (value >= 0)
     value = bits1.search_set
     if value > 0 then
         Response.Write "<tr><td align=right>" & value & "</td></tr>"
     end if
 Loop
 Response.write "</table>"
end sub

sub display_reverse_value (title)
 Response.Write "<table border=""2""><tr><th><b>" & title & " in reverse (" & _
                bits1.count & ")</b></th></tr>"
 
 value = 16383999
 Do While (value >= 0)
     value = bits1.search_set (TRUE, value)
     if value > 0 then
         Response.Write "<tr><td align=right>" & value & "</td></tr>"
     end if
 Loop
 Response.write "</table>"
end sub
 
function bits2session (obit)
    dim search_items
    redim search_items (obit.count)
    value = 0
    index = 1
    do while (value >= 0)
        value = obit.search_set (0, value)
        if value > 0 then
            response.write "<br>have found this value " & value
            search_items (index) = value
            index = index + 1
        end if
    loop
    session ("search_count") = obit.count
    session ("search_items") = search_items
end function

 Set bits1 = Server.CreateObject ("scl.bits")
 Set bits2 = Server.CreateObject ("scl.bits")
 set_bits1
 set_bits2

 bits2_value = bits2.save

 bits1.and CStr (bits2_value)
 display_value "Result of AND"
 display_reverse_value "Result of AND"

 set_bits1
 bits1.or CStr (bits2_value)
 display_value "Result of OR"

 set_bits1
 bits1.xor CStr (bits2_value)
 display_value "Result of XOR"

 Set bits1 = Nothing
 Set bits2 = Nothing

 Response.Write "<hr>Test for empty bitstring"
 Set bits1 = Server.CreateObject ("scl.bits")
 bits1.create
 Response.Write "<br>Count of empty = " & bits1.count
 Response.Write "<br>Value = " & bits1.save
 Response.Write "<hr>With one value (45312)"
 bits1.set 45312
 value = 0
 Do While (value >= 0)
     value = bits1.search_set
     if value > 0 then
         Response.Write "<br>Id is: " & value
     end if
 Loop
' Response.Write "<br>Value = " & bits1.save

 bits2session (bits1)


 Set bits1 = Nothing

%>
</BODY>
</HTML>