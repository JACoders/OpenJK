# SHA-1 of the netcodec sources (contents, in path order), for validate.bat: compared at the start and the end of a
# run, it tells whether a file changed during the run (the run then tested a mix and must be redone).
# Usage: powershell -NoProfile -ExecutionPolicy Bypass -File source_hash.ps1 <codemp\netcodec directory>
param([string]$Dir)
$sha = [System.Security.Cryptography.SHA1]::Create()
$items = 'CMakeLists.txt', 'CMakePresets.json', 'include', 'src', 'test', 'bench', 'tools', 'fuzz' | ForEach-Object { Join-Path $Dir $_ }
foreach ($f in Get-ChildItem -Path $items -Recurse -File | Sort-Object FullName) {
	$b = [System.IO.File]::ReadAllBytes($f.FullName)
	[void]$sha.TransformBlock($b, 0, $b.Length, $null, 0)
}
[void]$sha.TransformFinalBlock([byte[]]@(), 0, 0)
[System.BitConverter]::ToString($sha.Hash) -replace '-', ''
