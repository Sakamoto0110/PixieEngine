[CmdletBinding()]
param([switch]$IncludeUntracked)
& (Join-Path $PSScriptRoot 'scripts/checkdocs.ps1') -RepositoryPath $PSScriptRoot -ConfigPath (Join-Path $PSScriptRoot 'scripts/checkdocs.json') -IncludeUntracked:$IncludeUntracked
exit $LASTEXITCODE
