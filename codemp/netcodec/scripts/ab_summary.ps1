# Summary of ab_msvc.bat, as ab_summary.awk for the other A/B scripts: reads the bench_codec outputs out-<build>-<round>.txt
# of a directory and prints the best warm times of each build against "ref", then checks netcodec mismatches, the
# unchanged legacy encoder moving by more than 4 % within a build (a busy machine), and the legacy difference between
# builds (code placement: a netcodec difference of that size is not a result).
param([string]$Dir)
[System.Threading.Thread]::CurrentThread.CurrentCulture = [System.Globalization.CultureInfo]::InvariantCulture
$builds = [ordered]@{}
$bad = 0
foreach ($f in Get-ChildItem -Path $Dir -Filter 'out-*-*.txt' | Sort-Object { [int]($_.BaseName -split '-')[2] }, Name) {
	$b = ($f.BaseName -split '-')[1]
	$t = Get-Content $f.FullName
	$warm = { param($pattern) [double](($t | Where-Object { $_ -match $pattern } | Select-Object -First 1) -replace '.*warm\s+', '') }
	$e = & $warm '^\s+encode netcodec'
	$d = & $warm '^\s+decode netcodec'
	$le = & $warm '^\s+encode legacy'
	$m = $t | Where-Object { $_ -match 'netcodec mismatches (\d+)' } | Select-Object -First 1
	if ($m -match 'netcodec mismatches (\d+)') { $bad += [int]$Matches[1] }
	if (-not $builds.Contains($b)) { $builds[$b] = @{ e = $e; d = $d; lmin = $le; lmax = $le } }
	$x = $builds[$b]
	$x.e = [math]::Min($x.e, $e); $x.d = [math]::Min($x.d, $d)
	$x.lmin = [math]::Min($x.lmin, $le); $x.lmax = [math]::Max($x.lmax, $le)
}
$busy = $false
$names = @($builds.Keys | Where-Object { $_ -eq 'ref' }) + @($builds.Keys | Where-Object { $_ -ne 'ref' })
foreach ($b in $names) {
	$x = $builds[$b]
	$line = 'best {0,-5} warm encode {1,8:F1} decode {2,8:F1} | legacy encode {3,8:F1} to {4,8:F1}' -f $b, $x.e, $x.d, $x.lmin, $x.lmax
	if ($b -ne 'ref' -and $builds.Contains('ref')) {
		$r = $builds['ref']
		$line += ' | against ref: encode {0:+0.0;-0.0} %, decode {1:+0.0;-0.0} %, legacy {2:+0.0;-0.0} %' -f (100 * ($x.e / $r.e - 1)), (100 * ($x.d / $r.d - 1)), (100 * ($x.lmin / $r.lmin - 1))
	}
	Write-Output $line
	if ($x.lmax -gt $x.lmin * 1.04) { $busy = $true }
}
if ($bad -gt 0) { Write-Output 'WARNING: netcodec mismatches: the builds do not code the same bytes' }
if ($busy) { Write-Output 'WARNING: the unchanged legacy encoder moved by more than 4 % within a build: the machine was busy, run again when it is quiet' }
Write-Output '(the legacy difference between builds is code placement: a netcodec difference of that size is not a result)'
