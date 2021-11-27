$JsonPath = Join-Path "$PSScriptRoot" "XR_APILAYER_NOVENDOR_fov_modifier.json"
Start-Process -FilePath reg.exe -Verb RunAs -Wait -ArgumentList "DELETE HKLM\Software\Khronos\OpenXR\1\ApiLayers\Implicit /v `"$JsonPath`" /f"
