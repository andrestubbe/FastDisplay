$lines = Get-Content repo.jsonl
foreach ($l in $lines) {
    if ($l -match '"native/FastDesktop.cpp"') {
        $obj = $l | ConvertFrom-Json
        [IO.File]::WriteAllText("old_FastDesktop.cpp", $obj.content)
        break
    }
}
