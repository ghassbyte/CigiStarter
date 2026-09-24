<#
.SYNOPSIS
    Fetch SRTM terrain tiles for CigiStarter's HAT/HOT terrain database.

.DESCRIPTION
    CigiStarter answers HAT/HOT requests from the local Prepar3D's own terrain
    through the PdkTerrainProbe add-on when it is installed, and from an offline
    SRTM tile set in the "terrain" folder beside the program otherwise. The
    offline copy stays live while the local Prepar3D is held in slew mode by
    whatever drives it (TerrainDb.h explains why). This script fetches the raw
    1 arc-second ".hgt" tiles that folder needs, from the public AWS "skadi"
    mirror (SRTMGL1, public domain).

    Each 1-degree by 1-degree tile is about 25 MB uncompressed. Name the tiles
    you want directly, or give one or more lat,lon points and let the script
    work out which tiles cover them. Tiles already present are left alone
    unless -Force is given.

.PARAMETER Tiles
    Tile names, e.g. S07E106. One file per tile.

.PARAMETER Points
    "lat,lon" pairs (decimal degrees). Each is mapped to the tile that covers
    it, e.g. "-6.13,106.66" -> S07E106.

.PARAMETER Out
    Destination folder. Defaults to the "terrain" folder beside CigiStarter
    (this script lives in tools\, so ..\terrain).

.PARAMETER Force
    Re-download tiles that are already present.

.EXAMPLE
    # The two tiles covering Jakarta and Surabaya:
    .\get_terrain.ps1 -Tiles S07E106,S08E112

.EXAMPLE
    # Or by airport position:
    .\get_terrain.ps1 -Points "-6.13,106.66","-6.53,106.76","-7.38,112.79"

.EXAMPLE
    # Into a folder of your choice, then point the program at it:
    .\get_terrain.ps1 -Tiles S07E106 -Out D:\ig\terrain
    CigiStarter.exe --terrain-dir D:\ig\terrain
#>
[CmdletBinding()]
param(
    [string[]] $Tiles  = @(),
    [string[]] $Points = @(),
    [string]   $Out    = "",
    [switch]   $Force,
    [string]   $Source = "https://s3.amazonaws.com/elevation-tiles-prod/skadi"
)

$ErrorActionPreference = "Stop"

if ([string]::IsNullOrWhiteSpace($Out)) {
    $Out = Join-Path (Split-Path -Parent $PSScriptRoot) "terrain"
}

# The two products TerrainDb.cpp accepts: samples per side, and the byte size
# that gives. Anything else is not a tile and should not be installed.
$Expected = @(1201 * 1201 * 2, 3601 * 3601 * 2)

function ConvertTo-TileName([double] $lat, [double] $lon) {
    $la = [math]::Floor($lat)
    $lo = [math]::Floor($lon)
    if ($lo -ge 180) { $lo = 179 }   # nothing is named E180
    $ns = if ($la -lt 0) { "S" } else { "N" }
    $ew = if ($lo -lt 0) { "W" } else { "E" }
    "{0}{1:D2}{2}{3:D3}" -f $ns, [math]::Abs($la), $ew, [math]::Abs($lo)
}

# Collapse the tile arguments and the point arguments into one set of names.
$wanted = New-Object System.Collections.Generic.List[string]
foreach ($t in $Tiles) {
    if ($t -match '^\s*[NSns]\d{2}[EWew]\d{3}\s*$') {
        $wanted.Add($t.Trim().ToUpper())
    } else {
        Write-Warning "ignoring '$t': not a tile name like S07E106"
    }
}
foreach ($p in $Points) {
    $bits = $p -split ','
    if ($bits.Count -ne 2) {
        Write-Warning "ignoring '$p': expected ""lat,lon"""
        continue
    }
    $lat = 0.0; $lon = 0.0
    if (-not [double]::TryParse($bits[0], [ref] $lat) -or
        -not [double]::TryParse($bits[1], [ref] $lon)) {
        Write-Warning "ignoring '$p': lat/lon did not parse"
        continue
    }
    $wanted.Add((ConvertTo-TileName $lat $lon))
}
$wanted = $wanted | Select-Object -Unique

if ($wanted.Count -eq 0) {
    Write-Host "Nothing to fetch. Give -Tiles S07E106 or -Points ""-6.13,106.66""."
    exit 1
}

New-Item -ItemType Directory -Force -Path $Out | Out-Null
Write-Host "Terrain folder: $Out"
Write-Host ("Tiles wanted  : {0}" -f ($wanted -join ", "))

$failed = 0
foreach ($tile in $wanted) {
    $hgt = Join-Path $Out "$tile.hgt"
    if ((Test-Path $hgt) -and -not $Force) {
        Write-Host ("  {0}: already present, skipping" -f $tile)
        continue
    }

    $dir = $tile.Substring(0, 3)            # e.g. S07/S07E106.hgt.gz
    $url = "{0}/{1}/{2}.hgt.gz" -f $Source, $dir, $tile
    $gz  = Join-Path ([IO.Path]::GetTempPath()) "$tile.hgt.gz"

    Write-Host ("  {0}: downloading ..." -f $tile)
    try {
        # curl.exe ships with Windows 10+ and is much faster than
        # Invoke-WebRequest on a file this size. Fall back if it is missing.
        $curl = Get-Command curl.exe -ErrorAction SilentlyContinue
        if ($curl) {
            & $curl.Source -L --fail --silent --show-error -o $gz $url
            if ($LASTEXITCODE -ne 0) { throw "curl exited $LASTEXITCODE" }
        } else {
            Invoke-WebRequest -UseBasicParsing -Uri $url -OutFile $gz
        }

        $in  = [IO.File]::OpenRead($gz)
        try {
            $dst = [IO.File]::Create($hgt)
            try {
                $zip = New-Object IO.Compression.GZipStream(
                    $in, [IO.Compression.CompressionMode]::Decompress)
                try { $zip.CopyTo($dst) } finally { $zip.Dispose() }
            } finally { $dst.Dispose() }
        } finally { $in.Dispose() }
    } catch {
        Write-Warning ("  {0}: {1}" -f $tile, $_.Exception.Message)
        Remove-Item $hgt -ErrorAction SilentlyContinue
        ++$failed
        continue
    } finally {
        Remove-Item $gz -ErrorAction SilentlyContinue
    }

    $size = (Get-Item $hgt).Length
    if ($Expected -notcontains $size) {
        Write-Warning ("  {0}: unexpected size {1} bytes -- removing it" -f $tile, $size)
        Remove-Item $hgt -ErrorAction SilentlyContinue
        ++$failed
    } else {
        Write-Host ("  {0}: ok ({1:N1} MB)" -f $tile, ($size / 1MB))
    }
}

if ($failed -gt 0) {
    Write-Warning "$failed tile(s) failed."
    exit 1
}
Write-Host "Done. Start CigiStarter and the window's TerrainDB line should show them."
