# Simple test runner for dump_tokens.exe
# Runs a few integration tests (filter, exclude, compact JSON, memmon fail-on-alert)
$root = Split-Path -Parent $MyInvocation.MyCommand.Definition
$exe = Join-Path $root "..\dump_tokens.exe"
if (-not (Test-Path $exe)) { Write-Error "Executable not found: $exe"; exit 2 }

$all_ok = $true
$tests = @()

function Fail([string]$msg) {
    Write-Host "[FAIL] $msg" -ForegroundColor Red
    $global:all_ok = $false
}
function Pass([string]$msg) {
    Write-Host "[PASS] $msg" -ForegroundColor Green
}

# Test 1: filter (IDENT_VAR)
$test1file = Join-Path $root 't_filter.src'
 '!x;' | Out-File -FilePath $test1file -Encoding ASCII
$out = & $exe -j -c -F IDENT_VAR $test1file 2>&1
if ($LASTEXITCODE -ne 0) { Fail "filter test: exit code $LASTEXITCODE" } else {
    if ($out -match '"type":"IDENT_VAR"' -and $out -match '"lexeme":"!x"') { Pass "filter test" } else { Fail "filter test: unexpected output: $out" }
}

# Test 2: exclude TEXT
$test2file = Join-Path $root 't_exclude.src'
 'escreva("hello", !a);' | Out-File -FilePath $test2file -Encoding ASCII
$out2 = & $exe -j -c -E TEXT $test2file 2>&1
if ($LASTEXITCODE -ne 0) { Fail "exclude test: exit code $LASTEXITCODE" } else {
    if ($out2 -notmatch '"type":"TEXT"') { Pass "exclude TEXT test" } else { Fail "exclude TEXT test: TEXT token present" }
}

# Test 3: compact JSON vs pretty (smoke)
$test3file = Join-Path $root 't_compact.src'
 '!v = 1;' | Out-File -FilePath $test3file -Encoding ASCII
$compact = & $exe -j -c $test3file 2>&1
$pretty = & $exe -j $test3file 2>&1
if ($LASTEXITCODE -ne 0) { Fail "json tests: exit code $LASTEXITCODE" } else {
    if ($compact -match '^\[' -and $compact -notmatch "\n  ") { Pass "compact JSON" } else { Fail "compact JSON formatting unexpected" }
    if ($pretty -match '\n  \{|\A\[  ') { Pass "pretty JSON" } else { Fail "pretty JSON formatting unexpected" }
}

# Test 4: memmon fail-on-alert reading large stdin with tiny memlimit
$bigfile = Join-Path $root 't_big.src'
# create large file to force reallocs
$s = 'A' * 30000
Set-Content -Path $bigfile -Value $s -NoNewline -Encoding ASCII
$proc = Start-Process -FilePath $exe -ArgumentList "-m 1 -f -" -RedirectStandardInput $bigfile -RedirectStandardOutput "${root}\out.txt" -RedirectStandardError "${root}\err.txt" -NoNewWindow -Wait -PassThru
$err = Get-Content -Path (Join-Path $root 'err.txt') -Raw
if ($proc.ExitCode -eq 0) { Fail "memmon fail-on-alert: expected non-zero exit code" } else {
    if ($err -match 'fail-on-alert' -or $err -match 'ERRO: fail-on-alert') { Pass "memmon fail-on-alert" } else { Fail "memmon fail-on-alert: stderr did not mention fail-on-alert. stderr=$err" }
}

# Summary
if ($all_ok) { Write-Host "ALL TESTS PASSED" -ForegroundColor Green; exit 0 } else { Write-Host "SOME TESTS FAILED" -ForegroundColor Yellow; exit 1 }
