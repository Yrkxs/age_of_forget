[CmdletBinding()]
param(
    [string]$ExePath = (Join-Path $PSScriptRoot 'release\newAOE.exe'),
    [string]$WorkingDirectory = $PSScriptRoot,
    [int]$DurationSeconds = 5,
    [string[]]$Maps = @('map.njust', 'map1.njust', 'map2.njust', 'map3.njust'),
    [int[]]$Rotations = @(0, 90, 180, 270),
    [string]$OutputDirectory = (Join-Path $PSScriptRoot 'map-rotation-test-results')
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

function Resolve-ExistingPath([string]$Path, [string]$BaseDirectory) {
    if ([IO.Path]::IsPathRooted($Path)) {
        return [IO.Path]::GetFullPath($Path)
    }
    return [IO.Path]::GetFullPath((Join-Path $BaseDirectory $Path))
}

$ExePath = Resolve-ExistingPath $ExePath $WorkingDirectory
$WorkingDirectory = Resolve-ExistingPath $WorkingDirectory $PSScriptRoot
$OutputDirectory = Resolve-ExistingPath $OutputDirectory $WorkingDirectory

if (-not (Test-Path -LiteralPath $ExePath -PathType Leaf)) {
    throw "Executable not found: $ExePath"
}
if (-not (Test-Path -LiteralPath $WorkingDirectory -PathType Container)) {
    throw "Working directory not found: $WorkingDirectory"
}
if ($DurationSeconds -lt 1) {
    throw 'DurationSeconds must be at least 1.'
}

New-Item -ItemType Directory -Force -Path $OutputDirectory | Out-Null
$stamp = Get-Date -Format 'yyyyMMdd-HHmmss'
$runDirectory = Join-Path $OutputDirectory $stamp
New-Item -ItemType Directory -Force -Path $runDirectory | Out-Null

$failurePatterns = @(
    'map rotation failed',
    'fixed map file not found',
    'map file not found',
    'Critical',
    'Fatal'
)
$results = New-Object System.Collections.Generic.List[object]
$total = $Maps.Count * $Rotations.Count
$index = 0

foreach ($map in $Maps) {
    $mapPath = Resolve-ExistingPath $map $WorkingDirectory
    if (-not (Test-Path -LiteralPath $mapPath -PathType Leaf)) {
        $results.Add([pscustomobject]@{
            Map = $map
            Rotation = '-'
            Status = 'FAIL'
            Reason = "Map file not found: $mapPath"
            Log = ''
        })
        $index += $Rotations.Count
        continue
    }

    foreach ($rotation in $Rotations) {
        $index++
        $safeMap = [IO.Path]::GetFileNameWithoutExtension($map)
        $caseName = '{0:D2}-{1}-rotate-{2}' -f $index, $safeMap, $rotation
        $stdoutPath = Join-Path $runDirectory "$caseName.stdout.log"
        $stderrPath = Join-Path $runDirectory "$caseName.stderr.log"
        $gameLogPath = Join-Path $runDirectory "$caseName.GameLog.log"
        $gameLog = Join-Path $WorkingDirectory 'GameLog.log'
        $status = 'PASS'
        $reason = "Ran for $DurationSeconds seconds"
        $process = $null

        Write-Host "[$index/$total] $map @ ${rotation}deg" -ForegroundColor Cyan
        Remove-Item -LiteralPath $gameLog -Force -ErrorAction SilentlyContinue

        try {
            $process = Start-Process -FilePath $ExePath `
                -WorkingDirectory $WorkingDirectory `
                -ArgumentList @('--offscreen', '--freq', 'MAX', '--map', $map, '--rotate', $rotation) `
                -RedirectStandardOutput $stdoutPath `
                -RedirectStandardError $stderrPath `
                -PassThru

            $deadline = (Get-Date).AddSeconds($DurationSeconds)
            while (-not $process.HasExited -and (Get-Date) -lt $deadline) {
                Start-Sleep -Milliseconds 200
                $process.Refresh()
            }

            if ($process.HasExited) {
                $process.WaitForExit()
                $process.Refresh()
                $exitCode = $process.ExitCode
                if ($null -eq $exitCode) {
                    $status = 'FAIL'
                    $reason = 'Process exited, but no exit code was available'
                } elseif ($exitCode -ne 0) {
                    $status = 'FAIL'
                    $reason = "Process exited with code $exitCode"
                } else {
                    $status = 'FAIL'
                    $reason = 'Process exited before the observation window ended'
                }
            } else {
                $process.Kill()
                $process.WaitForExit()
            }
        } catch {
            $status = 'FAIL'
            $reason = $_.Exception.Message
            if ($null -ne $process -and -not $process.HasExited) {
                $process.Kill()
                $process.WaitForExit()
            }
        }

        if (Test-Path -LiteralPath $gameLog -PathType Leaf) {
            Copy-Item -LiteralPath $gameLog -Destination $gameLogPath -Force
        }

        $logParts = @($stdoutPath, $stderrPath, $gameLogPath) | Where-Object {
            Test-Path -LiteralPath $_ -PathType Leaf
        }
        $allLogText = ($logParts | ForEach-Object {
            Get-Content -LiteralPath $_ -Raw -ErrorAction SilentlyContinue
        }) -join "`n"

        $matchedFailure = $failurePatterns | Where-Object {
            $allLogText -match [regex]::Escape($_)
        } | Select-Object -First 1
        if ($null -ne $matchedFailure) {
            $status = 'FAIL'
            $reason = "Detected: $matchedFailure"
        }

        $results.Add([pscustomobject]@{
            Map = $map
            Rotation = $rotation
            Status = $status
            Reason = $reason
            Log = $gameLogPath
        })
        Write-Host "       $status - $reason" -ForegroundColor $(if ($status -eq 'PASS') { 'Green' } else { 'Red' })
    }
}

$summaryPath = Join-Path $runDirectory 'summary.csv'
$results | Export-Csv -LiteralPath $summaryPath -NoTypeInformation -Encoding UTF8

Write-Host ''
Write-Host '========== Map rotation test summary ==========' -ForegroundColor White
$results | Format-Table Map, Rotation, Status, Reason -AutoSize
$failed = @($results | Where-Object Status -eq 'FAIL')
Write-Host "Total: $($results.Count), Passed: $($results.Count - $failed.Count), Failed: $($failed.Count)"
Write-Host "Details: $runDirectory"

if ($failed.Count -gt 0) {
    exit 1
}
exit 0
