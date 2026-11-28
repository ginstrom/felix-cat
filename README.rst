Felix CAT System
================

Felix is a computer assisted translation (CAT) tool for Windows. It is also called a "translation memory" system.

This package consists of the following tools:

- Felix (main program)
- Word Assist (MS Word add-in for Felix)
- Excel Assist (MS Excel add-in for Felix)
- PowerPoint Assist (MS PowerPoint add-in for Felix)

Felix was originally distributed at felix-cat.com, which is no longer online.

Installing Felix
----------------

Download ``Felix_Setup_<version>.exe`` from the
`latest release <https://github.com/ginstrom/felix-cat/releases/latest>`_ and
run it. You need:

- 64-bit Windows 10 or 11, and administrator rights to install
- 64-bit Microsoft Office (Microsoft 365, Office 2021 or 2024) for the Word,
  Excel and PowerPoint add-ins; this is the default edition of current Office

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

Why Windows warns you
~~~~~~~~~~~~~~~~~~~~~

Felix's releases aren't code-signed yet. Code signing lets Windows show who
published a program; without it, Windows treats the installer as coming from an
unknown publisher. We plan to sign releases (see `Code signing status`_ below).

Each release is built by GitHub Actions from a tagged commit, and comes with a
checksum and a build provenance attestation that you can check (see `Verifying
the download`_).

SmartScreen
~~~~~~~~~~~

.. TODO: screenshot of the "Windows protected your PC" dialog (docs/images/smartscreen.png)

When you run the installer, Windows may show "Windows protected your PC" and
say that Microsoft Defender SmartScreen prevented an unrecognized app from
starting. To continue:

1. Choose **More info**.
2. Check that the app is ``Felix_Setup_<version>.exe`` (Publisher: Unknown
   publisher).
3. Choose **Run anyway**.

Alternatively, before running the installer, right-click the downloaded file
and choose **Properties**. Under *Security*, tick **Unblock**, choose **OK**,
then run it. The checkbox appears only on files downloaded from the internet.

An organization's policy can remove **Run anyway**.

Verifying the download
~~~~~~~~~~~~~~~~~~~~~~

To check that your download is the file GitHub Actions built from the release's
tag, run these in PowerShell in your Downloads folder (the examples use
version 3.0.1):

::

    Get-FileHash .\Felix_Setup_3.0.1.exe -Algorithm SHA256

The hash must match the line for the installer in ``SHA256SUMS.txt`` on the
release page. This shows that the file isn't corrupted or swapped, provided
``SHA256SUMS.txt`` came from the release page.

With the `GitHub CLI <https://cli.github.com/>`_, signed in (run
``gh auth login`` first):

::

    gh attestation verify .\Felix_Setup_3.0.1.exe --repo ginstrom/felix-cat

This shows that GitHub Actions built the file from this repository. For a
stricter check, add
``--signer-workflow ginstrom/felix-cat/.github/workflows/release.yml``.

Smart App Control (Windows 11)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

When Smart App Control is **On**, Windows blocks apps it can't vouch for,
including unsigned installers. There is no **Run anyway** and no per-app
exception.

To tell whether it's on, open **Windows Security > App & browser control >
Smart App Control settings**; it shows **On**, **Evaluation** or **Off**.
Evaluation doesn't block anything. You can reach the same page through
Settings > Privacy & security > Windows Security > App & browser control.
If Smart App Control blocked Felix, its notification and **Virus & threat
protection > Protection history** say so.

A signed release is expected to help, because Smart App Control lets software
run when it is signed with a certificate from a certificate authority in
Microsoft's Trusted Root Program. It checks the other program files too, so
this depends on Felix's other files (such as ``SciLexer.dll``) being accepted as
well; see `Code signing status`_.

You can turn Smart App Control off, but that lowers protection for the whole PC,
so we don't recommend doing it just for Felix. Recent Windows updates let you
turn it back on without reinstalling Windows.

Office add-ins
~~~~~~~~~~~~~~

Felix's add-ins are unsigned COM add-ins, and Office loads them by default.

If your organization turns on **File > Options > Trust Center > Trust Center
Settings > Add-ins > Require Application Add-ins to be signed by Trusted
Publisher**, Office won't load them, and the message bar says they were
disabled. This usually comes from organization policy, so the option may be
greyed out. Even after releases are signed, your organization must also trust
the publisher.

To re-enable an add-in that Office disabled (for example after a crash):

1. Choose **File > Options > Add-ins**.
2. Next to **Manage**, choose **Disabled Items**, then **Go**.
3. Select the Felix add-in and choose **Enable**.
4. Next to **Manage**, choose **COM Add-ins**, then **Go**.
5. Tick **Felix Addin** (its description reads "Felix interface for Word",
   "Excel" or "PowerPoint", depending on the application) and choose **OK**.

See the manual's
`Troubleshooting <https://ginstrom.github.io/felix-cat/manual/en/troubleshooting.html#word>`_
page if the Felix toolbar still doesn't appear.

Code signing status
~~~~~~~~~~~~~~~~~~~

Felix's releases are not code-signed yet. We intend to apply to the
`SignPath Foundation <https://signpath.org>`_'s free code signing program for
open source projects. Signed releases are expected to show a verified publisher
and to get past Smart App Control. One caveat: ``SciLexer.dll``, the editor
component Felix ships, is an old unsigned third-party build that may still need
attention before signing (tracked in
`ginstrom/felix-cat#44 <https://github.com/ginstrom/felix-cat/issues/44>`_).

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
installer on every push (``.github/workflows/build.yml``) and builds draft
releases from version tags (``release.yml``), with SHA-256 checksums and build
provenance attestations.

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