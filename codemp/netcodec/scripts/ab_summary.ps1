# Summary of ab_msvc.bat, as ab_summary.awk for the other A/B scripts: reads the bench_codec outputs out-<build>-<round>.txt
# of a directory and prints the best warm and cold times of each build against "ref" (a warm time of 0: the build's
# benchmark predates the warm timing, an old reference commit), then checks netcodec mismatches, the unchanged legacy
# encoder moving by more than 4 % within a build (a busy machine), and the legacy difference between builds (code
# placement: a netcodec difference of that size is not a result).
param([string]$Dir)
[System.Threading.Thread]::CurrentThread.CurrentCulture = [System.Globalization.CultureInfo]::InvariantCulture
function Pct($x, $r) { if ($r -gt 0) { '{0:+0.0;-0.0} %' -f (100 * ($x / $r - 1)) } else { 'n/a' } }
$builds = [ordered]@{}
$bad = 0
foreach ($f in Get-ChildItem -Path $Dir -Filter 'out-*-*.txt' | Sort-Object { [int]($_.BaseName -split '-')[2] }, Name) {
	$b = ($f.BaseName -split '-')[1]
	$t = Get-Content $f.FullName
	# cold and warm ns per message of a timing line (warm 0 when the benchmark has no warm column)
	$time = { param($name)
		foreach ($l in $t) {
			if ($l -match "^\s+$name\s+([\d.]+) ns/message(\s+warm\s+([\d.]+))?") { return @([double]$Matches[1], [double]('0' + $Matches[3])) }
		}
		return @(0.0, 0.0)
	}
	$en = & $time 'encode netcodec'
	$dn = & $time 'decode netcodec'
	$le = (& $time 'encode legacy')[1]
	$m = $t | Where-Object { $_ -match 'netcodec mismatches (\d+)' } | Select-Object -First 1
	if ($m -match 'netcodec mismatches (\d+)') { $bad += [int]$Matches[1] }
	if (-not $builds.Contains($b)) { $builds[$b] = @{ e = $en[1]; d = $dn[1]; ce = $en[0]; cd = $dn[0]; lmin = $le; lmax = $le } }
	$x = $builds[$b]
	$x.e = [math]::Min($x.e, $en[1]); $x.d = [math]::Min($x.d, $dn[1])
	$x.ce = [math]::Min($x.ce, $en[0]); $x.cd = [math]::Min($x.cd, $dn[0])
	$x.lmin = [math]::Min($x.lmin, $le); $x.lmax = [math]::Max($x.lmax, $le)
}
$busy = $false
$nowarm = $false
$names = @($builds.Keys | Where-Object { $_ -eq 'ref' }) + @($builds.Keys | Where-Object { $_ -ne 'ref' })
foreach ($b in $names) {
	$x = $builds[$b]
	$line = 'best {0,-5} warm encode {1,8:F1} decode {2,8:F1} | cold encode {3,6:F0} decode {4,6:F0} | legacy warm encode {5,8:F1} to {6,8:F1}' -f $b, $x.e, $x.d, $x.ce, $x.cd, $x.lmin, $x.lmax
	if ($b -ne 'ref' -and $builds.Contains('ref')) {
		$r = $builds['ref']
		$line += ' | against ref: warm encode {0}, decode {1}, cold encode {2}, decode {3}, legacy {4}' -f (Pct $x.e $r.e), (Pct $x.d $r.d), (Pct $x.ce $r.ce), (Pct $x.cd $r.cd), (Pct $x.lmin $r.lmin)
	}
	Write-Output $line
	if ($x.lmax -gt $x.lmin * 1.04) { $busy = $true }
	if ($x.e -eq 0) { $nowarm = $true }
}
if ($bad -gt 0) { Write-Output 'WARNING: netcodec mismatches: the builds do not code the same bytes' }
if ($busy) { Write-Output 'WARNING: the unchanged legacy encoder moved by more than 4 % within a build: the machine was busy, run again when it is quiet' }
if ($nowarm) { Write-Output 'NOTE: a build has no warm timing (its benchmark predates it): only the cold timings compare, and they vary by up to 9 % between runs' }
Write-Output '(the legacy difference between builds is code placement: a netcodec difference of that size is not a result)'
