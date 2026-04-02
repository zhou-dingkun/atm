param(
    [string]$QtDir = "D:\Qt\6.10.0\mingw_64",
    [string]$BuildDir = "c:\Users\zdk\Desktop\project\atm\qt_gui\build",
    [string]$DeployDir = "c:\Users\zdk\Desktop\project\atm\qt_gui\build\deploy",
    [string]$InnoISCC = "C:\Program Files (x86)\Inno Setup 6\ISCC.exe"
)

$exe = Join-Path $BuildDir "atm_gui_qt.exe"
$windeployqt = Join-Path $QtDir "bin\windeployqt.exe"

if (!(Test-Path $exe)) {
    Write-Error "找不到 $exe，请先编译 Qt GUI。"
    exit 1
}
if (!(Test-Path $windeployqt)) {
    Write-Error "找不到 windeployqt：$windeployqt"
    exit 1
}
if (!(Test-Path $InnoISCC)) {
    Write-Error "找不到 ISCC：$InnoISCC，请安装 Inno Setup。"
    exit 1
}

if (Test-Path $DeployDir) {
    Remove-Item -Recurse -Force $DeployDir
}
New-Item -ItemType Directory -Path $DeployDir | Out-Null

Copy-Item $exe -Destination $DeployDir
Copy-Item "c:\Users\zdk\Desktop\project\atm\src\atm.exe" -Destination $DeployDir

& $windeployqt --dir $DeployDir $exe

Push-Location "c:\Users\zdk\Desktop\project\atm\installer"
& $InnoISCC "atm.iss"
Pop-Location

Write-Host "Installer generated in c:\Users\zdk\Desktop\project\atm\installer"