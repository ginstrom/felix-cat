Felix CAT System
================

Felix is a computer assisted translation (CAT) tool for Windows. It is also called a "translation memory" system.

This package consists of the following tools:

- Felix (main program)
- Word Assist (MS Word add-in for Felix)
- Excel Assist (MS Excel add-in for Felix)
- PowerPoint Assist (MS PowerPoint add-in for Felix)

Felix was originally distributed at felix-cat.com, which is no longer online.

Building
--------

Felix and its add-ins build with **Visual Studio 2026** (toolset v145). Felix
itself is 32-bit; the Office add-ins build as 64-bit (for 64-bit Office) or
32-bit. See `BUILDING.md <BUILDING.md>`_ for the full setup: libraries,
configurations, running Felix, registering it and the add-ins with Office, and
the unit tests.

Building Felix requires the following separate libraries:

- Windows Template Library (WTL) 10: https://sourceforge.net/projects/wtl/
- Boost (Test and Filesystem compiled): https://www.boost.org/
- Microsoft Office, to build the add-ins (they import its type libraries)

Felix also uses these, which are included in the repository:

- cpptempl (``third_party/cpptempl``), its HTML template engine
- SciLexer.dll, from https://www.scintilla.org/

The ``python_tools`` folder holds Python 2 tools from the original
distribution; they are not part of the C++ build.

History
-------

Felix was developed with Visual Studio 2013 and kept in Mercurial. This
repository starts from a snapshot of that code (see ``.hg_archival.txt``) and
continues with its migration to Visual Studio 2026 and 64-bit Office.

License
-------

MIT; see ``LICENSE.txt`` (Japanese: ``LICENSE_JP.txt``). Third-party code keeps
its own license (for example ``third_party/cpptempl/LICENSE.txt``).