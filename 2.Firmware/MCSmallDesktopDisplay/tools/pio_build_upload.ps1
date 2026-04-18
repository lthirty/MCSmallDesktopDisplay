param(
  [string]$Port = "COM3"
)

$ErrorActionPreference = "Stop"

$projectRoot = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
$pioExe = Join-Path $env:USERPROFILE ".platformio\penv\Scripts\platformio.exe"
$binExportDir = (Resolve-Path (Join-Path $projectRoot "..\02.bin文件-封装好的代码")).Path
$env:BIN_EXPORT_DIR = $binExportDir

if (-not (Test-Path $pioExe)) {
  throw "未找到 PlatformIO 可执行文件: $pioExe"
}

function Get-FreeDriveLetter {
  foreach ($d in @("X","Y","Z","W","V")) {
    if (-not (Get-PSDrive -Name $d -ErrorAction SilentlyContinue)) {
      return $d
    }
  }
  throw "没有可用盘符用于 subst 映射。"
}

$workDir = $projectRoot
$mappedDrive = $null

try {
  if ($projectRoot.Length -gt 80) {
    $mappedDrive = Get-FreeDriveLetter
    cmd /c "subst $mappedDrive`: `"$projectRoot`""
    $workDir = "$mappedDrive`:\"
    Write-Host "已启用短路径映射: $workDir -> $projectRoot"
  }

  Write-Host "开始编译..."
  & $pioExe run -d $workDir
  if ($LASTEXITCODE -ne 0) {
    throw "编译失败，退出码: $LASTEXITCODE"
  }

  Write-Host "开始烧写到 $Port ..."
  & $pioExe run -d $workDir -t upload --upload-port $Port
  if ($LASTEXITCODE -ne 0) {
    throw "烧写失败，退出码: $LASTEXITCODE"
  }

  Write-Host "完成：编译+烧写成功。"
}
finally {
  if ($mappedDrive) {
    cmd /c "subst $mappedDrive`: /d" | Out-Null
    Write-Host "已清理盘符映射: $mappedDrive`:"
  }
}
