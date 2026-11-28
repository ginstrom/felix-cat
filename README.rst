Felix CAT System
================

Felix is a computer assisted translation (CAT) tool for Windows. It is also called a "translation memory" system.

This package consists of the following tools:

- Felix (main program)
- Word Assist (MS Word add-in for Felix)
- Excel Assist (MS Excel add-in for Felix)
- PowerPoint Assist (MS PowerPoint add-in for Felix)

Felix was originally distributed at felix-cat.com, which is no longer online.

Installing
----------

Download ``Felix_Setup_<version>.exe`` from the
`latest release <https://github.com/ginstrom/felix-cat/releases/latest>`_ and
run it. You need:

- 64-bit Windows 10 or 11, and administrator rights to install
- 64-bit Microsoft Office (Microsoft 365, Office 2021 or 2024) for the Word,
  Excel and PowerPoint add-ins; this is the default edition of current Office

Releases up to 3.0.1 aren't code-signed, so Windows SmartScreen may warn
that they come from an unknown publisher: choose **More info**, then **Run
anyway**. Later releases are signed (see "Code signing" below).

It installs Felix to ``C:\Program Files\Felix``, the Office add-ins (each
optional) and the Visual C++ runtimes they need. In Word, Excel and
PowerPoint, Felix's toolbar appears on the ribbon's **Add-ins** tab.

The user manual is online in
`English <https://ginstrom.github.io/felix-cat/manual/en/>`_ and
`Japanese <https://ginstrom.github.io/felix-cat/manual/jp/>`_ (Felix's Help menu
opens it too).

**Upgrading from Felix 2.0:** uninstall 2.0 first, so Office doesn't keep
starting the old version. Back up ``%LOCALAPPDATA%\Felix`` beforehand if you
customized settings or templates, because the 2.0 uninstaller deletes that
folder. Your memory and glossary files aren't affected.

Building
--------

Felix and its add-ins build with **Visual Studio 2026** (toolset v145). Felix
itself is 32-bit; the Office add-ins build as 64-bit (for 64-bit Office) or
32-bit. See `BUILDING.md <BUILDING.md>`_ for the full setup: libraries,
configurations, running Felix, registering it and the add-ins with Office, and
the unit tests.

Building Felix requires the following separate libraries:

- Windows Template Library (WTL) 10: https://sourceforge.net/projects/wtl/
- Boost (Test, Filesystem and Regex compiled): https://www.boost.org/

Microsoft Office is not needed: the Office type library headers are in
``common/office_typelibs``. GitHub Actions builds Felix, the add-ins and the
installer on every push (``.github/workflows/build.yml``) and builds signed
releases from version tags (``release.yml``).

Felix also uses these, which are included in the repository:

- cpptempl (``third_party/cpptempl``), its HTML template engine
- SciLexer.dll, from https://www.scintilla.org/

The ``python_tools`` folder holds Python 2 tools from the original
distribution; they are not part of the C++ build.

Code signing
------------

Free code signing provided by `SignPath.io <https://signpath.io>`_, certificate
by `SignPath Foundation <https://signpath.org>`_.

Releases are built from this repository by GitHub Actions and signed through
the SignPath Foundation, so Windows shows **SignPath Foundation** as the
publisher. See the `code signing policy <CODE_SIGNING_POLICY.md>`_ for what is
signed, who approves it, and Felix's privacy statement.

History
-------

Felix was developed with Visual Studio 2013 and kept in Mercurial. This
repository starts from a snapshot of that code (see ``.hg_archival.txt``) and
continues with its migration to Visual Studio 2026 and 64-bit Office.

License
-------

MIT; see ``LICENSE.txt`` (Japanese: ``LICENSE_JP.txt``). Third-party code keeps
its own license (for example ``third_party/cpptempl/LICENSE.txt``).