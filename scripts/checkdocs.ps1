# Requires PowerShell 7. This checker validates structure, never product acceptance.
[CmdletBinding()]
param(
    [Parameter(Mandatory)][string]$RepositoryPath,
    [string]$ConfigPath,
    [switch]$IncludeUntracked,
    [string[]]$AdditionalPath = @()
)
$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
$root = [IO.Path]::GetFullPath($RepositoryPath)
$prefix = $root.TrimEnd('/','\') + [IO.Path]::DirectorySeparatorChar
$errors = [Collections.Generic.List[string]]::new()
$ids = @{}
$anchors = @{}
$config = @{ exclude = @(); requiredDocuments = @(); metadataSchema = ''; closureDirectory = ''; closureIndex = '' }
if ($ConfigPath) {
    $loaded = Get-Content -LiteralPath $ConfigPath -Raw | ConvertFrom-Json -AsHashtable
    foreach ($key in $loaded.Keys) { $config[$key] = $loaded[$key] }
}
function Report([string]$path, [string]$message) { $errors.Add("${path}: $message") }
function FullPath([string]$relative) {
    $full = [IO.Path]::GetFullPath((Join-Path $root $relative))
    if (-not $full.StartsWith($prefix, [StringComparison]::OrdinalIgnoreCase)) { throw "Path leaves repository: $relative" }
    return $full
}
function ReadText([string]$path) { [IO.File]::ReadAllText($path, [Text.UTF8Encoding]::new($false, $true)) }
function WithoutFences([string]$value) { [regex]::Replace($value, '(?ms)^\s*(`{3,}|~{3,})[^\r\n]*\r?\n.*?^\s*\1\s*$', '') }
function Metadata([string]$value) {
    $fields = @{}
    $header = ([regex]::Split((WithoutFences $value), '(?m)^## '))[0]
    foreach ($m in [regex]::Matches($header, '(?m)^\*\*([^*]+):\*\*\s*([^\r\n]*)')) { $fields[$m.Groups[1].Value] = $m.Groups[2].Value.Trim() }
    return $fields
}
function AnchorSet([string]$path) {
    if (-not $anchors.ContainsKey($path)) {
        $set = [Collections.Generic.HashSet[string]]::new([StringComparer]::Ordinal)
        $counts = @{}
        foreach ($m in [regex]::Matches((WithoutFences (ReadText $path)), '(?m)^#{1,6}\s+(.+?)\s*#*\s*$')) {
            $heading = [regex]::Replace($m.Groups[1].Value, '\[([^\]]+)\]\([^)]+\)', '$1')
            $slug = [regex]::Replace($heading.ToLowerInvariant(), '[^\w\- ]', '').Replace(' ', '-')
            if ($counts.ContainsKey($slug)) { $counts[$slug]++; $slug += '-' + $counts[$slug] } else { $counts[$slug] = 0 }
            [void]$set.Add($slug)
        }
        foreach ($m in [regex]::Matches((ReadText $path), '<a\s+(?:id|name)=["'']([^"'']+)["'']')) { [void]$set.Add($m.Groups[1].Value) }
        $anchors[$path] = $set
    }
    return ,$anchors[$path]
}
$gitArgs = @('-C', $root, '-c', 'core.quotepath=false', 'ls-files', '--cached')
if ($IncludeUntracked) { $gitArgs += @('--others', '--exclude-standard') }
$paths = @(& git @gitArgs)
if ($LASTEXITCODE -ne 0) { throw 'RepositoryPath must be a Git working tree.' }
$paths += $AdditionalPath
$paths += $config.requiredDocuments
$files = @($paths | Sort-Object -Unique | Where-Object {
    $p = $_
    $p.EndsWith('.md') -and -not ($config.exclude | Where-Object { $p -like $_ }) -and
    $p -notmatch '(^|/)(\.git|\.local|\.claude|\.codex|node_modules|bin|obj)/' -and
    (Test-Path -LiteralPath (Join-Path $root $p) -PathType Leaf)
})
$schema = if ($config.metadataSchema) { Get-Content -LiteralPath (FullPath $config.metadataSchema) -Raw | ConvertFrom-Json -AsHashtable } else { $null }
foreach ($required in $config.requiredDocuments) {
    if (-not (Test-Path -LiteralPath (FullPath $required) -PathType Leaf)) { Report $required 'required document is missing' }
}
foreach ($relative in $files) {
    $path = FullPath $relative
    try { $text = ReadText $path } catch { Report $relative 'invalid UTF-8'; continue }
    $fields = Metadata $text
    if ($fields.ContainsKey('Document ID')) {
        $id = $fields['Document ID']
        if ($ids.ContainsKey($id)) { Report $relative "duplicate Document ID $id (also $($ids[$id]))" }
        $ids[$id] = $relative
        if ($schema) {
            foreach ($name in $schema.metadata.requiredForExtendedSchema) { if (-not $fields.ContainsKey($name)) { Report $relative "missing metadata: $name" } }
            foreach ($name in $schema.metadata.fields.Keys) {
                if ($fields.ContainsKey($name) -and $fields[$name] -cnotin $schema.metadata.fields[$name]) { Report $relative "invalid metadata: $name = $($fields[$name])" }
            }
        }
    }
    $body = [regex]::Replace((WithoutFences $text), '`[^`\r\n]*`', '')
    # Inline links plus reference-style definitions. Code samples are not links.
    $targets = [Collections.Generic.List[string]]::new()
    foreach ($m in [regex]::Matches($body, '\]\(<?([^\s)>]+)>?(?:\s+"[^"]*")?\)')) { $targets.Add($m.Groups[1].Value) }
    foreach ($m in [regex]::Matches($body, '(?m)^\s*\[[^\]]+\]:\s*<?([^\s>]+)>?')) { $targets.Add($m.Groups[1].Value) }
    foreach ($target in $targets) {
        if ($target -match '^[a-zA-Z][a-zA-Z0-9+.-]*:') { continue }
        $parts = $target.Split('#', 2)
        $dest = if ($parts[0]) { [IO.Path]::GetFullPath((Join-Path (Split-Path $path) ([Uri]::UnescapeDataString($parts[0])))) } else { $path }
        if ($dest -ne $root -and -not $dest.StartsWith($prefix, [StringComparison]::OrdinalIgnoreCase)) { Report $relative "link leaves repository: $target"; continue }
        $local = [IO.Path]::GetRelativePath($root, $dest).Replace('\','/')
        if ($local -match '(^|/)\.local(/|$)') { Report $relative "link depends on local state: $target"; continue }
        if (-not (Test-Path -LiteralPath $dest)) { Report $relative "missing link: $target"; continue }
        if ($parts.Count -eq 2 -and $parts[1] -and $dest.EndsWith('.md') -and -not (AnchorSet $dest).Contains([Uri]::UnescapeDataString($parts[1]))) { Report $relative "missing anchor: $target" }
    }
    # Parent and Breakdown are links, not a prescribed task spelling or status vocabulary.
    foreach ($m in [regex]::Matches($(if ($fields['Lifecycle'] -eq 'historical') { '' } else { $body }), '(?m)^\*\*(Parent|Breakdown):\*\*\s*([^\r\n]+)')) {
        if ($m.Groups[2].Value.Trim() -ne 'none' -and $m.Groups[2].Value -notmatch '\[[^\]]+\]\([^)]+\)') { Report $relative "$($m.Groups[1].Value) must reference its related work" }
    }
    if ($config.closureDirectory -and $relative.StartsWith($config.closureDirectory.TrimEnd('/') + '/') -and $relative -ne $config.closureIndex) {
        foreach ($name in @('Roadmap source', 'Planning baseline', 'Work IDs', 'Outcome', 'Evidence', 'Limitations')) { if (-not $fields.ContainsKey($name)) { Report $relative "closure missing $name" } }
        foreach ($heading in @('Preserved roadmap', 'Acceptance disposition')) { if ($text -notmatch ('(?m)^## ' + [regex]::Escape($heading) + '\s*$')) { Report $relative "closure missing section: $heading" } }
        if ($config.closureIndex -and (Test-Path -LiteralPath (FullPath $config.closureIndex))) {
            $name = [IO.Path]::GetFileName($relative)
            if ((ReadText (FullPath $config.closureIndex)) -notmatch [regex]::Escape($name)) { Report $relative 'closure is not discoverable from its index' }
        }
    }
}
$errors | ForEach-Object { Write-Output $_ }
Write-Output "$($files.Count) documents checked, $($errors.Count) problem(s). Structural checks do not establish acceptance."
if ($errors.Count) { exit 1 }
