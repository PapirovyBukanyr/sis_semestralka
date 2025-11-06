param(
    [int]$Rate = 5,            # packets per second
    [int]$Count = 0,           # 0 = infinite
    [string]$DestHost = "127.0.0.1",
    [int]$Port = 9000
)

# Simple synthetic sequence trainer for the analyzer NN.
# Sends JSON lines containing timestamp, export_bytes and export_flows.
# The NN's preproc normalizes export_bytes -> in0 (bytes/1e6) and export_flows -> in1 (flows/100).

# Define a repeating sequence of normalized feature pairs (in0,in1) in [0,1]
$pattern = @(
    @{ in0 = 0.10; in1 = 0.20 },
    @{ in0 = 0.70; in1 = 0.30 },
    @{ in0 = 0.40; in1 = 0.90 }
)

$udp = New-Object System.Net.Sockets.UdpClient
 $endpoint = New-Object System.Net.IPEndPoint ([System.Net.IPAddress]::Parse($DestHost), $Port)

$idx = 0
$sent = 0
$intervalMs = [int](1000 / [double]$Rate)
if($intervalMs -lt 1){ $intervalMs = 1 }
Write-Host "Starting synthetic sender -> $DestHost`:$Port (rate=$Rate pkt/s). Press Ctrl+C to stop."

try {
    while($Count -eq 0 -or $sent -lt $Count){
        $v = $pattern[$idx]
        # convert normalized features back to measurable units used by preproc
        $export_bytes = [math]::Round($v.in0 * 1e6)
        $export_flows = [math]::Round($v.in1 * 100)
        $ts = [int][double](Get-Date -UFormat %s)
        $obj = @{ timestamp = $ts; export_bytes = $export_bytes; export_flows = $export_flows; export_packets = 0; export_rtr = 0; export_rtt = 0; export_srt = 0 }
        $json = $obj | ConvertTo-Json -Compress
        $bytes = [System.Text.Encoding]::UTF8.GetBytes($json)
    $udp.Send($bytes, $bytes.Length, $endpoint) | Out-Null
    Write-Host "sent: $json"

        $sent++
        $idx = ($idx + 1) % $pattern.Count
        Start-Sleep -Milliseconds $intervalMs
    }
} finally {
    $udp.Close()
}

Write-Host "Finished. Sent $sent packets."