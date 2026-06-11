param(
    [string]$ProjectRoot = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path,
    [string]$DonePath = '',
    [string]$InboxPath = '',
    [string]$OutputPath = ''
)

$ErrorActionPreference = 'Stop'

if (-not $DonePath) {
    $DonePath = Join-Path $ProjectRoot 'Coordination\done'
}
if (-not $InboxPath) {
    $InboxPath = Join-Path $ProjectRoot 'Coordination\inbox-hex'
}
if (-not $OutputPath) {
    $OutputPath = Join-Path $ProjectRoot 'Saved\Proof\coordination-status-summary.json'
}

$allowedStatuses = @(
    'completed',
    'blocked_requires_handoff',
    'staged_not_live',
    'proof_only',
    'superseded'
)

$proofDir = Split-Path -Parent $OutputPath
New-Item -ItemType Directory -Force -Path $proofDir | Out-Null

function Add-Count([hashtable]$Table, [string]$Key) {
    if ([string]::IsNullOrWhiteSpace($Key)) { $Key = 'legacy_unknown' }
    if (-not $Table.ContainsKey($Key)) { $Table[$Key] = 0 }
    $Table[$Key]++
}

function Get-TaskStatus([string]$Text) {
    $matches = [regex]::Matches($Text, '(?im)^Status:\s*([a-z_]+)\b')
    if ($matches.Count -gt 0) {
        $latest = $matches[$matches.Count - 1]
        $status = $latest.Groups[1].Value.ToLowerInvariant()
        if ($allowedStatuses -contains $status) {
            return $status
        }
        return 'unrecognized_status'
    }
    return 'legacy_unknown'
}

function Get-InferredStatus([string]$Text, [string]$ExplicitStatus) {
    if ($ExplicitStatus -ne 'legacy_unknown') {
        return $ExplicitStatus
    }

    if ($Text -match '(?i)blocked_requires_handoff|required live-session handoff|not handed over|without explicit handoff|blocked by required|blocked by live-session|must not be closed|pending while live canary') {
        return 'blocked_requires_handoff'
    }
    if ($Text -match '(?i)staged_not_live|staged|not live|not loaded|not adopted|no live-adoption claim|has not loaded|until a host-safe refresh|until.*new module') {
        return 'staged_not_live'
    }
    if ($Text -match '(?i)proof_only|proof-only|proof only|read-only evidence|read-only proof|diagnosis|inventory|audit|summary') {
        return 'proof_only'
    }
    if ($Text -match '(?im)^Completed by|^Result:\s*completed|\bBuildExitCode=0\b|\bAutomationExitCode=0\b|exit code:\s*0|exit `0`') {
        return 'completed'
    }

    return 'legacy_unknown'
}

$doneFiles = if (Test-Path -LiteralPath $DonePath -PathType Container) {
    @(Get-ChildItem -LiteralPath $DonePath -File -Filter '*.md')
} else {
    @()
}
$inboxFiles = if (Test-Path -LiteralPath $InboxPath -PathType Container) {
    @(Get-ChildItem -LiteralPath $InboxPath -File -Filter '*.md')
} else {
    @()
}

$statusCounts = @{}
$inferredStatusCounts = @{}
$records = @()

foreach ($file in ($doneFiles | Sort-Object Name)) {
    $text = Get-Content -Raw -LiteralPath $file.FullName
    $status = Get-TaskStatus $text
    $inferredStatus = Get-InferredStatus $text $status
    Add-Count $statusCounts $status
    Add-Count $inferredStatusCounts $inferredStatus
    $completedAndLive =
        $status -eq 'completed' -and
        ($text -match '(?im)\blive_adopted\b|completed_and_live\s*=\s*true|LiveStatus\s*[:=]\s*live_adopted')

    $records += [pscustomobject]@{
        Name = $file.Name
        Status = $status
        InferredStatus = $inferredStatus
        StatusSource = if ($status -eq 'legacy_unknown') { 'inferred_from_legacy_text' } else { 'explicit_status_line' }
        StatusLineCount = [regex]::Matches($text, '(?im)^Status:\s*([a-z_]+)\b').Count
        CompletedAndLive = [bool]$completedAndLive
        LastWriteTimeUtc = $file.LastWriteTimeUtc.ToString('o')
    }
}

foreach ($status in $allowedStatuses) {
    if (-not $statusCounts.ContainsKey($status)) {
        $statusCounts[$status] = 0
    }
    if (-not $inferredStatusCounts.ContainsKey($status)) {
        $inferredStatusCounts[$status] = 0
    }
}
if (-not $statusCounts.ContainsKey('legacy_unknown')) { $statusCounts['legacy_unknown'] = 0 }
if (-not $statusCounts.ContainsKey('unrecognized_status')) { $statusCounts['unrecognized_status'] = 0 }
if (-not $inferredStatusCounts.ContainsKey('legacy_unknown')) { $inferredStatusCounts['legacy_unknown'] = 0 }
if (-not $inferredStatusCounts.ContainsKey('unrecognized_status')) { $inferredStatusCounts['unrecognized_status'] = 0 }

$completedAndLiveCount = @($records | Where-Object { $_.CompletedAndLive }).Count
$statusObjects = foreach ($key in ($statusCounts.Keys | Sort-Object)) {
    [pscustomobject]@{ Status = $key; Count = [int]$statusCounts[$key] }
}
$inferredStatusObjects = foreach ($key in ($inferredStatusCounts.Keys | Sort-Object)) {
    [pscustomobject]@{ Status = $key; Count = [int]$inferredStatusCounts[$key] }
}

$result = [pscustomobject]@{
    ProjectRoot = $ProjectRoot
    ObservedUtc = (Get-Date).ToUniversalTime().ToString('o')
    DonePath = $DonePath
    InboxPath = $InboxPath
    DoneCount = $doneFiles.Count
    ActiveInboxCount = $inboxFiles.Count
    ActiveInboxFiles = @($inboxFiles | Sort-Object Name | Select-Object Name, Length, LastWriteTimeUtc)
    StatusCounts = $statusObjects
    InferredStatusCounts = $inferredStatusObjects
    CompletedCount = [int]$statusCounts['completed']
    CompletedAndLiveCount = $completedAndLiveCount
    BlockedRequiresHandoffCount = [int]$statusCounts['blocked_requires_handoff']
    StagedNotLiveCount = [int]$statusCounts['staged_not_live']
    ProofOnlyCount = [int]$statusCounts['proof_only']
    SupersededCount = [int]$statusCounts['superseded']
    LegacyUnknownCount = [int]$statusCounts['legacy_unknown']
    UnrecognizedStatusCount = [int]$statusCounts['unrecognized_status']
    InferredCompletedCount = [int]$inferredStatusCounts['completed']
    InferredBlockedRequiresHandoffCount = [int]$inferredStatusCounts['blocked_requires_handoff']
    InferredStagedNotLiveCount = [int]$inferredStatusCounts['staged_not_live']
    InferredProofOnlyCount = [int]$inferredStatusCounts['proof_only']
    InferredSupersededCount = [int]$inferredStatusCounts['superseded']
    InferredLegacyUnknownCount = [int]$inferredStatusCounts['legacy_unknown']
    InferredUnrecognizedStatusCount = [int]$inferredStatusCounts['unrecognized_status']
    Records = $records
}

$result | ConvertTo-Json -Depth 8 | Set-Content -LiteralPath $OutputPath -Encoding UTF8
$result | ConvertTo-Json -Depth 8
