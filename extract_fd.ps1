$lines = Get-Content repo.jsonl
foreach ($l in $lines) {
    if ($l -match '"native/FastDisplay.cpp"') {
        $obj = $l | ConvertFrom-Json
        [IO.File]::WriteAllText("old_FastDisplay.cpp", $obj.content)
        break
    }
}
