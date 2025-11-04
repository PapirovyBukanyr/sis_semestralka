param(
    [string]$AnalyzerExe = "./bin/analyzer.exe"
)

function Copy-IfExists($src, $dst){
    if(Test-Path $src){
        Write-Host "Copying $src -> $dst"
        Copy-Item -Force $src $dst
        return $true
    }
    return $false
}

# Ensure output dir exists
if(-not (Test-Path "./bin")) { New-Item -ItemType Directory -Path ./bin | Out-Null }

$dllCopied = $false

# Common MSYS2 / MinGW64 locations
$candidates = @(
    "$Env:MSYS2_HOME\mingw64\bin\SDL2.dll",
    "C:\\msys64\\mingw64\\bin\\SDL2.dll",
    "C:\\msys64\\usr\\bin\\SDL2.dll",
    "C:\\msys64\\usr\\local\\bin\\SDL2.dll",
    "$Env:ProgramFiles\\SDL2\\bin\\SDL2.dll"
)

foreach($p in $candidates){
    if([string]::IsNullOrEmpty($p)) { continue }
    if(Copy-IfExists $p "./bin/SDL2.dll"){ $dllCopied = $true; break }
}

if(-not $dllCopied){
    Write-Host "SDL2.dll not found in common locations. If you want the SDL window UI, install SDL2 and ensure SDL2.dll is available or place it in ./bin/." -ForegroundColor Yellow
}

if(-not (Test-Path $AnalyzerExe)){
    Write-Host "Analyzer executable not found at $AnalyzerExe" -ForegroundColor Red
    exit 1
}

Write-Host "Starting analyzer: $AnalyzerExe"
# Start process and forward output
$p = Start-Process -FilePath $AnalyzerExe -NoNewWindow -PassThru -Wait
exit $p.ExitCode
