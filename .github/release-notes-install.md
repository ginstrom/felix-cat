## Installing and verifying

<!-- unsigned -->
This release is **not code-signed** yet, so Windows SmartScreen will say
"Windows protected your PC". Choose **More info**, then **Run anyway**. On PCs
with Smart App Control turned on, Windows blocks unsigned installers
completely. See [Installing Felix](https://github.com/ginstrom/felix-cat#installing-felix).
<!-- /unsigned -->

To check that your download is the file GitHub Actions built from this
release's tag, in PowerShell in your Downloads folder:

    Get-FileHash .\Felix_Setup_{{VERSION}}.exe -Algorithm SHA256

The hash must match the line for the installer in `SHA256SUMS.txt`. With the
[GitHub CLI](https://cli.github.com/), signed in:

    gh attestation verify .\Felix_Setup_{{VERSION}}.exe --repo ginstrom/felix-cat
