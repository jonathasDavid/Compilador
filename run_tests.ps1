# Run parser size tests and summarize results
$tests = @(
    @{ exe = "tests\\t01.exe"; name = "t01_texto_happy"; expectCode = 0; expectStderrContains = "" },
    @{ exe = "tests\\t02.exe"; name = "t02_texto_overflow"; expectCode = 0; expectStderrContains = "ALERTA SEMANTICO" },
    @{ exe = "tests\\t03.exe"; name = "t03_texto_missing_bracket"; expectCode = 1; expectStderrContains = "ERRO SINTATICO" }
    ,@{ exe = "tests\\t04.exe"; name = "t04_return_overflow"; expectCode = 0; expectStderrContains = "ALERTA SEMANTICO" }
    ,@{ exe = "tests\\t05.exe"; name = "t05_escreva_literal_overflow"; expectCode = 0; expectStderrContains = "ALERTA SEMANTICO" }
    ,@{ exe = "tests\\t06.exe"; name = "t06_escreva_literal_vs_var"; expectCode = 0; expectStderrContains = "ALERTA SEMANTICO" }
    ,@{ exe = "tests\\t07.exe"; name = "t07_a3_assignment_then_escreva"; expectCode = 0; expectStderrContains = "ALERTA SEMANTICO" }
    ,@{ exe = "tests\\t_memmon_alerts.exe"; name = "t_memmon_alerts"; expectCode = 1; expectStderrContains = "Memoria Insuficiente" }
    ,@{ exe = "tests\\t08.exe"; name = "t08_decimal_ok"; expectCode = 0; expectStderrContains = "" }
    ,@{ exe = "tests\\t09.exe"; name = "t09_decimal_overflow"; expectCode = 0; expectStderrContains = "ALERTA SEMANTICO" }
    ,@{ exe = "tests\\t10.exe"; name = "t10_decimal_assign_var"; expectCode = 0; expectStderrContains = "ALERTA SEMANTICO" }
    ,@{ exe = "tests\\t11.exe"; name = "t11_return_decimal_overflow"; expectCode = 0; expectStderrContains = "ALERTA SEMANTICO" }
    ,@{ exe = "tests\\t12.exe"; name = "t12_escreva_decimal_vs_var"; expectCode = 0; expectStderrContains = "ALERTA SEMANTICO" }
    ,@{ exe = "tests\\t_symtab_unit.exe"; name = "t_symtab_unit"; expectCode = 0; expectStderrContains = "" }
    ,@{ exe = "tests\\t_principal_missing.exe"; name = "t_principal_missing"; expectCode = 1; expectStderrContains = "Modulo Principal Inexistente" }
)

$results = @()
foreach ($t in $tests) {
    $exePath = Join-Path $PSScriptRoot $t.exe
    Write-Host "Running $($t.name) -> $exePath"
    if (-not (Test-Path $exePath)) { Write-Host "  SKIP: executable not found: $exePath"; $results += @{ name=$t.name; status='SKIP'; code=-1; out=''; err='not found' }; continue }
    $processInfo = New-Object System.Diagnostics.ProcessStartInfo
    $processInfo.FileName = $exePath
    $processInfo.RedirectStandardOutput = $true
    $processInfo.RedirectStandardError = $true
    $processInfo.UseShellExecute = $false
    $p = New-Object System.Diagnostics.Process
    $p.StartInfo = $processInfo
    $p.Start() | Out-Null
    $out = $p.StandardOutput.ReadToEnd()
    $err = $p.StandardError.ReadToEnd()
    $p.WaitForExit()
    $code = $p.ExitCode
    Write-Host "  exit=$code"
    if ($out) { Write-Host "  stdout:`n$out" }
    if ($err) { Write-Host "  stderr:`n$err" }
    $pass = $true
    if ($code -ne $t.expectCode) { $pass = $false }
    if ($t.expectStderrContains -ne "" -and ($err -notlike "*" + $t.expectStderrContains + "*")) { $pass = $false }
    if ($pass) { $status = 'PASS' } else { $status = 'FAIL' }
    $results += @{ name=$t.name; status=$status; code=$code; out=$out; err=$err }
}

Write-Host "\nSummary:"
foreach ($r in $results) { Write-Host "$($r.name): $($r.status) (exit $($r.code))" }

# exit with non-zero if any FAIL
if ($results | Where-Object { $_.status -eq 'FAIL' }) { exit 1 } else { exit 0 }
