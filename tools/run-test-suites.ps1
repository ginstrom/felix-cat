param([string]$Exe = "$PSScriptRoot\..\Felix\Testing\Felix.exe",
      [string]$OutDir = "$PSScriptRoot\..\Felix\Testing\test-results",
      [int]$TimeoutSec = 120,
      [string[]]$Only)

# Runs every top-level Boost.Test suite in its own Felix.exe process, so that a
# crash (access violation, stack overflow) only spoils its own suite. After such
# a crash Boost.Test reports the remaining tests as "passed" without running them,
# so a single run of Felix.exe can't be trusted once anything crashes.
#
#   powershell -ExecutionPolicy Bypass -File tools\run-test-suites.ps1
#   powershell -ExecutionPolicy Bypass -File tools\run-test-suites.ps1 -Only TestMemory,TestFelixModel
#
# Writes one <suite>.txt report (and <suite>.debug.txt with the failure details
# Felix sends to OutputDebugString) per suite, plus summary.csv, to $OutDir.

$Exe = [IO.Path]::GetFullPath($Exe)
$OutDir = [IO.Path]::GetFullPath($OutDir)

Add-Type -TypeDefinition @"
using System; using System.Text; using System.Threading;
using System.Runtime.InteropServices;
public class DbWin2 {
  [DllImport("kernel32.dll", CharSet=CharSet.Unicode)] static extern IntPtr CreateFileMapping(IntPtr h, IntPtr a, uint p, uint hi, uint lo, string n);
  [DllImport("kernel32.dll")] static extern IntPtr MapViewOfFile(IntPtr h, uint a, uint hi, uint lo, UIntPtr n);
  [DllImport("kernel32.dll", CharSet=CharSet.Unicode)] static extern IntPtr CreateEvent(IntPtr a, bool m, bool i, string n);
  [DllImport("kernel32.dll")] static extern bool SetEvent(IntPtr h);
  [DllImport("kernel32.dll")] static extern uint WaitForSingleObject(IntPtr h, uint ms);
  static volatile bool stop; static Thread t;
  public static volatile int TargetPid;
  public static StringBuilder Text = new StringBuilder();
  public static string Take() { lock (Text) { string s = Text.ToString(); Text.Clear(); return s; } }
  public static string Start() {
    IntPtr ready = CreateEvent(IntPtr.Zero, false, false, "DBWIN_BUFFER_READY");
    IntPtr data  = CreateEvent(IntPtr.Zero, false, false, "DBWIN_DATA_READY");
    IntPtr map   = CreateFileMapping(new IntPtr(-1), IntPtr.Zero, 4, 0, 4096, "DBWIN_BUFFER");
    if (ready == IntPtr.Zero || data == IntPtr.Zero || map == IntPtr.Zero) return "setup failed";
    IntPtr view = MapViewOfFile(map, 4, 0, 0, new UIntPtr(4096));
    t = new Thread(() => {
      SetEvent(ready);
      while (!stop) {
        if (WaitForSingleObject(data, 200) == 0) {
          int pid = Marshal.ReadInt32(view);
          string s = Marshal.PtrToStringAnsi(new IntPtr(view.ToInt64() + 4));
          if (pid == TargetPid) lock (Text) Text.Append(s);
          SetEvent(ready);
        }
      }
    });
    t.IsBackground = true; t.Start(); return "ok";
  }
  public static void Stop() { stop = true; if (t != null) t.Join(1000); }
}
"@

New-Item -ItemType Directory -Force $OutDir | Out-Null
"listener: $([DbWin2]::Start())"

# Suite names: unindented lines of the content listing (trailing * = enabled).
$env:BOOST_TEST_LIST_CONTENT = 'HRF'
$p = Start-Process $Exe -WorkingDirectory (Split-Path $Exe) -PassThru -Wait `
      -RedirectStandardOutput "$OutDir\list-out.txt" -RedirectStandardError "$OutDir\list.txt"
Remove-Item Env:BOOST_TEST_LIST_CONTENT
# (The listing also picks up trace lines that Felix writes to stderr; skip those.)
$suites = Get-Content "$OutDir\list.txt" | Where-Object { $_ -match '^\w+\*?$' } | ForEach-Object { $_.TrimEnd('*') } | Select-Object -Unique
# "-File ... -Only A,B" arrives as the single string "A,B".
if ($Only) { $Only = $Only -split ','; $suites = $suites | Where-Object { $Only -contains $_ } }
"suites: $($suites.Count)"

$summary = @()
foreach ($s in $suites) {
  $env:BOOST_TEST_RUN_FILTERS = $s
  $safe = $s -replace '[^A-Za-z0-9_]', '_'
  $err = "$OutDir\$safe.txt"
  $p = Start-Process $Exe -WorkingDirectory (Split-Path $Exe) -PassThru `
        -RedirectStandardOutput "$OutDir\$safe.out.txt" -RedirectStandardError $err
  [DbWin2]::TargetPid = $p.Id
  $done = $p.WaitForExit($TimeoutSec * 1000)
  if (-not $done) { Stop-Process -Id $p.Id -Force; $p.WaitForExit(5000) | Out-Null }
  Start-Sleep -Milliseconds 300
  $dbg = [DbWin2]::Take()
  $text = if (Test-Path $err) { [IO.File]::ReadAllText($err) } else { '' }

  $ran     = ([regex]::Matches($text, '(?m)^    Test case .* has passed with:')).Count
  $bare    = ([regex]::Matches($text, '(?m)^    Test case .* has passed\s*$')).Count
  $failed  = [regex]::Matches($text, '(?m)^    Test case "([^"]+)" (has failed|was aborted)') | ForEach-Object { $_.Groups[1].Value + ' (' + $_.Groups[2].Value + ')' }
  $crash   = ([regex]::Matches($dbg, 'memory access violation|stack overflow|unknown type|Assertion failed')) | ForEach-Object { $_.Value } | Select-Object -Unique
  # A crash is suspect if later cases in the suite were reported bare.
  $summary += [pscustomobject]@{
    Suite = $s; Ran = $ran; Bare = $bare; Failed = ($failed -join '; ');
    Crash = ($crash -join ','); Timeout = (-not $done)
  }
  if ($dbg) { [IO.File]::WriteAllText("$OutDir\$safe.debug.txt", $dbg) }
}
Remove-Item Env:BOOST_TEST_RUN_FILTERS -ErrorAction SilentlyContinue
[DbWin2]::Stop()

$summary | Export-Csv "$OutDir\summary.csv" -NoTypeInformation
"total ran (with assertions): " + ($summary | Measure-Object Ran -Sum).Sum
"total bare passes:           " + ($summary | Measure-Object Bare -Sum).Sum
$bad = @($summary | Where-Object { $_.Failed -or $_.Crash -or $_.Timeout })
"suites with failures/crashes/timeouts: $($bad.Count)"
$bad | Format-Table -AutoSize -Wrap | Out-String -Width 300
if ($bad.Count) { exit 1 } else { exit 0 }
