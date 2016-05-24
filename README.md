# The iMatix Application Framework (iAF)

This repository contains the [src](src/) and [original released 
archives](pub/) for the iMatix Application Framework, developed
by iMatix from 1999-2006.  The source is as released on 2006-03-24,
the only known public release (taken from `download.imatix.com`).

The code was [released by iMatix under the 
MPLv2](https://github.com/imatix-legacy/license) on 2016-04-29.

From the files included, and the iMatix development at the time,
it is likely that the source was only ever built on a 10+ year old
Microsoft Windows platform, and thus it may not be completely
portable.  Study of the source, for useful ideas, is possibly more
useful than attempting to build or use iAF directly now.

The iMatix Application Framework was a three tier (presentation,
object, and database) framework -- roughly analagous to the
[Model-View-Controller](https://en.wikipedia.org/wiki/Model%E2%80%93view%E2%80%93controller)
pattern, that made extensive use of [Domain Specific
Languages](https://en.wikipedia.org/wiki/Domain-specific_language) (DSLs)
and [code generation](https://en.wikipedia.org/wiki/Automatic_programming)
in order to allow the developer to work at a high level, but quickly
produce efficient code.  Because of the time when these were
developed, and the use of the iMatix
[GSL](https://github.com/imatix-legacy/gslgen) language (which
worked natively with -- simple! -- [XML
documents](https://en.wikipedia.org/wiki/XML)), these domain specific
languages were represented in XML.  (A more modern implementation
would probably have used [YAML](https://en.wikipedia.org/wiki/YAML) or
perhaps [JSON](https://en.wikipedia.org/wiki/JSON).)

A small example can be found in [the example directory](example/skeleton/).

More information can be found in the original whitepapers about the
iMatix Application Framework can be found on the [legacy iMatix
website](http://imatix-legacy.github.io/):

*   [The iMatix Application Framework](http://imatix-legacy.github.io/twp/iaf.pdf)

*   [The iAF Presentation Framework Language](http://imatix-legacy.github.io/twp/iafpfl.pdf) (PFL)

*   [The iAF Object Framework Language](http://imatix-legacy.github.io/twp/iafofl.pdf) (OFL)

*   [The iAF Database Framework Language](http://imatix-legacy.github.io/twp/iafdfl.pdf) (DFL)

*   [The iAF Object Access Language](http://imatix-legacy.github.io/twp/iafoal.pdf) (OAL)
