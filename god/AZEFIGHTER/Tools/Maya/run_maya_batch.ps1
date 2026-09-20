# Maya をバッチモードで起動して Python スクリプトを実行する。
# mayapy は日本語環境のユーザー設定読み込みで落ちることがあるため、mayabatch を使い、
# ユーザー設定はこのフォルダの appdir に分けて読ませる
param([Parameter(Mandatory = $true)][string]$Script, [int]$TimeoutSec = 300)
$here = $PSScriptRoot
$env:MAYA_APP_DIR = Join-Path $here 'appdir'
$env:MAYA_SKIP_USERSETUP_PY = '1'
$env:MAYA_UI_LANGUAGE = 'en_US'
$env:MAYA_DISABLE_CIP = '1'
$env:MAYA_DISABLE_CER = '1'
$env:PYTHONPATH = $null
$path = (Resolve-Path $Script).Path.Replace('\', '/')
$arg = '-command "python(\"__file__=''' + $path + '''; exec(open(''' + $path + ''', encoding=''utf-8'').read())\")"'
$log = Join-Path $here 'maya_batch.log'
$p = Start-Process -FilePath 'C:\Program Files\Autodesk\Maya2024\bin\mayabatch.exe' -ArgumentList $arg -RedirectStandardOutput $log -RedirectStandardError "$log.err" -NoNewWindow -PassThru
if (-not $p.WaitForExit($TimeoutSec * 1000)) { 'TIMEOUT'; $p.Kill() }
Get-Content $log | Where-Object { $_ -match 'RESULT|SHEET|Error|Traceback' }
