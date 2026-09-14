# Windows PowerShell 5.1+; only Arduino CLI is needed in addition to Windows.
param(
    [ValidateSet('compile', 'upload', 'ports')][string]$Action = 'compile',
    [string]$Port = $env:PORTNO
)
$ErrorActionPreference = 'Stop'
try {
    Set-Location -LiteralPath $PSScriptRoot
    $cli = $env:ARDUINO_CLI
    if (-not $cli) {
        $candidates = @(
            "$env:LOCALAPPDATA\Programs\Arduino IDE\resources\app\lib\backend\resources\arduino-cli.exe",
            "$env:ProgramFiles\Arduino IDE\resources\app\lib\backend\resources\arduino-cli.exe"
        )
        foreach ($candidate in $candidates) {
            if (Test-Path -LiteralPath $candidate) { $cli = $candidate; break }
        }
        if (-not $cli) {
            $command = Get-Command arduino-cli -ErrorAction SilentlyContinue
            if ($command) { $cli = $command.Source }
        }
    }
    if (-not $cli) { throw 'Install Arduino IDE 2.3.10, or set ARDUINO_CLI to Arduino CLI 1.5.1.' }
    $version = & $cli version
    if ($LASTEXITCODE -ne 0 -or "$version" -notmatch 'Version: 1\.5\.1\s') {
        throw "Required Arduino CLI 1.5.1; found: $version"
    }
    Write-Host $version
    if ($Action -eq 'ports') { & $cli board list; exit $LASTEXITCODE }
    $name = Split-Path -Leaf $PSScriptRoot
    if (-not (Test-Path -LiteralPath 'sketch.yaml') -or -not (Test-Path -LiteralPath "$name.ino")) {
        throw 'Keep sketch.yaml and the matching FolderName.ino in this sketch folder.'
    }
    if ($Action -eq 'upload' -and -not $Port) {
        $raw = & $cli board list --json
        if ($LASTEXITCODE -ne 0) { throw 'Serial discovery failed.' }
        $devices = ($raw -join "`n" | ConvertFrom-Json).detected_ports
        $ports = @($devices | Where-Object {
            $_.port.protocol -eq 'serial' -and $_.port.properties.vid -and $_.port.properties.pid
        } | ForEach-Object { $_.port.address })
        if ($ports.Count -ne 1) {
            & $cli board list
            throw 'Connect exactly one USB serial board, or use .\upload.ps1 -Port COM7 (or set $env:PORTNO).'
        }
        $Port = $ports[0]
    }
    & $cli compile --profile nanolab --clean --build-path (Join-Path $PSScriptRoot 'build') .
    if ($LASTEXITCODE -ne 0) { throw 'Compilation failed; no firmware uploaded.' }
    if ($Action -eq 'upload') {
        Write-Host "Uploading to $Port at 1000000 baud"
        & $cli upload --profile nanolab --port $Port --input-dir (Join-Path $PSScriptRoot 'build') --upload-property upload.speed=1000000 .
        if ($LASTEXITCODE -ne 0) { throw 'Upload failed.' }
    }
    exit 0
} catch {
    Write-Error $_ -ErrorAction Continue
    exit 1
}
