$gameProjectPath_wgcl = ".\wgcl\"
$gameProjectPath_ball = ".\ball\"
$gameProjectPath_putt = ".\putt\"
$gameProjectPath_share = ".\share\"

# ===================================================================================================================================================

# NORMAL LIMITS
#     Game Project: 80000
#     Game Project + WGCL Server: 120000
#
# HIGH LIMITS
#     Game Project: 1280000
#     Game Project + WGCL Server: 1920000

[int]$linecount_limit_normal = 80000
[int]$linecount_limit_high_g = 1280000 # Normal limit x 16

[int]$lineCount_wgcl_c_total   = 0
[int]$lineCount_wgcl_cpp_total = 0

[int]$lineCount_ball_c_total   = 0
[int]$lineCount_ball_cpp_total = 0

[int]$lineCount_putt_c_total   = 0
[int]$lineCount_putt_cpp_total = 0

[int]$lineCount_share_c_total   = 0
[int]$lineCount_share_cpp_total = 0

Get-ChildItem $gameProjectPath_wgcl -Filter ".\*.c" | ForEach-Object {
	$fileName_wgcl_c = $_
	$lineCount_wgcl_c_current = (Get-Content "wgcl\$fileName_wgcl_c" | Measure-Object -Line).Lines
	$lineCount_wgcl_c_total = $lineCount_wgcl_c_total + $lineCount_wgcl_c_current
}

Get-ChildItem $gameProjectPath_wgcl -Filter ".\*.cpp" | ForEach-Object {
	$fileName_wgcl_cpp = $_
	$lineCount_wgcl_cpp_current = (Get-Content "wgcl\$fileName_wgcl_cpp" | Measure-Object -Line).Lines
	$lineCount_wgcl_cpp_total = $lineCount_wgcl_cpp_total + $lineCount_wgcl_cpp_current
}

Get-ChildItem $gameProjectPath_ball -Filter ".\*.c" | ForEach-Object {
	$fileName_ball_c = $_
	$lineCount_ball_c_current = (Get-Content "ball\$fileName_ball_c" | Measure-Object -Line).Lines
	$lineCount_ball_c_total = $lineCount_ball_c_total + $lineCount_ball_c_current
}

Get-ChildItem $gameProjectPath_ball -Filter ".\*.cpp" | ForEach-Object {
	$fileName_ball_cpp = $_
	$lineCount_ball_cpp_current = (Get-Content "ball\$fileName_ball_cpp" | Measure-Object -Line).Lines
	$lineCount_ball_cpp_total = $lineCount_ball_cpp_total + $lineCount_ball_cpp_current
}

Get-ChildItem $gameProjectPath_putt -Filter ".\*.c" | ForEach-Object {
	$fileName_putt_c = $_
	$lineCount_putt_c_current = (Get-Content "putt\$fileName_putt_c" | Measure-Object -Line).Lines
	$lineCount_putt_c_total = $lineCount_putt_c_total + $lineCount_putt_c_current
}

Get-ChildItem $gameProjectPath_putt -Filter ".\*.cpp" | ForEach-Object {
	$fileName_putt_cpp = $_
	$lineCount_putt_cpp_current = (Get-Content "putt\$fileName_putt_cpp" | Measure-Object -Line).Lines
	$lineCount_putt_cpp_total = $lineCount_putt_cpp_total + $lineCount_putt_cpp_current
}

Get-ChildItem $gameProjectPath_share -Filter ".\*.c" | ForEach-Object {
	$fileName_share_c = $_
	$lineCount_share_c_current = (Get-Content "share\$fileName_share_c" | Measure-Object -Line).Lines
	$lineCount_share_c_total = $lineCount_share_c_total + $lineCount_share_c_current
}

Get-ChildItem $gameProjectPath_share -Filter ".\*.cpp" | ForEach-Object {
	$fileName_share_cpp = $_
	$lineCount_share_cpp_current = (Get-Content "share\$fileName_share_cpp" | Measure-Object -Line).Lines
	$lineCount_share_cpp_total = $lineCount_share_cpp_total + $lineCount_share_cpp_current
}

# ===================================================================================================================================================

Write-Host "===== Zusammenfassung für Neverball-Quellcode ====="
Write-Host ""

if (($lineCount_ball_c_total + $lineCount_wgcl_c_total + $lineCount_putt_c_total + $lineCount_share_c_total + $lineCount_ball_cpp_total + $lineCount_wgcl_cpp_total + $lineCount_putt_cpp_total + $lineCount_share_cpp_total) -gt $linecount_limit_high_g)
{
	Write-Host "(!) HOHE CODEZEILEN"
	Write-Host "    Sie können nich mehr als $linecount_limit_high_g zeilen in einem einziges Projekt."
	Write-Host ""
	exit 1
}

if (($lineCount_ball_c_total + $lineCount_wgcl_c_total + $lineCount_putt_c_total + $lineCount_share_c_total + $lineCount_ball_cpp_total + $lineCount_wgcl_cpp_total + $lineCount_putt_cpp_total + $lineCount_share_cpp_total) -gt $linecount_limit_normal)
{
	Write-Host "(!) HOHE CODEZEILEN"
	Write-Host "    Sie haben die gesamte maximale Grenze von $linecount_limit_normal Zeilen erreicht."
	Write-Host "    Sie können den Wert auf $linecount_limit_high_g erhöhen, aber Ihre Projekte können auf einigen alten Geräten instabil werden, daher wird empfohlen, unter diesem Grenzwert zu bleiben."
	Write-Host "    Spielprojekt, deren gesamte Zeilenzahl im Code über diesem Limit liegt, zeigen beim Herunterladen von PB+NB Game Core Launcher eine Warnung an."
	Write-Host ""
}

[int]$ball_final_total_c = $lineCount_ball_c_total + $lineCount_wgcl_c_total + $lineCount_share_c_total
[int]$ball_final_total_cpp = $lineCount_ball_cpp_total + $lineCount_wgcl_cpp_total + $lineCount_share_cpp_total

[int]$putt_final_total_c = $lineCount_putt_c_total + $lineCount_share_c_total
[int]$putt_final_total_cpp = $lineCount_putt_cpp_total + $lineCount_share_cpp_total

[int]$entire_final_total_c = $lineCount_ball_c_total + $lineCount_wgcl_c_total + $lineCount_putt_c_total + $lineCount_share_c_total
[int]$entire_final_total_cpp = $lineCount_ball_cpp_total + $lineCount_wgcl_cpp_total + $lineCount_putt_cpp_total + $lineCount_share_cpp_total
[int]$entire_final_total = $entire_final_total_c + $entire_final_total_cpp

Write-Host "- Pennyball + Neverball WGCL"
Write-Host "  C-Codezeilen (Aktuell / Gesamt): $lineCount_wgcl_c_total / $ball_final_total_c"
Write-Host "  C++-Codezeilen (Aktuell / Gesamt): $lineCount_wgcl_cpp_total / $ball_final_total_cpp"
Write-Host ""
Write-Host "- Neverball"
Write-Host "  C-Codezeilen (Aktuell / Gesamt): $lineCount_ball_c_total / $ball_final_total_c"
Write-Host "  C++-Codezeilen (Aktuell / Gesamt): $lineCount_ball_cpp_total / $ball_final_total_cpp"
Write-Host ""
Write-Host "- Neverputt"
Write-Host "  C-Codezeilen (Aktuell / Gesamt): $lineCount_putt_c_total / $putt_final_total_c"
Write-Host "  C++-Codezeilen (Aktuell / Gesamt): $lineCount_putt_cpp_total / $putt_final_total_cpp"
Write-Host ""
Write-Host "- Shared"
Write-Host "  C-Codezeilen (Aktuell): $lineCount_share_c_total"
Write-Host "  C++-Codezeilen (Aktuell): $lineCount_share_cpp_total"
Write-Host ""
Write-Host "- Ganzes Projekt"
Write-Host "  C-Codezeilen (Aktuell): $entire_final_total_c"
Write-Host "  C++-Codezeilen (Aktuell): $entire_final_total_cpp"
Write-Host "  Codezeilen (Insgesamt) (Aktuell): $entire_final_total"
Write-Host ""