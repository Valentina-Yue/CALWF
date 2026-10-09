# ============================================================
# CALWF 打包脚本（PowerShell）
# ============================================================
# 用法：在 PowerShell 里运行
#   cd D:\CALWF
#   .\package.ps1
# ============================================================

$ErrorActionPreference = "Stop"

$RootDir   = "D:\CALWF"
$TempDir   = "$env:TEMP\CALWF_package"
$ZipName   = "CALWF_submission.zip"
$ZipPath   = "$RootDir\$ZipName"

Write-Host ""
Write-Host "=================================================="
Write-Host "  CALWF 打包"
Write-Host "=================================================="

# 清理旧的临时目录
if (Test-Path $TempDir) {
    Remove-Item $TempDir -Recurse -Force
}
New-Item -ItemType Directory -Path $TempDir | Out-Null

# ---------- 1. 源代码 ----------
Write-Host ""
Write-Host "[1/5] 复制源代码..."

Copy-Item "$RootDir\src"     "$TempDir\src"     -Recurse -Force
Copy-Item "$RootDir\include" "$TempDir\include" -Recurse -Force

# ---------- 2. 配置文件 ----------
Write-Host ""
Write-Host "[2/5] 复制配置文件..."

$configFiles = @(
    "CMakeLists.txt",
    "build.bat",
    "config.json",
    "README.md",
    "LICENSE",
    ".gitignore"
)
foreach ($f in $configFiles) {
    $src = Join-Path $RootDir $f
    if (Test-Path $src) {
        Copy-Item $src "$TempDir\" -Force
    }
}

# ---------- 3. 文档 ----------
Write-Host ""
Write-Host "[3/5] 复制文档..."

if (Test-Path "$RootDir\docs") {
    # 创建 docs 目录
    New-Item -ItemType Directory -Path "$TempDir\docs" -Force | Out-Null
    
    # 只复制根目录的 .md 文件，不复制 archive
    Get-ChildItem "$RootDir\docs" -Filter "*.md" -File | ForEach-Object {
        Copy-Item $_.FullName "$TempDir\docs\" -Force
    }
}

# ---------- 4. 数据配置 ----------
Write-Host ""
Write-Host "[4/5] 复制数据配置（不含大语料）..."

New-Item -ItemType Directory -Path "$TempDir\data\config"   -Force | Out-Null
New-Item -ItemType Directory -Path "$TempDir\data\metadata" -Force | Out-Null

$dataFiles = @(
    @{ Src = "$RootDir\data\config\prompts.json";     Dst = "$TempDir\data\config\" },
    @{ Src = "$RootDir\data\config\risk_words.txt";   Dst = "$TempDir\data\config\" },
    @{ Src = "$RootDir\data\metadata\llm_meta.csv";   Dst = "$TempDir\data\metadata\" }
)
foreach ($item in $dataFiles) {
    if (Test-Path $item.Src) {
        Copy-Item $item.Src $item.Dst -Force
    }
}

# ---------- 5. 打包 ----------
Write-Host ""
Write-Host "[5/5] 打包为 zip..."

if (Test-Path $ZipPath) {
    Remove-Item $ZipPath -Force
}

Compress-Archive -Path "$TempDir\*" -DestinationPath $ZipPath

# 清理临时目录
Remove-Item $TempDir -Recurse -Force

# ---------- 完成 ----------
$size = [math]::Round((Get-Item $ZipPath).Length / 1KB, 2)

Write-Host ""
Write-Host "=================================================="
Write-Host "  打包成功！"
Write-Host "=================================================="
Write-Host ""
Write-Host "  文件位置：$ZipPath"
Write-Host "  文件大小：$size KB"
Write-Host ""