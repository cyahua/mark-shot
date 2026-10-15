<#
.SYNOPSIS
采样指定 Windows 进程的工作集和私有提交量，不启动、终止或回收目标进程
.PARAMETER ProcessId
已启动的 Mark Shot 或 Snow Shot 进程编号
.PARAMETER Seconds
采样持续时间，单位秒
.PARAMETER Scenario
场景标记，例如 idle、capture-4k、after-capture 或 after-recording
#>
[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)]
    [int]$ProcessId,
    [ValidateRange(1, 3600)]
    [int]$Seconds = 60,
    [string]$Scenario = "idle"
)

$ErrorActionPreference = "Stop"
$samples = @()
$process = Get-Process -Id $ProcessId -ErrorAction Stop
try {
    for ($index = 0; $index -lt $Seconds; $index++) {
        $process.Refresh()
        if ($process.HasExited) {
            throw "Process exited during measurement"
        }
        $samples += [pscustomobject]@{
            seconds = $index
            working_set_bytes = $process.WorkingSet64
            private_commit_bytes = $process.PrivateMemorySize64
            peak_working_set_bytes = $process.PeakWorkingSet64
            threads = $process.Threads.Count
            handles = $process.HandleCount
        }
        if ($index + 1 -lt $Seconds) {
            Start-Sleep -Seconds 1
        }
    }
    [pscustomobject]@{
        process_id = $ProcessId
        process_name = $process.ProcessName
        scenario = $Scenario
        platform = "windows"
        samples = $samples
    } | ConvertTo-Json -Depth 4
} finally {
    $process.Dispose()
}
