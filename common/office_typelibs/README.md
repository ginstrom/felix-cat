# Office type library headers

Felix and the Word, Excel and PowerPoint add-ins call Office through COM,
using the C++ wrappers that the compiler's `#import` directive generates
from Office's type libraries. `#import` needs Office installed on the build
machine, and GitHub's runners don't have Office. So the generated headers
are committed here, and the code `#include`s them instead.

| Header | Replaces the `#import`s in | Type libraries (namespace) |
|--------|----------------------------|----------------------------|
| `msoffice_word.h` | `common\WordAutomationObject.h` (Felix, Word Assist) | MSO (`MSOffice`), VBE6EXT (`VBE6`), MSWORD (`MSWord`) |
| `msoffice_excel.h` | `Felix\ExcelInterfaceReal.h`, `ExcelAssist\ExcelAssist\ExcelImport.h` | MSO (`MSOffice`), VBE6EXT (`VBE6`), EXCEL (`Excel`) |
| `powerpoint.h` | `PowerPointAssist\PowerPointAssist\PowerPointImports.h` | MSO (`Office`), VBE6EXT (`VBE6`), MSPPT (`PowerPoint`), EXCEL (`Excel`) |
| `addin_designer.h` | the add-ins' and their tests' `stdafx.h` | MSADDNDR (`AddInDesignerObjects`): `IDTExtensibility2` |

The generated files are in `msoffice\`, `powerpoint\` and `addin\`. One set
serves both platforms: when MSBuild builds the x64 add-ins, `#import` reads
the same 32-bit ("Win32") type library registrations as for Win32, so these
are exactly the headers the x64 add-ins were built against before. (A 64-bit
command-line `cl` reads the win64 registrations instead; their Word and
Add-in Designer headers differ, and the add-in code doesn't compile with
them. The script uses the x86 compiler for that reason.)

Only the headers the compiler generated are here (`.tlh` declarations, `.tli`
inline wrappers). Office's own `.olb`, `.dll` and `.exe` files are not, and
must not be: Microsoft doesn't allow redistributing them.

Not covered here, because Windows itself provides them: `mshtml.tlb`
(`Felix\stdafx.h`) and VBScript regular expressions (`common\RegEx.h`). And
`Felix.tlb` is Felix's own, built by the Felix project.

## Regenerating

On a machine with Visual Studio 2026 and Office (Word, Excel and PowerPoint):

```powershell
powershell -ExecutionPolicy Bypass -File tools\regenerate-office-typelibs.ps1
```

It compiles the `#import` directives listed in the script and rewrites this
folder's `msoffice\`, `powerpoint\` and `addin\` subfolders. Then build and
test as usual and commit the changes.

The script also tidies the output: it drops the compiler version line and the
absolute paths, removes the `#pragma start_map_region` / `implementation_key`
mapping that current compilers use to tie a `.tlh` to its `.tli` by absolute
path, and makes each `.tlh` `#include` its `.tli` by relative path instead.

Change an `#import` attribute (`rename`, `rename_namespace`, `exclude`, ...)
in the script, not in the code: the attributes decide the generated names
the code uses. The headers here were generated from 64-bit Microsoft 365
(Office 16.0). Excel Assist's old `rename("CopyFile", "CopyFIleForExcel")`
typo was dropped so it can share Felix's Excel headers; nothing used that name.
