# Builds build/studio/TitbitStudio.exe: Titbit Studio as one file, no Python needed
# to run it. Needs Python 3.11 with: pip install -r requirements-build.txt
param([string]$Python = 'python')
$ErrorActionPreference = 'Stop'
Push-Location $PSScriptRoot
try {
    & $Python -m PyInstaller --noconfirm --onefile --windowed --name TitbitStudio `
        --distpath ..\build\studio --workpath ..\build\studio-work --specpath ..\build\studio-work `
        --add-data "$PSScriptRoot\web;web" --paths . `
        --hidden-import server --hidden-import formats --hidden-import exporters `
        --hidden-import animation --hidden-import scene_runtime `
        studio_launcher.py
    if ($LASTEXITCODE) { throw "PyInstaller failed ($LASTEXITCODE)" }
    Write-Host "OK: $(Resolve-Path ..\build\studio\TitbitStudio.exe)"
} finally { Pop-Location }
