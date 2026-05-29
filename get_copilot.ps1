$json = Get-Content C:\Users\andre\.gemini\antigravity\brain\6ce8eef0-6d3d-415e-9e5a-45a4acb476e9\.system_generated\logs\transcript.jsonl -Raw
$lines = $json -split "`n"
foreach ($l in $lines) {
    if ($l.Trim() -ne '') {
        $obj = $l | ConvertFrom-Json
        if ($obj.content -match 'Bing copilot') {
            [IO.File]::WriteAllText("copilot_msg.txt", $obj.content)
            break
        }
    }
}
