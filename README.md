# FOV modifier OpenXR API Layer

## Download

A ZIP file containing the necessary files to install and use the layer can be found on the release page: https://github.com/mbucchia/XR_APILAYER_NOVENDOR_fov_modifier/releases.

## Setup

1. Create a folder in `%ProgramFiles%`. It's important to make it in `%ProgramFiles%` so that UWP applications can access it! For example: `C:\Program Files\OpenXR-API-Layers`.

2. Place `XR_APILAYER_NOVENDOR_fov_modifier.json`, `XR_APILAYER_NOVENDOR_fov_modifier.dll`, `Install-Layer.ps1` and `Uninstall-Layer.ps1` in the folder created above.

3. Run the script `Install-Layer.ps1` **as Administrator**.

4. Start the OpenXR Developer Tools for Windows Mixed Reality, under the *System Status* tab, scroll down to *API Layers*. A layer named `XR_APILAYER_NOVENDOR_fov_modifier` should be listed.

## Removal

1. Go to the folder where the API layer is installed. For example: `C:\Program Files\OpenXR-API-Layers`.

2. Run the script `Uninstall-Layer.ps1` **as Administrator**.

3. Start the OpenXR Developer Tools for Windows Mixed Reality, under the *System Status* tab, scroll down to *API Layers*. There should be no layer named `XR_APILAYER_NOVENDOR_fov_modifier`.

## App configuration

1. First, retrieve the name that the application passes to OpenXR. In order to do that, run the application while the API layer is enabled.

2. Locate the log file for the layer. It will typically be `%LocalAppData%\XR_APILAYER_NOVENDOR_fov_modifier.log`.

3. In the log file, search for the first line saying "Could not load config for ...":

```
dllHome is "C:\Program Files\OpenXR-API-Layers"
XR_APILAYER_NOVENDOR_fov_modifier layer is active
Could not load config for "FS2020"
Could not load config for "Zouna"
Using FOV for left 1.000 1.000 1.000 1.000 and right 1.000 1.000 1.000 
```

4. In the same folder where `XR_APILAYER_NOVENDOR_fov_modifier.json` was copied during setup, create a file named after the application, and with the extension `.cfg`. For example `C:\Program Files\OpenXR-API-Layers\FS2020.cfg`.

In this file, set the factor for each FOV angle that needs to be changed. The factor is equal to the percentage divided by 100. This means that 0.5 is 50%, and 1 is 100%.

```
left.up=0.95
left.down=0.95
left.right=0.9
right.up=0.95
right.down=0.95
right.left=0.9
```

In the example above, `left.left` and `right.right` were left out and will assume their default value of 1 (100%).

5. When running the application, the changes should take affect. Inspect the log file if it needs to be confirmed:

```
dllHome is "C:\Program Files\OpenXR-API-Layers"
XR_APILAYER_NOVENDOR_fov_modifier layer is active
Loading config for "FS2020"
Using FOV for left 0.950 0.950 1.000 0.900 and right 0.950 0.950 0.900 1.000
```
