$JsonPath = Join-Path "$PSScriptRoot" "XR_APILAYER_NOVENDOR_fov_modifier.json"
reg ADD "HKLM\Software\Khronos\OpenXR\1\ApiLayers\Implicit" /v $JsonPath /f /t REG_DWORD /d 0
