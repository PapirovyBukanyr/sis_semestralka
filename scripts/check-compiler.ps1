# Check for a C compiler in PATH. Returns 0 if one is found, 1 otherwise.
$compilers = @('gcc','cc','cl')
foreach ($c in $compilers) {
    if (Get-Command $c -ErrorAction SilentlyContinue) {
        Write-Output "C compiler found: $c"
        exit 0
    }
}
Write-Error "No C compiler found in PATH. Install GCC (MinGW/MSYS) or MSVC and ensure it's on PATH."
exit 1
