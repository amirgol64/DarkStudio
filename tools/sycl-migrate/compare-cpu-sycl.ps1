# DarkStudio - compare Darknet CPU and SYCL (Intel GPU) inference on the same images (M0b).
# SPDX-License-Identifier: Apache-2.0
#
# Runs darknet_06_images_to_json from the CPU install (build/install) and from the SYCL install (build/install-sycl),
# then compares every detection: class, probability and box.
#
# Usage: powershell -File tools\sycl-migrate\compare-cpu-sycl.ps1 [-Images <files>] [-Repeat 3]

param(
	[string[]] $Images = (Get-ChildItem "$PSScriptRoot\..\..\darknet\artwork" -Filter *.jpg | ForEach-Object FullName),
	[int] $Repeat = 3,
	[string] $Model = "$PSScriptRoot\..\..\models\pretrained",
	[string] $OneApiBin = "C:\Program Files (x86)\Intel\oneAPI\2026.1\bin"
)

$ErrorActionPreference = 'Stop'
$root = (Resolve-Path "$PSScriptRoot\..\..").Path
$work = Join-Path $root 'build\compare-cpu-sycl'
New-Item -ItemType Directory -Force $work | Out-Null

# repeat the image list so the timing after the first (JIT / warm-up) image is meaningful
$list = @()
for ($i = 0; $i -lt $Repeat; $i ++) { $list += $Images }

function Invoke-Darknet([string] $name, [string] $bin, [string] $extraPath)
{
	$dir = Join-Path $work $name
	New-Item -ItemType Directory -Force $dir | Out-Null
	$old = $env:PATH
	$env:PATH = "$bin;$extraPath;$old"
	Push-Location $dir
	try
	{
		$darknetArgs = @("$Model\coco.names", "$Model\yolov4-tiny.cfg", "$Model\yolov4-tiny.weights") + $list
		& "$bin\darknet_06_images_to_json.exe" @darknetArgs *> run.log
		if ($LASTEXITCODE -ne 0) { throw "$name run failed, see $dir\run.log" }
	}
	finally
	{
		Pop-Location
		$env:PATH = $old
	}
	return (Get-Content (Join-Path $dir 'output.json') -Raw | ConvertFrom-Json).file
}

$cpu  = Invoke-Darknet 'cpu'  (Join-Path $root 'build\install\bin')      ''
$sycl = Invoke-Darknet 'sycl' (Join-Path $root 'build\install-sycl\bin') $OneApiBin

$worstProb = 0.0
$worstBox  = 0
$mismatch  = 0
for ($i = 0; $i -lt $Images.Count; $i ++)
{
	# an image without detections has no "predictions", so filter out the resulting $null
	$a = @($cpu[$i].predictions | Where-Object { $_ })
	$b = @($sycl[$i].predictions | Where-Object { $_ })
	$name = Split-Path $cpu[$i].filename -Leaf
	if ($a.Count -ne $b.Count)
	{
		Write-Output ("{0,-12} DIFFERENT COUNT: cpu={1} sycl={2}" -f $name, $a.Count, $b.Count)
		$mismatch ++
		continue
	}
	for ($k = 0; $k -lt $a.Count; $k ++)
	{
		$p = $a[$k]; $q = $b[$k]
		$dp = [math]::Abs($p.best_probability - $q.best_probability)
		$db = @([math]::Abs($p.rect.x - $q.rect.x), [math]::Abs($p.rect.y - $q.rect.y), [math]::Abs($p.rect.width - $q.rect.width), [math]::Abs($p.rect.height - $q.rect.height)) | Measure-Object -Maximum | ForEach-Object Maximum
		if ($p.best_class -ne $q.best_class) { $mismatch ++ }
		$worstProb = [math]::Max($worstProb, $dp)
		$worstBox  = [math]::Max($worstBox, $db)
		Write-Output ("{0,-12} {1,-8} cpu {2,6:P1}  sycl {3,6:P1}  diff {4:N4}  box diff {5}px" -f $name, $p.name.Split(' ')[0], $p.best_probability, $q.best_probability, $dp, $db)
	}
}

function Get-SteadyMs($files)
{
	# skip the first pass over the images: it includes model upload and SYCL kernel JIT compilation
	$steady = $files | Select-Object -Skip $Images.Count | ForEach-Object { [double]($_.duration -replace '[^\d.]', '') }
	return ($steady | Measure-Object -Average).Average
}

Write-Output ''
Write-Output ("class mismatches: {0}, worst probability difference: {1:N4}, worst box difference: {2}px" -f $mismatch, $worstProb, $worstBox)
Write-Output ("first image:  cpu {0}, sycl {1}" -f $cpu[0].duration, $sycl[0].duration)
Write-Output ("steady state: cpu {0:N1} ms, sycl {1:N1} ms per image" -f (Get-SteadyMs $cpu), (Get-SteadyMs $sycl))
