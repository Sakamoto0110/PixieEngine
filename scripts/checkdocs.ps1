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
$pathComparison = if ($IsWindows) { [StringComparison]::OrdinalIgnoreCase } else { [StringComparison]::Ordinal }
$errors = [Collections.Generic.List[string]]::new()
$notValidated = [Collections.Generic.HashSet[string]]::new([StringComparer]::Ordinal)
$ids = @{}
$anchors = [Collections.Generic.Dictionary[string,object]]::new($(if ($IsWindows) { [StringComparer]::OrdinalIgnoreCase } else { [StringComparer]::Ordinal }))
$config = @{ exclude = @(); requiredDocuments = @(); metadataSchema = ''; closureDirectory = ''; closureIndex = ''; scopeId = ''; scopes = @(); identityManifest = ''; identityBaseline = '' }
if ($ConfigPath) {
    $loaded = Get-Content -LiteralPath $ConfigPath -Raw | ConvertFrom-Json -AsHashtable
    foreach ($key in $loaded.Keys) { $config[$key] = $loaded[$key] }
}
function Report([string]$path, [string]$message) { $errors.Add("${path}: $message") }
function Within([string]$path, [string]$directory) {
    return $path.Equals($directory, $pathComparison) -or $path.StartsWith($directory.TrimEnd('/','\') + [IO.Path]::DirectorySeparatorChar, $pathComparison)
}
function AssertNoReparse([string]$path) {
    # Check every existing ancestor, including roots and intermediate junctions.
    $cursor = [IO.Path]::GetFullPath($path)
    while ($cursor) {
        $item = Get-Item -LiteralPath $cursor -Force -ErrorAction SilentlyContinue
        if ($item -and ($item.Attributes -band [IO.FileAttributes]::ReparsePoint)) { throw "reparse path is not allowed: $cursor" }
        $parent = [IO.Path]::GetDirectoryName($cursor)
        if ($parent -eq $cursor) { break }
        $cursor = $parent
    }
}
$scoped = [bool]$config.scopeId
$scopes = [Collections.Generic.List[hashtable]]::new()
if ($scoped) {
    if ($config.scopeId -cnotmatch '^[a-z0-9][a-z0-9-]*$') { throw 'Invalid scopeId.' }
    AssertNoReparse $root
    $scopes.Add(@{ id = $config.scopeId; root = $root; available = $true; optional = $false })
    foreach ($entry in $config.scopes) {
        if ($entry -isnot [Collections.IDictionary] -or $entry.id -cnotmatch '^[a-z0-9][a-z0-9-]*$' -or -not $entry.path -or $entry.optional -isnot [bool]) { throw 'Invalid scope declaration.' }
        if ([IO.Path]::IsPathRooted($entry.path)) { throw 'Scope paths must be relative to RepositoryPath.' }
        $scopeRoot = [IO.Path]::GetFullPath((Join-Path $root $entry.path)).TrimEnd('/','\')
        AssertNoReparse $scopeRoot
        foreach ($existing in $scopes) {
            if ($entry.id -ceq $existing.id -or (Within $scopeRoot $existing.root) -or (Within $existing.root $scopeRoot)) { throw "Duplicate or overlapping scope: $($entry.id)" }
        }
        $available = Test-Path -LiteralPath $scopeRoot -PathType Container
        $scopes.Add(@{ id = $entry.id; root = $scopeRoot; available = $available; optional = $entry.optional })
        if (-not $available) {
            if ($entry.optional) { [void]$notValidated.Add("optional scope $($entry.id) unavailable; not validated") }
            else { Report $entry.id 'required scope unavailable' }
        }
    }
} elseif ($config.scopes.Count -or $config.identityManifest -or $config.identityBaseline) { throw 'scopeId is required for cross-scope configuration.' }
if ($config.identityBaseline -and -not $config.identityManifest) { throw 'identityBaseline requires identityManifest.' }
function ScopeFor([string]$path) {
    foreach ($scope in $scopes) { if (Within $path $scope.root) { return $scope } }
    return $null
}
function ResolveTarget([string]$path, [string]$label) {
    if ($scoped) {
        $scope = ScopeFor $path
        if (-not $scope) { Report $label 'path leaves declared scopes'; return $false }
        try { AssertNoReparse $path } catch { Report $label $_.Exception.Message; return $false }
        $local = [IO.Path]::GetRelativePath($scope.root, $path).Replace('\','/')
        if ($local -match '(^|/)\.local(/|$)') { Report $label 'link depends on local state'; return $false }
        if (-not $scope.available) {
            if ($scope.optional) { [void]$notValidated.Add("$label : optional scope $($scope.id) unavailable; target not validated") }
            else { Report $label "target scope $($scope.id) unavailable" }
            return $false
        }
    } else {
        if (-not (Within $path $root)) { Report $label 'link leaves repository'; return $false }
        $local = [IO.Path]::GetRelativePath($root, $path).Replace('\','/')
        if ($local -match '(^|/)\.local(/|$)') { Report $label 'link depends on local state'; return $false }
    }
    if (-not (Test-Path -LiteralPath $path)) { Report $label 'missing link'; return $false }
    return $true
}
function FullPath([string]$relative) {
    $full = [IO.Path]::GetFullPath((Join-Path $root $relative))
    if (-not $full.StartsWith($prefix, $pathComparison)) { throw "Path leaves repository: $relative" }
    if ($scoped) { AssertNoReparse $full }
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
    if ($fields.ContainsKey('Canonical scope')) {
        $canonicalScope = @($scopes | Where-Object { $_.id -ceq $fields['Canonical scope'] })
        if (-not $scoped -or $canonicalScope.Count -ne 1) { Report $relative 'Canonical scope must identify a declared scope' }
        elseif (-not $fields.ContainsKey('Canonical') -or -not $fields['Canonical'] -or [IO.Path]::IsPathRooted($fields['Canonical'])) { Report $relative 'scoped Canonical must be a relative path' }
        else {
            $canonicalParts = $fields['Canonical'].Split('#', 2)
            $canonicalTarget = [IO.Path]::GetFullPath((Join-Path $canonicalScope[0].root ([Uri]::UnescapeDataString($canonicalParts[0]))))
            if (-not (Within $canonicalTarget $canonicalScope[0].root)) { Report $relative 'Canonical path leaves its declared scope' }
            elseif (ResolveTarget $canonicalTarget "${relative}: Canonical") {
                if (-not (Test-Path -LiteralPath $canonicalTarget -PathType Leaf)) { Report $relative 'Canonical target must be a file' }
                elseif ($canonicalParts.Count -eq 2 -and $canonicalParts[1] -and -not (AnchorSet $canonicalTarget).Contains([Uri]::UnescapeDataString($canonicalParts[1]))) { Report $relative 'missing Canonical anchor' }
            }
        }
    }
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
    foreach ($m in [regex]::Matches($body, '\]\((?:<([^>\r\n]+)>|([^\s)>]+))(?:\s+"[^"]*")?\)')) { $targets.Add($(if ($m.Groups[1].Success) { $m.Groups[1].Value } else { $m.Groups[2].Value })) }
    foreach ($m in [regex]::Matches($body, '(?m)^\s*\[[^\]]+\]:\s*(?:<([^>\r\n]+)>|([^\s>]+))')) { $targets.Add($(if ($m.Groups[1].Success) { $m.Groups[1].Value } else { $m.Groups[2].Value })) }
    foreach ($target in $targets) {
        if ($target -match '^[a-zA-Z][a-zA-Z0-9+.-]*:') { continue }
        $parts = $target.Split('#', 2)
        $dest = if ($parts[0]) { [IO.Path]::GetFullPath((Join-Path (Split-Path $path) ([Uri]::UnescapeDataString($parts[0])))) } else { $path }
        if (-not (ResolveTarget $dest "${relative}: $target")) { continue }
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
$registryFiles = [Collections.Generic.Dictionary[string,string]]::new([StringComparer]::Ordinal)
function RequireFields($value, [string[]]$required, [string[]]$optional = @()) {
    if ($value -isnot [Collections.IDictionary]) { throw 'expected an object' }
    foreach ($name in $required) { if (-not $value.Contains($name)) { throw "missing field: $name" } }
    foreach ($name in $value.Keys) { if ($name -cnotin $required -and $name -cnotin $optional) { throw "unknown field: $name" } }
}
function ValidateLocation($value) {
    if ($value.scope -isnot [string] -or $value.scope -cnotmatch '^[a-z0-9][a-z0-9-]*$' -or $value.path -isnot [string] -or -not $value.path) { throw 'invalid scope/path' }
    if ($value.Contains('anchor') -and ($value.anchor -isnot [string] -or -not $value.anchor)) { throw 'invalid anchor' }
}
function RegistryCoordinate($entry) {
    # Coordinates are lexical within a declared root even when that optional root
    # is unavailable. Files and fragments are checked separately by RegistryLocation.
    $scope = @($scopes | Where-Object { $_.id -ceq $entry.scope })
    if ($scope.Count -ne 1) { throw "undeclared identity scope: $($entry.scope)" }
    if ([IO.Path]::IsPathRooted($entry.path)) { throw 'identity path must be relative' }
    $location = [IO.Path]::GetFullPath((Join-Path $scope[0].root $entry.path))
    if (-not (Within $location $scope[0].root)) { throw 'identity path leaves its scope' }
    $local = [IO.Path]::GetRelativePath($scope[0].root, $location).Replace('\','/')
    if ($IsWindows) { $local = $local.ToLowerInvariant() }
    $fragment = if ($entry.Contains('anchor')) { $entry.anchor } else { '' }
    return ConvertTo-Json -InputObject @($entry.scope, $local, $fragment) -Compress
}
function RegistryLocation($entry, [string]$label) {
    $scope = @($scopes | Where-Object { $_.id -ceq $entry.scope })
    if ($scope.Count -ne 1) { Report $label "undeclared identity scope: $($entry.scope)"; return $null }
    if (-not $entry.path -or [IO.Path]::IsPathRooted($entry.path)) { Report $label 'identity path must be relative'; return $null }
    $location = [IO.Path]::GetFullPath((Join-Path $scope[0].root $entry.path))
    if (-not (Within $location $scope[0].root)) { Report $label 'identity path leaves its scope'; return $null }
    if (-not (ResolveTarget $location $label)) { return $null }
    if (-not (Test-Path -LiteralPath $location -PathType Leaf)) { Report $label 'identity target must be a file'; return $null }
    if ($entry.Contains('anchor') -and $entry.anchor -and -not (AnchorSet $location).Contains($entry.anchor)) { Report $label 'missing identity anchor' }
    return $location
}
function LoadRegistry([string]$relative, [string]$label) {
    if ([IO.Path]::IsPathRooted($relative)) { Report $label 'registry path must be relative'; return $null }
    $location = [IO.Path]::GetFullPath((Join-Path $root $relative))
    if (-not (ResolveTarget $location $label)) { return $null }
    try {
        $registry = ReadText $location | ConvertFrom-Json -AsHashtable
        RequireFields $registry @('schemaVersion','identities','aliases','projections') @('locationAliases')
        if ($registry.schemaVersion -isnot [long] -and $registry.schemaVersion -isnot [int]) { throw 'schemaVersion must be an integer' }
        if ($registry.schemaVersion -ne 1 -or $registry.identities -isnot [array] -or $registry.aliases -isnot [array] -or $registry.projections -isnot [array]) { throw 'expected schemaVersion 1 and identities/aliases/projections arrays' }
        foreach ($entry in $registry.identities) {
            RequireFields $entry @('key','scope','path','kind') @('anchor')
            ValidateLocation $entry
            if ($entry.key -isnot [string] -or $entry.key -cnotmatch '^[a-z0-9][a-z0-9-]*::[^\s:]+$' -or $entry.kind -cnotin @('document','item') -or ($entry.kind -ceq 'item' -and -not $entry.Contains('anchor'))) { throw 'invalid canonical identity' }
        }
        foreach ($entry in $registry.aliases) {
            RequireFields $entry @('key','scope','path','target') @('anchor')
            ValidateLocation $entry
            if ($entry.key -isnot [string] -or $entry.key -cnotmatch '^[a-z0-9][a-z0-9-]*::[^\s:]+$' -or $entry.target -isnot [string] -or $entry.target -cnotmatch '^[a-z0-9][a-z0-9-]*::[^\s:]+$') { throw 'invalid alias key/target' }
        }
        if (-not $registry.Contains('locationAliases')) { $registry['locationAliases'] = @() }
        if ($registry.locationAliases -isnot [array]) { throw 'locationAliases must be an array' }
        foreach ($entry in $registry.locationAliases) {
            RequireFields $entry @('scope','path','target') @('anchor')
            ValidateLocation $entry
            if ($entry.target -isnot [string] -or $entry.target -cnotmatch '^[a-z0-9][a-z0-9-]*::[^\s:]+$') { throw 'invalid location alias target' }
        }
        foreach ($entry in $registry.projections) {
            RequireFields $entry @('scope','path','sources')
            ValidateLocation $entry
            if ($entry.sources -isnot [array] -or -not $entry.sources.Count) { throw 'projection must have sources' }
            foreach ($source in $entry.sources) {
                RequireFields $source @('key','sha256')
                if ($source.key -isnot [string] -or $source.key -cnotmatch '^[a-z0-9][a-z0-9-]*::[^\s:]+$' -or $source.sha256 -isnot [string] -or $source.sha256 -cnotmatch '^[0-9a-f]{64}$') { throw 'invalid projection source key/hash' }
            }
        }
        return $registry
    } catch { Report $label "invalid registry: $($_.Exception.Message)"; return $null }
}
if ($config.identityManifest) {
    $registry = LoadRegistry $config.identityManifest 'identityManifest'
    if ($registry) {
        $canonical = [Collections.Generic.Dictionary[string,object]]::new([StringComparer]::Ordinal)
        $aliases = [Collections.Generic.Dictionary[string,object]]::new([StringComparer]::Ordinal)
        foreach ($entry in $registry.identities) {
            if ($entry -isnot [Collections.IDictionary] -or $entry.key -cnotmatch '^[a-z0-9][a-z0-9-]*::[^\s:]+$' -or $entry.kind -cnotin @('document','item') -or ($entry.kind -ceq 'item' -and -not $entry.anchor)) { Report 'identityManifest' 'invalid canonical identity'; continue }
            if ($canonical.ContainsKey($entry.key)) { Report $entry.key 'duplicate canonical key'; continue }
            $canonical[$entry.key] = $entry
            $location = RegistryLocation $entry $entry.key
            if ($location) {
                $registryFiles[$entry.key] = $location
                if ($entry.kind -ceq 'document') {
                    $fields = Metadata (ReadText $location)
                    if ($fields.ContainsKey('Document ID') -and $fields['Document ID'] -cne $entry.key.Split('::', 2)[1]) { Report $entry.key 'canonical Document ID differs from qualified key suffix' }
                }
            }
        }
        foreach ($entry in $registry.aliases) {
            if ($entry -isnot [Collections.IDictionary] -or $entry.key -cnotmatch '^[a-z0-9][a-z0-9-]*::[^\s:]+$' -or -not $entry.target) { Report 'identityManifest' 'invalid alias'; continue }
            if ($aliases.ContainsKey($entry.key) -or $canonical.ContainsKey($entry.key)) { Report $entry.key 'alias key collision'; continue }
            $aliases[$entry.key] = $entry
            [void](RegistryLocation $entry $entry.key)
            if (-not $canonical.ContainsKey($entry.target)) { Report $entry.key 'alias target must be a direct canonical key; chains/cycles are forbidden' }
        }
        $canonicalCoordinates = [Collections.Generic.HashSet[string]]::new([StringComparer]::Ordinal)
        $identityAliasCoordinates = [Collections.Generic.HashSet[string]]::new([StringComparer]::Ordinal)
        $locationAliases = [Collections.Generic.Dictionary[string,object]]::new([StringComparer]::Ordinal)
        foreach ($entry in $canonical.Values) {
            try { [void]$canonicalCoordinates.Add((RegistryCoordinate $entry)) }
            catch { Report $entry.key $_.Exception.Message }
        }
        foreach ($entry in $aliases.Values) {
            try { [void]$identityAliasCoordinates.Add((RegistryCoordinate $entry)) }
            catch { Report $entry.key $_.Exception.Message }
        }
        foreach ($entry in $registry.locationAliases) {
            $label = "location alias $($entry.scope)/$($entry.path)"
            try { $coordinate = RegistryCoordinate $entry }
            catch { Report $label $_.Exception.Message; continue }
            if ($locationAliases.ContainsKey($coordinate)) { Report $label 'duplicate location alias source coordinate'; continue }
            $locationAliases[$coordinate] = $entry
            if ($canonicalCoordinates.Contains($coordinate)) { Report $label 'location alias shadows a canonical coordinate; self/chain ambiguity is forbidden' }
            if ($identityAliasCoordinates.Contains($coordinate)) { Report $label 'location alias overlaps an identity alias source coordinate' }
            if (-not $canonical.ContainsKey($entry.target)) { Report $label 'location alias target must be a direct canonical key; chains/cycles are forbidden' }
            [void](RegistryLocation $entry $label)
        }
        if ($config.identityBaseline) {
            $baseline = LoadRegistry $config.identityBaseline 'identityBaseline'
            if ($baseline) {
                foreach ($entry in $baseline.identities) {
                    if (-not $canonical.ContainsKey($entry.key) -or $canonical[$entry.key].kind -cne $entry.kind) { Report $entry.key 'baseline canonical identity removed or reassigned' }
                    # Scope/path relocation is deliberately not identity reassignment.
                }
                foreach ($entry in $baseline.aliases) {
                    if (-not $aliases.ContainsKey($entry.key) -or $aliases[$entry.key].target -cne $entry.target) { Report $entry.key 'baseline alias removed or reassigned' }
                }
                foreach ($entry in $baseline.locationAliases) {
                    try { $coordinate = RegistryCoordinate $entry }
                    catch { Report 'identityBaseline location alias' $_.Exception.Message; continue }
                    if (-not $locationAliases.ContainsKey($coordinate) -or $locationAliases[$coordinate].target -cne $entry.target) { Report 'identityBaseline' 'baseline location alias removed or reassigned' }
                }
            }
        }
        foreach ($projection in $registry.projections) {
            if ($projection -isnot [Collections.IDictionary] -or $projection.sources -isnot [array]) { Report 'identityManifest' 'invalid projection'; continue }
            [void](RegistryLocation $projection 'projection')
            foreach ($source in $projection.sources) {
                if ($source -isnot [Collections.IDictionary] -or -not $canonical.ContainsKey([string]$source.key) -or $source.sha256 -cnotmatch '^[0-9a-f]{64}$') { Report 'projection' 'invalid canonical source/hash'; continue }
                if ($registryFiles.ContainsKey($source.key)) {
                    $actual = (Get-FileHash -LiteralPath $registryFiles[$source.key] -Algorithm SHA256).Hash.ToLowerInvariant()
                    if ($actual -cne $source.sha256) { Report $source.key 'stale catalog projection source hash' }
                } else { [void]$notValidated.Add("projection source $($source.key) unavailable; freshness not validated") }
            }
        }
    }
}
$errors | ForEach-Object { Write-Output $_ }
$notValidated | Sort-Object | ForEach-Object { Write-Output "NOT VALIDATED: $_" }
Write-Output "$($files.Count) documents checked, $($errors.Count) problem(s). Structural checks do not establish acceptance."
if ($errors.Count) { exit 1 }
