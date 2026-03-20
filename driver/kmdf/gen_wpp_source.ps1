$ErrorActionPreference = "Stop"

$projectDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$root = Join-Path $projectDir ".."
$root = [System.IO.Path]::GetFullPath($root)
$outPath = Join-Path $root "pmicglink8550_wpp.c"

$mainPath = Join-Path $root "main.c"
$mainLines = Get-Content -Path $mainPath

$includeFiles = @(
    "device.c",
    "interfaces.c",
    "channel.c",
    "transport.c",
    "crashdump.c",
    "battery.c",
    "platform.c",
    "requests.c",
    "ulog.c",
    "queue.c"
)

$skipLines = @(
    '#include "main.tmh"'
) + ($includeFiles | ForEach-Object { '#include "' + $_ + '"' })

$builder = New-Object System.Text.StringBuilder

foreach ($line in $mainLines) {
    if ($skipLines -contains $line) {
        continue
    }

    [void]$builder.AppendLine($line)
    if ($line -eq '#include "kmdf/trace.h"') {
        [void]$builder.AppendLine('#include "pmicglink8550_wpp.tmh"')
    }
}

foreach ($file in $includeFiles) {
    [void]$builder.AppendLine("")
    [void]$builder.Append((Get-Content -Raw -Path (Join-Path $root $file)))
    [void]$builder.AppendLine("")
}

[System.IO.File]::WriteAllText($outPath, $builder.ToString(), [System.Text.Encoding]::ASCII)
