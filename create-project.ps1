param([Parameter(Mandatory=$true)][string]$Name, [string]$Parent = (Get-Location).Path)
$ErrorActionPreference = 'Stop'
try {
    if ($Name -notmatch '^[A-Za-z][A-Za-z0-9_]*$') { throw 'Use letters, digits and underscores, beginning with a letter.' }
    $target = Join-Path $Parent $Name
    if (Test-Path -LiteralPath $target) { throw "Already exists; nothing changed: $target" }
    $library = Join-Path $target 'libraries/CanStatusClient'
    New-Item -ItemType Directory -Path $library -Force | Out-Null
    Copy-Item -LiteralPath (Join-Path $PSScriptRoot 'src') -Destination $library -Recurse
    foreach ($file in @('library.properties', 'LICENSE')) {
        Copy-Item -LiteralPath (Join-Path $PSScriptRoot $file) -Destination $library
    }
    Copy-Item -LiteralPath (Join-Path $PSScriptRoot 'templates/Project.ino') -Destination (Join-Path $target "$Name.ino")
    Copy-Item -LiteralPath (Join-Path $PSScriptRoot 'templates/PROJECT.md') -Destination (Join-Path $target 'README.md')
    Get-ChildItem -LiteralPath (Join-Path $PSScriptRoot 'examples/TestClient') -File |
        Where-Object { $_.Extension -in @('.command', '.ps1') } |
        Copy-Item -Destination $target
    $yaml = Get-Content -LiteralPath (Join-Path $PSScriptRoot 'examples/TestClient/sketch.yaml') -Raw
    $yaml = $yaml.Replace('dir: ../..', 'dir: libraries/CanStatusClient').Replace(
        '# This example uses the library at the root of the Nanolab repository.',
        '# This exact local library source is included in the submission.')
    [IO.File]::WriteAllText((Join-Path $target 'sketch.yaml'), $yaml)
    [IO.File]::WriteAllText((Join-Path $target '.gitignore'), "build/`n.DS_Store`n")
    $version = (Get-Content -LiteralPath (Join-Path $PSScriptRoot 'library.properties') | Where-Object { $_ -like 'version=*' }) -replace '^version=', ''
    [IO.File]::WriteAllText((Join-Path $library 'ORIGIN.txt'), "https://github.com/domino4com/Nanolab`nBundled library version: $version`n")
    Write-Host "Created $target. Open $Name.ino, merge your existing code, then run compile.ps1."
} catch { Write-Error $_ -ErrorAction Continue; exit 1 }
