function Get-CerfVsToolchain {
    $vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
    $toolchain = [pscustomobject]@{
        Vswhere         = $vswhere
        MinToolsVersion = [version]'14.30'
        Instance        = $null
        ToolsVersion    = $null
        PlatformToolset = $null
        Installed       = @()
        MSBuild         = $null
        CMake           = $null
    }
    if (-not (Test-Path $vswhere)) { return $toolchain }

    $instances = (& $vswhere -all -prerelease -requires Microsoft.Component.MSBuild -format json |
        ConvertFrom-Json) | Sort-Object { [version]$_.installationVersion } -Descending
    foreach ($candidate in $instances) {
        $toolsRoot = Join-Path $candidate.installationPath 'VC\Tools\MSVC'
        if (-not (Test-Path $toolsRoot)) { continue }
        $names = @(Get-ChildItem $toolsRoot -Directory | Select-Object -ExpandProperty Name)
        $toolchain.Installed += "$($candidate.displayName): $($names -join ', ')"
        $pick = $names |
            Where-Object { $_ -notlike '14.51.*' -and [version]$_ -ge $toolchain.MinToolsVersion } |
            Sort-Object { [version]$_ } | Select-Object -First 1
        if ($pick) {
            $toolchain.Instance     = $candidate
            $toolchain.ToolsVersion = $pick
            break
        }
    }
    if (-not $toolchain.ToolsVersion) { return $toolchain }

    # CMake cmGlobalVisualStudio10Generator.cxx SetGeneratorToolset:
    # "The v143 toolset reserves versions 14.30 through 14.49."
    $toolchain.PlatformToolset = 'v' + ($toolchain.ToolsVersion -replace '^(\d\d)\.(\d).*$', '$1$2')
    if ($toolchain.PlatformToolset -eq 'v144') { $toolchain.PlatformToolset = 'v143' }

    $root = $toolchain.Instance.installationPath
    $toolchain.MSBuild = & $vswhere -path $root -find 'MSBuild\**\Bin\amd64\MSBuild.exe' |
        Select-Object -First 1
    $toolchain.CMake = Join-Path $root 'Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe'
    return $toolchain
}
