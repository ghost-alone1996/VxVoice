param(
    [string]$QtPrefix = "C:\Qt\6.7.3\mingw_64",
    [string]$MingwBin = "C:\Qt\Tools\mingw1310_64\bin",
    [string]$PjsipRoot = "C:\Qt\pjsip-mingw64",
    [string]$WixDir = "C:\Users\leonardo\Documents\ProjetoDEV\Softphone-Windows\build_tools\wix"
)

$ErrorActionPreference = "Stop"
$ProgressPreference = "SilentlyContinue"

$ProjectDir = $PSScriptRoot
$BuildDir = Join-Path $ProjectDir "build"
$StageDir = Join-Path $BuildDir "msi_stage"
$BuildWinDir = Join-Path $ProjectDir "build_win"
$OutputDir = Split-Path -Parent $ProjectDir

$env:PATH = "$MingwBin;$QtPrefix\bin;$env:PATH"

Write-Host "==> Configurando CMake (Qt 6 + MinGW + PJSIP)..." -ForegroundColor Cyan
cmake -S $ProjectDir -B $BuildDir -G "MinGW Makefiles" `
    -DCMAKE_BUILD_TYPE=Release `
    "-DCMAKE_PREFIX_PATH=$QtPrefix" `
    "-DCMAKE_CXX_COMPILER=$MingwBin/g++.exe" `
    "-DCMAKE_C_COMPILER=$MingwBin/gcc.exe" `
    "-DCMAKE_MAKE_PROGRAM=$MingwBin/mingw32-make.exe" `
    "-DPJSIP_ROOT=$PjsipRoot"

Write-Host "==> Compilando VxVoice.exe..." -ForegroundColor Cyan
cmake --build $BuildDir --config Release --parallel

$ExePath = Join-Path $BuildDir "VxVoice.exe"
if (-not (Test-Path $ExePath)) {
    throw "Executavel nao encontrado em $ExePath"
}

Write-Host "==> Preparando staging e dependencias Qt6..." -ForegroundColor Cyan
if (Test-Path $StageDir) {
    Remove-Item -Recurse -Force $StageDir
}
New-Item -ItemType Directory -Force -Path (Join-Path $StageDir "sounds") | Out-Null

Copy-Item $ExePath -Destination $StageDir -Force
Copy-Item (Join-Path $ProjectDir "resources\sounds\*.wav") -Destination (Join-Path $StageDir "sounds") -Force
Copy-Item (Join-Path $ProjectDir "resources\icons\app_icon.ico") -Destination (Join-Path $StageDir "vxvoice.ico") -Force
Copy-Item (Join-Path $BuildWinDir "dist\LICENSE.txt") -Destination $StageDir -Force
Copy-Item (Join-Path $BuildWinDir "dist\README.txt") -Destination $StageDir -Force
Copy-Item (Join-Path $BuildWinDir "dist\vxvoice.ini") -Destination $StageDir -Force

& "$QtPrefix\bin\windeployqt6.exe" --release --no-translations --no-opengl-sw (Join-Path $StageDir "VxVoice.exe")
if ($LASTEXITCODE -ne 0) { throw "windeployqt6 falhou com codigo $LASTEXITCODE" }
foreach ($dll in @("libgcc_s_seh-1.dll", "libstdc++-6.dll", "libwinpthread-1.dll")) {
    if (-not (Test-Path (Join-Path $StageDir $dll))) {
        Copy-Item (Join-Path $MingwBin $dll) -Destination $StageDir -Force
    }
}

# Remove VxVoice.exe do stage para evitar duplicidade com C_VxVoiceExe em vxvoice_qt.wxs
Remove-Item (Join-Path $StageDir "VxVoice.exe") -Force

# Gera RTF simples de licenca para WixUI_InstallDir
$LicenseTxt = Get-Content (Join-Path $BuildWinDir "dist\LICENSE.txt") -Raw
$LicenseRtfPath = Join-Path $BuildDir "license.rtf"
$RtfEscaped = $LicenseTxt -replace '\\', '\\' -replace '\{', '\{' -replace '\}', '\}' -replace "`r?`n", "\par`r`n"
"{\rtf1\ansi\deff0{\fonttbl{\f0 Segoe UI;}}\fs20 $RtfEscaped}" | Set-Content -Path $LicenseRtfPath -Encoding Ascii

$HeatExe = Join-Path $WixDir "heat.exe"
$CandleExe = Join-Path $WixDir "candle.exe"
$LightExe = Join-Path $WixDir "light.exe"

$HarvestWxs = Join-Path $BuildDir "VxVoice_Files.wxs"
$MainWxs = Join-Path $BuildWinDir "vxvoice_qt.wxs"

Write-Host "==> Mapeando arquivos com WiX Heat..." -ForegroundColor Cyan
& $HeatExe dir $StageDir `
    -cg AppFilesComponentGroup `
    -dr INSTALLFOLDER `
    -scom -sreg -srd -sfrag `
    -gg -var var.StageDir `
    -out $HarvestWxs

Write-Host "==> Compilando tabelas MSI com WiX Candle..." -ForegroundColor Cyan
& $CandleExe -arch x64 `
    "-dStageDir=$StageDir" `
    "-dExePath=$ExePath" `
    "-dIconPath=$(Join-Path $ProjectDir 'resources\icons\app_icon.ico')" `
    "-dLicenseRtf=$LicenseRtfPath" `
    -ext WixUIExtension `
    -out "$BuildDir\" `
    $MainWxs $HarvestWxs

$MsiX64 = Join-Path $OutputDir "vxvoice-2.0.6-x64.msi"
$MsiWin = Join-Path $OutputDir "vxvoice-2.0.6-Windows.msi"

Write-Host "==> Gerando pacote .msi final com WiX Light..." -ForegroundColor Cyan
& $LightExe -sval -ext WixUIExtension -cultures:pt-BR `
    -out $MsiX64 `
    (Join-Path $BuildDir "vxvoice_qt.wixobj") `
    (Join-Path $BuildDir "VxVoice_Files.wixobj")

Copy-Item $MsiX64 -Destination $MsiWin -Force

Write-Host ""
Write-Host "==> Pacote MSI gerado com sucesso:" -ForegroundColor Green
Write-Host "    $MsiX64" -ForegroundColor Green
Write-Host "    $MsiWin" -ForegroundColor Green
