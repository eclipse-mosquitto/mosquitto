param(
    [Parameter(Mandatory = $true)]
    [string]$SourceRoot,

    [Parameter(Mandatory = $true)]
    [string]$StagingDir,

    [switch]$ExcludeDev
)

function Copy-StagedFiles {
    param(
        [Parameter(Mandatory = $true)]
        [string]$Destination,

        [Parameter(Mandatory = $true)]
        [string[]]$Files
    )

    $targetDir = if ([string]::IsNullOrEmpty($Destination)) {
        $StagingDir
    } else {
        Join-Path $StagingDir $Destination
    }

    New-Item -ItemType Directory -Path $targetDir -Force | Out-Null

    foreach ($file in $Files) {
        $sourcePath = Join-Path $SourceRoot $file
        if (-not (Test-Path -LiteralPath $sourcePath)) {
            throw "Missing staged file: $file"
        }

        Copy-Item -LiteralPath $sourcePath -Destination $targetDir -Force
    }
}

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

if (Test-Path -LiteralPath $StagingDir) {
    Remove-Item -LiteralPath $StagingDir -Recurse -Force
}

New-Item -ItemType Directory -Path $StagingDir -Force | Out-Null

Copy-StagedFiles '.' @(
    'logo/mosquitto.ico',
    'build64/src/Release/mosquitto.exe',
    'build64/apps/db_dump/Release/mosquitto_db_dump.exe',
    'build64/apps/mosquitto_ctrl/Release/mosquitto_ctrl.exe',
    'build64/apps/mosquitto_passwd/Release/mosquitto_passwd.exe',
    'build64/apps/mosquitto_signal/Release/mosquitto_signal.exe',
    'build64/client/Release/mosquitto_pub.exe',
    'build64/client/Release/mosquitto_sub.exe',
    'build64/client/Release/mosquitto_rr.exe',
    'build64/libcommon/Release/mosquitto_common.dll',
    'build64/lib/Release/mosquitto.dll',
    'build64/lib/cpp/Release/mosquittopp.dll',
    'build64/plugins/acl-file/Release/mosquitto_acl_file.dll',
    'build64/plugins/dynamic-security/Release/mosquitto_dynamic_security.dll',
    'build64/plugins/password-file/Release/mosquitto_password_file.dll',
    'build64/plugins/persist-sqlite/Release/mosquitto_persist_sqlite.dll',
    'build64/plugins/sparkplug-aware/Release/mosquitto_sparkplug_aware.dll',
    # 'aclfile.example',
    'ChangeLog.txt',
    'NOTICE.md',
    # 'pskfile.example',
    # 'pwfile.example',
    # 'README.md',
    # 'README-windows.txt',
    # 'README-letsencrypt.md',
    # 'SECURITY.md',
    'edl-v10',
    'epl-v20',
    'mosquitto.conf',
    'build64/vcpkg_installed/x64-windows-release/bin/cjson.dll',
    'build64/vcpkg_installed/x64-windows-release/bin/libcrypto-3-x64.dll',
    'build64/vcpkg_installed/x64-windows-release/bin/libmicrohttpd-dll.dll',
    'build64/vcpkg_installed/x64-windows-release/bin/libssl-3-x64.dll',
    'build64/vcpkg_installed/x64-windows-release/bin/pthreadVC3.dll',
    'build64/vcpkg_installed/x64-windows-release/bin/sqlite3.dll'
)

if (-not $ExcludeDev) {
    Copy-StagedFiles 'devel' @(
        'build64/lib/Release/mosquitto.lib',
        'build64/lib/cpp/Release/mosquittopp.lib',
        'build64/src/Release/mosquitto_broker.lib',
        'include/mosquitto.h',
        'include/mosquitto_broker.h',
        'include/mosquitto_plugin.h',
        'include/mosquittopp.h',
        'include/mqtt_protocol.h'
    )

    Copy-StagedFiles 'devel/mosquitto' @(
        'include/mosquitto/broker.h',
        'include/mosquitto/broker_control.h',
        'include/mosquitto/broker_plugin.h',
        'include/mosquitto/defs.h',
        'include/mosquitto/libcommon.h',
        'include/mosquitto/libcommon_base64.h',
        'include/mosquitto/libcommon_cjson.h',
        'include/mosquitto/libcommon_file.h',
        'include/mosquitto/libcommon_memory.h',
        'include/mosquitto/libcommon_password.h',
        'include/mosquitto/libcommon_properties.h',
        'include/mosquitto/libcommon_random.h',
        'include/mosquitto/libcommon_string.h',
        'include/mosquitto/libcommon_time.h',
        'include/mosquitto/libcommon_topic.h',
        'include/mosquitto/libcommon_utf8.h',
        'include/mosquitto/libmosquitto.h',
        'include/mosquitto/libmosquitto_auth.h',
        'include/mosquitto/libmosquitto_callbacks.h',
        'include/mosquitto/libmosquitto_connect.h',
        'include/mosquitto/libmosquitto_create_delete.h',
        'include/mosquitto/libmosquitto_helpers.h',
        'include/mosquitto/libmosquitto_loop.h',
        'include/mosquitto/libmosquitto_message.h',
        'include/mosquitto/libmosquitto_options.h',
        'include/mosquitto/libmosquitto_publish.h',
        'include/mosquitto/libmosquitto_socks.h',
        'include/mosquitto/libmosquitto_subscribe.h',
        'include/mosquitto/libmosquitto_tls.h',
        'include/mosquitto/libmosquitto_unsubscribe.h',
        'include/mosquitto/libmosquitto_will.h',
        'include/mosquitto/libmosquittopp.h',
        'include/mosquitto/mqtt_protocol.h'
    )
}

Copy-StagedFiles 'dashboard' @(
    'dashboard/src/index.html',
    'dashboard/src/listeners.html'
)

Copy-StagedFiles 'dashboard/app' @(
    'dashboard/src/app/consts.js',
    'dashboard/src/app/dashboard.js',
    'dashboard/src/app/index.js',
    'dashboard/src/app/listeners.js',
    'dashboard/src/app/sidebar.js'
)

Copy-StagedFiles 'dashboard/css' @(
    'dashboard/src/css/styles.css'
)

Copy-StagedFiles 'dashboard/lib' @(
    'dashboard/src/lib/chart.umd.js',
    'dashboard/src/lib/chartjs-plugin-zoom.min.js',
    'dashboard/src/lib/hammer.min.js'
)

Copy-StagedFiles 'dashboard/media' @(
    'dashboard/src/media/banner.svg',
    'dashboard/src/media/favicon-16x16.png',
    'dashboard/src/media/favicon-32x32.png',
    'dashboard/src/media/mosquitto-logo.png'
)

Copy-StagedFiles 'dashboard/tailwind' @(
    'dashboard/src/tailwind.config.js',
    'dashboard/src/tailwind/styles.css'
)

Copy-StagedFiles 'dashboard/utils' @(
    'dashboard/src/utils/assert.js',
    'dashboard/src/utils/queue.js',
    'dashboard/src/utils/utils.js'
)