% \iffalse meta-comment
%
% Copyright 1993 1994 1995 1996 1997 1998 1999
% The LaTeX3 Project and any individual authors listed elsewhere
% in this file. 
% 
% This file is part of the Standard LaTeX `Tools Bundle'.
% -------------------------------------------------------
% 
% This file may be distributed under the terms of the LaTeX Project
% Public License, as described in lppl.txt in the base LaTeX distribution.
% Either version 1.0 or, at your option, any later version.
% 
% \fi

README for the `tools' bundle  (December 1998)
=============================

This `bundle' consists of LaTeX2e packages written and supported by
members of the LaTeX3 Project Team.

The documented source code of each package is in a file with extension
`.dtx'.  Running LaTeX on the file tools.ins will produce all the
package files, and some associated files.

So you should first process tools.ins:

  latex tools.ins

The files with extensions `.sty' and `.tex' (including a file whose
name is just `.tex') should then be moved to a directory on LaTeX's
standard input path.

See the Note at the end of this file if you have problems processing
the tools.ins file.

Documentation for the individual packages may then be obtained by
running LaTeX on the `.dtx' files.

For example:

  latex array.dtx

will produce the file array.dvi, documenting the array package.


The file manifest.txt contains a list of the main files in the
distribution together with a one-or-two line summary of each package.


Copyright
=========
Copyright is maintained on each of these packages by the author(s)
of the package. 


Distribution Conditions
=======================
All the files in this bundle are distributed under the terms of
the LaTeX Project Public License (LPPL) as found in the base
LaTeX distribution,  CTAN:macros/latex/base/lppl.txt;
either version 1 of the license, or (at your option) any later
version.

Commercial users of the multicol package are asked to read the
notice at the head of the file multicol.dtx.


Reporting Bugs
==============

If you wish to report a problem or bug in any of these packages, use
the latexbug.tex program that comes with the standard LaTeX
distribution.  Please ensure that you enter `tools' category when
prompted with a menu of categories, so that the message will be
automatically forwarded to the appropriate part of our database.

When reporting bugs, please produce a small test file that shows the
problem, and ensure that you are using the current version of the
package, and of the base LaTeX software.


NOTE (docstrip version)
=======================

If   latex tools.ins
produces the `docstrip interactive mode' prompt:

  * First type the extension of your input file(s):  *
  \infileext=

Then your version of docstrip is too old.
Quit (eg by hitting `enter' to all questions) and get a newer
docstrip.tex. It must be at least version 2.4.

A suitable docstrip.tex may be found from `CTAN' archives such as
ftp.dante.de   tex-archive/macros/latex/unpacked/docstrip.tex

Docstrip is part of the base LaTeX distribution, so if you have
an old docstrip then your LaTeX is out of date and you may consider
getting the whole of that directory and re-installing LaTeX.
However you need to fetch only the file docstrip.tex to unpack
this tools distribution with your existing format.

