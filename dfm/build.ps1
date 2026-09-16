$ErrorActionPreference = 'Stop'

# 脚本自身位于 dfm/ 下，目录结构：dfm/src、dfm/include、dfm/build
$srcDir   = Join-Path $PSScriptRoot 'src'
$incDir   = Join-Path $PSScriptRoot 'include'
$buildDir = Join-Path $PSScriptRoot 'build'
$outExe   = Join-Path $buildDir 'main.exe'

if (-not (Test-Path $srcDir)) { Write-Error "找不到源码目录: $srcDir"; exit 1 }

New-Item -ItemType Directory -Force -Path $buildDir | Out-Null

$files = Get-ChildItem (Join-Path $srcDir '*.c') | Select-Object -ExpandProperty FullName
if (-not $files) { Write-Error "src 目录下没有找到任何 .c 文件"; exit 1 }

Write-Host '>> 编译源文件:'
$files | ForEach-Object { Write-Host "   $_" }

& gcc @files -finput-charset=GBK -fexec-charset=GBK -std=c17 `
    -Wall -Wextra -Wpedantic "-I$incDir" -o $outExe

if ($LASTEXITCODE -eq 0) {
    Write-Host ">> 构建成功: $outExe"
} else {
    Write-Host ">> 构建失败，gcc 退出码 $LASTEXITCODE"
}
exit $LASTEXITCODE
