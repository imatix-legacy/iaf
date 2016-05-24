    -idle

After-Init:
    (--) Ok                                 -> Get-Token
          + Open-Markup-File
          + Initialise-Tag-Tree
          + Get-Content-Token
    (--) Error                              ->
          + Signal-Error
          + Terminate-The-Program

Get-Token:
    (--) Open-Tag                           -> Go-To-End-Of-Tag
          + Stack-Tag
          + Get-Tag-Value
          + Get-Tag-Attribute
    (--) End-Tag                            -> Go-To-End-Of-End-Tag
          + Stack-Tag
          + Get-Tag-Value
          + Get-Tag-Attribute
    (--) Data                               -> Get-Token
          + Goto-End-Of-Data
          + Create-New-Data
          + Get-Content-Token
    (--) Unknow-Data                        -> Get-Token
          + Goto-End-Of-File
          + Create-New-Tag
          + Get-Content-Token
    (--) Load-Buffer                        -> Get-Token
          + Load-Buffer
          + Get-Content-Token
    (--) End-Of-File                        ->
          + Close-Markup-File
          + Terminate-The-Program

Go-To-End-Of-Tag:
    (--) Unknow-Tag                         -> Process-Unknow-Tag
          + Search-For-Scripting-Tag
    (--) Normal                             -> Go-To-End-Of-Tag
          + Go-To-End-Of-Tag
    (--) Open-Tag                           -> Go-To-End-Of-Tag
          + Stack-Tag
          + Get-Tag-Value
          + Get-Tag-Attribute
    (--) Invalid-Tag                        -> Go-To-End-Of-Tag
          + Unstack-Tag
          + Return-Before-Tag
    (--) Script                             -> Script-End-Tag
          + Go-To-End-Of-Script-Tag
          + Check-If-Need-End-Tag
    (--) End-Of-Tag                         -> Go-To-End-Of-Tag
          + Unstack-Tag
    (--) End                                -> Get-Token
          + Remove-Crlf
          + Create-New-Tag
          + Check-If-Need-End-Of-Tag
          + Get-Content-Token
    (--) Script-Data                        -> Go-To-End-Of-Tag
          + Unstack-Tag
          + Create-New-Tag
          + Go-To-End-Of-Script-Data
          + Restore-Stack
          + Create-Script-Data
          + Go-To-End-Of-Script-Tag

Go-To-End-Of-End-Tag:
    (--) Unknow-Tag                         -> Go-To-End-Of-End-Tag
          + Go-To-End-Of-Tag
    (--) Normal                             -> Go-To-End-Of-End-Tag
          + Go-To-End-Of-Tag
    (--) Open-Tag                           -> Go-To-End-Of-End-Tag
          + Stack-Tag
          + Get-Tag-Value
          + Get-Tag-Attribute
    (--) Script                             -> Go-To-End-Of-End-Tag
          + Go-To-End-Of-Tag
    (--) End-Of-Tag                         -> Go-To-End-Of-End-Tag
          + Unstack-Tag
    (--) End                                -> Get-Token
          + Remove-Crlf
          + Create-New-Tag
          + Move-To-Parent-Tag
          + Get-Content-Token

Script-End-Tag:
    (--) No                                 -> Go-To-End-Of-Tag
          + Unstack-Tag
    (--) Yes                                -> Get-Token
          + Goto-Script-End-Tag
          + Create-New-Tag
          + Get-Content-Token
    (--) Script-Data                        -> Get-Token
          + Create-New-Tag
          + Go-To-End-Of-Script-Data
          + Create-Script-Data
          + Goto-Script-End-Tag
          + Create-New-Tag
          + Get-Content-Token

Process-Unknow-Tag:
    (--) Found                              -> Go-To-End-Of-Tag
          + Get-Tag-Attribute
    (--) Not-Found                          -> Get-Token
          + Go-To-End-Of-Tag
          + Remove-Crlf
          + Create-New-Tag
          + Get-Content-Token

Defaults:
    (--) Error                              ->
          + Signal-Error
          + Terminate-The-Program
    (--) End-Of-File                        ->
          + Close-Markup-File
          + Terminate-The-Program
    (--) Last-Tag                           ->
          + Create-New-Tag
          + Close-Markup-File
          + Terminate-The-Program
