# DarkStudio - prepare the LEGO Gears dataset for a CPU vs SYCL (Intel GPU) training comparison (M0b).
# SPDX-License-Identifier: Apache-2.0
#
# The dataset is by Stéphane Charette, CC BY-NC-SA: it is used for local testing only and never redistributed.
#   https://www.ccoderun.ca/programming/2024-05-01_LegoGears/
#
# Usage: pwsh -File tools\train-test\prepare-legogears.ps1 [-MaxBatches 300] [-Batch 64] [-Subdivisions 1]
# Output: build\train-test\legogears\{LegoGears.data, LegoGears.cfg, LegoGears.names, train.txt, valid.txt}

param(
	[int] $MaxBatches = 300,
	[int] $Batch = 64,
	[int] $Subdivisions = 1,
	[int] $ValidEvery = 10		# every N-th image goes to the validation list
)

$ErrorActionPreference = 'Stop'
$root	= (Resolve-Path "$PSScriptRoot\..\..").Path
$zip	= Join-Path $root 'datasets\legogears_2_dataset.zip'
$src	= Join-Path $root 'datasets\legogears\LegoGears_v2'
$out	= Join-Path $root 'build\train-test\legogears'

if (-not (Test-Path $src))
{
	New-Item -ItemType Directory -Force (Split-Path $zip) | Out-Null
	if (-not (Test-Path $zip))
	{
		Invoke-WebRequest 'https://www.ccoderun.ca/programming/2024-05-01_LegoGears/legogears_2_dataset.zip' -OutFile $zip
	}
	Expand-Archive $zip -DestinationPath (Join-Path $root 'datasets\legogears') -Force
}
New-Item -ItemType Directory -Force $out | Out-Null

# deterministic split, sorted by path so CPU and GPU runs always see the same lists
$images	= Get-ChildItem $src -Recurse -Filter *.jpg | Sort-Object FullName | ForEach-Object FullName
$train	= @(); $valid = @()
for ($i = 0; $i -lt $images.Count; $i ++)
{
	if (($i % $ValidEvery) -eq ($ValidEvery - 1)) { $valid += $images[$i] } else { $train += $images[$i] }
}
Set-Content (Join-Path $out 'train.txt') $train
Set-Content (Join-Path $out 'valid.txt') $valid
Copy-Item (Join-Path $src 'LegoGears.names') $out -Force

$classes = (Get-Content (Join-Path $src 'LegoGears.names') | Where-Object { $_.Trim() }).Count
Set-Content (Join-Path $out 'LegoGears.data') @(
	"classes = $classes",
	"train = $out\train.txt",
	"valid = $out\valid.txt",
	"names = $out\LegoGears.names",
	"backup = $out"
)

# training settings: the published cfg has batch=1 (set for inference); burn-in and steps scaled to MaxBatches
$burnIn	= [math]::Max(1, [int]($MaxBatches / 3))
$steps	= "{0},{1}" -f [int]($MaxBatches * 0.8), [int]($MaxBatches * 0.9)
$cfg = Get-Content (Join-Path $src 'LegoGears.cfg') | ForEach-Object {
	switch -Regex ($_)
	{
		'^batch='			{ "batch=$Batch"; break }
		'^subdivisions='	{ "subdivisions=$Subdivisions"; break }
		'^max_batches='		{ "max_batches=$MaxBatches"; break }
		'^burn_in='			{ "burn_in=$burnIn"; break }
		'^steps='			{ "steps=$steps"; break }
		default				{ $_ }
	}
}
Set-Content (Join-Path $out 'LegoGears.cfg') $cfg

"images: {0} train, {1} valid, {2} classes" -f $train.Count, $valid.Count, $classes
"cfg: batch=$Batch subdivisions=$Subdivisions max_batches=$MaxBatches burn_in=$burnIn steps=$steps"
"output: $out"
