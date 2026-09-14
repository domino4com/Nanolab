param([string]$Port = $env:PORTNO)
& (Join-Path $PSScriptRoot 'nanolab.ps1') -Action upload -Port $Port
exit $LASTEXITCODE
