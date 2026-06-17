#!/usr/bin/env pwsh
<#
.SYNOPSIS
    Compare participant HALO branches with the reference nstojkov/dev branch
.DESCRIPTION
    This script generates a detailed comparison report identifying issues in participant branches
#>

$ReferenceCommit = "nstojkov/dev"
$ParticipantBranches = @(
    "remotes/origin/G1_HALO",
    "remotes/origin/G2_HALO", 
    "remotes/origin/G3_HALO",
    "remotes/origin/G4_HALO",
    "remotes/origin/G5_HALO",
    "remotes/origin/ftn-halo"
)

# Key files to compare
$KeyFiles = @(
    "halo_dist/hadls/*",
    "apps/zephyr-hart1/src/*",
    "apps/freertos-hart2/src/*",
    "apps/bare-hart3/src/*",
    "apps/linux-hart4/src/*",
    "Makefile",
    "include/memmap.h",
    "src/memory_layout.h"
)

# Create comparison report directory
$ReportDir = "comparison_report"
if (-not (Test-Path $ReportDir)) {
    New-Item -ItemType Directory -Path $ReportDir | Out-Null
}

Write-Host "Starting comparison analysis..."
Write-Host "Reference branch: $ReferenceCommit"
Write-Host "Participant branches: $($ParticipantBranches.Count) found"
Write-Host ""

# For each participant branch
foreach ($Branch in $ParticipantBranches) {
    $BranchName = $Branch.Split('/')[-1]
    $ReportFile = "$ReportDir/$BranchName`_analysis.txt"
    
    Write-Host "Analyzing $BranchName..."
    
    # Get file differences
    $Diff = git diff --name-status $ReferenceCommit...$Branch | Where-Object { $_ -match "(halo_dist|apps|Makefile|memmap|memory_layout)" }
    
    # Get commit differences
    $CommitDiff = git log --oneline --no-decorate $ReferenceCommit..$Branch
    
    # Compare HALO ADL files specifically
    $HaloAdlRef = git show $ReferenceCommit`:halo_dist/hadls/ 2>/dev/null | Select-String -Pattern ".*\.adl|.*\.hml" -ErrorAction SilentlyContinue
    $HaloAdlBranch = git show $Branch`:halo_dist/hadls/ 2>/dev/null | Select-String -Pattern ".*\.adl|.*\.hml" -ErrorAction SilentlyContinue
    
    # Generate report
    @"
========================================
COMPARISON REPORT: $BranchName
========================================
Reference: $ReferenceCommit
Branch: $Branch
Date: $(Get-Date)

## FILES MODIFIED

$($Diff | ForEach-Object { "  $_" })

## COMMITS AHEAD

$($CommitDiff | ForEach-Object { "  $_" })

## HALO ADL FILES (Reference)
$($HaloAdlRef | ForEach-Object { "  $_" })

## HALO ADL FILES (Participant)
$($HaloAdlBranch | ForEach-Object { "  $_" })

========================================
"@ | Out-File -FilePath $ReportFile -Encoding UTF8

    Write-Host "  Report saved to: $ReportFile"
}

Write-Host ""
Write-Host "All comparison reports generated in: $ReportDir"
