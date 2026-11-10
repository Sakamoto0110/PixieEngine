[CmdletBinding()]
param([switch]$IncludeUntracked, [switch]$Workspace)
$config = if ($Workspace) { 'scripts/checkdocs.workspace.json' } else { 'scripts/checkdocs.json' }
& (Join-Path $PSScriptRoot 'scripts/checkdocs.ps1') -RepositoryPath $PSScriptRoot -ConfigPath (Join-Path $PSScriptRoot $config) -IncludeUntracked:$IncludeUntracked
exit $LASTEXITCODE
