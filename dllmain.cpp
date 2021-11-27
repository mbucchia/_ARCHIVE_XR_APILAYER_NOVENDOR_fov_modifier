// SPDX-License-Identifier: Apache-2.0
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

// This OpenXR layer demonstrates how to intercept the OpenXR calls to xrLocateViews() in order to alter the FOV based on per-application settings.

#include "pch.h"

namespace {
    const std::string LayerName = "XR_APILAYER_NOVENDOR_fov_modifier";

    // The path where the DLL loads config files and stores logs.
    std::string dllHome;

    // The file logger.
    std::ofstream logStream;

    // Function pointers to interact with the next layers and/or the OpenXR runtime.
    PFN_xrGetInstanceProcAddr nextXrGetInstanceProcAddr = nullptr;
    PFN_xrLocateViews nextXrLocateViews = nullptr;

    void Log(const char* fmt, ...);

    struct {
        bool loaded;

        float leftAngleUp;
        float leftAngleDown;
        float leftAngleLeft;
        float leftAngleRight;
        float rightAngleUp;
        float rightAngleDown;
        float rightAngleLeft;
        float rightAngleRight;
        
        void Dump()
        {
            if (loaded)
            {
                Log("Using FOV for left %.3f %.3f %.3f %.3f and right %.3f %.3f %.3f %.3f\n",
                    leftAngleUp, leftAngleDown, leftAngleLeft, leftAngleRight,
                    rightAngleUp, rightAngleDown, rightAngleLeft, rightAngleRight);
            }
        }

        void Reset()
        {
            loaded = false;
            leftAngleUp = 1.0f;
            leftAngleDown = 1.0f;
            leftAngleLeft = 1.0f;
            leftAngleRight = 1.0f;
            rightAngleUp = 1.0f;
            rightAngleDown = 1.0f;
            rightAngleLeft = 1.0f;
            rightAngleRight = 1.0f;
        }
    } config;

    // Utility logging function.
    void InternalLog(const char* fmt, va_list va)
    {
        char buf[1024];
        _vsnprintf_s(buf, sizeof(buf), fmt, va);
        OutputDebugStringA(buf);
        if (logStream.is_open())
        {
            logStream << buf;
            logStream.flush();
        }
    }

    // General logging function.
    void Log(const char* fmt, ...)
    {
        va_list va;
        va_start(va, fmt);
        InternalLog(fmt, va);
        va_end(va);
    }

    // Debug logging function. Can make things very slow (only enabled on Debug builds).
    void DebugLog(const char* fmt, ...)
    {
#ifdef _DEBUG
        va_list va;
        va_start(va, fmt);
        InternalLog(fmt, va);
        va_end(va);
#endif
    }

    // Load configuration for our layer.
    bool LoadConfiguration(const std::string configName)
    {
        if (configName.empty())
        {
            return false;
        }

        std::ifstream configFile(std::filesystem::path(dllHome) / std::filesystem::path(configName + ".cfg"));
        if (configFile.is_open())
        {
            Log("Loading config for \"%s\"\n", configName.c_str());

            unsigned int lineNumber = 0;
            std::string line;
            while (std::getline(configFile, line))
            {
                lineNumber++;
                try
                {
                    // TODO: Handle comments, white spaces, blank lines...
                    const auto offset = line.find('=');
                    if (offset != std::string::npos)
                    {
                        const std::string name = line.substr(0, offset);
                        const float value = std::stof(line.substr(offset + 1));

                        if (name == "left.up")
                        {
                            config.leftAngleUp = value;
                        }
                        else if (name == "left.down")
                        {
                            config.leftAngleDown = value;
                        }
                        else if (name == "left.left")
                        {
                            config.leftAngleLeft = value;
                        }
                        else if (name == "left.right")
                        {
                            config.leftAngleRight = value;
                        }
                        else if (name == "right.up")
                        {
                            config.rightAngleUp = value;
                        }
                        else if (name == "right.down")
                        {
                            config.rightAngleDown = value;
                        }
                        else if (name == "right.left")
                        {
                            config.rightAngleLeft = value;
                        }
                        else if (name == "right.right")
                        {
                            config.rightAngleRight = value;
                        }
                    }
                }
                catch (...)
                {
                    Log("Error parsing L%u\n", lineNumber);
                }
            }
            configFile.close();

            config.loaded = true;

            return true;
        }

        Log("Could not load config for \"%s\"\n", configName.c_str());

        return false;
    }

    // Overrides the behavior of xrLocateViews().
    XrResult FOVModifier_xrLocateViews(
        const XrSession session,
        const XrViewLocateInfo* const viewLocateInfo,
        XrViewState* const viewState,
        const uint32_t viewCapacityInput,
        uint32_t* const viewCountOutput,
        XrView* const views)
    {
        DebugLog("--> FOVModifier_xrLocateViews\n");

        // Call the chain to perform the actual operation.
        const XrResult result = nextXrLocateViews(session, viewLocateInfo, viewState, viewCapacityInput, viewCountOutput, views);
        if (result == XR_SUCCESS)
        {
            // Apply our logic to modify the FOV.
            if (viewLocateInfo->viewConfigurationType == XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO)
            {
                views[0].fov.angleDown *= config.leftAngleDown;
                views[0].fov.angleUp *= config.leftAngleUp;
                views[0].fov.angleLeft *= config.leftAngleLeft;
                views[0].fov.angleRight *= config.leftAngleRight;
                views[1].fov.angleDown *= config.rightAngleDown;
                views[1].fov.angleUp *= config.rightAngleUp;
                views[1].fov.angleLeft *= config.rightAngleLeft;
                views[1].fov.angleRight *= config.rightAngleRight;
            }
        }

        DebugLog("<-- FOVModifier_xrLocateViews %d\n", result);

        return result;
    }

    // Entry point for OpenXR calls.
    XrResult FOVModifier_xrGetInstanceProcAddr(
        const XrInstance instance,
        const char* const name,
        PFN_xrVoidFunction* const function)
    {
        DebugLog("--> FOVModifier_xrGetInstanceProcAddr \"%s\"\n", name);

        // Call the chain to resolve the next function pointer.
        const XrResult result = nextXrGetInstanceProcAddr(instance, name, function);
        if (config.loaded && result == XR_SUCCESS)
        {
            const std::string apiName(name);

            // Intercept the calls handled by our layer.
            if (apiName == "xrLocateViews") {
                nextXrLocateViews = reinterpret_cast<PFN_xrLocateViews>(*function);
                *function = reinterpret_cast<PFN_xrVoidFunction>(FOVModifier_xrLocateViews);
            }

            // Leave all unhandled calls to the next layer.
        }

        DebugLog("<-- FOVModifier_xrGetInstanceProcAddr %d\n", result);

        return result;
    }

    // Entry point for creating the layer.
    XrResult FOVModifier_xrCreateApiLayerInstance(
        const XrInstanceCreateInfo* const instanceCreateInfo,
        const struct XrApiLayerCreateInfo* const apiLayerInfo,
        XrInstance* const instance)
    {
        DebugLog("--> FOVModifier_xrCreateApiLayerInstance\n");

        if (!apiLayerInfo ||
            apiLayerInfo->structType != XR_LOADER_INTERFACE_STRUCT_API_LAYER_CREATE_INFO ||
            apiLayerInfo->structVersion != XR_API_LAYER_CREATE_INFO_STRUCT_VERSION ||
            apiLayerInfo->structSize != sizeof(XrApiLayerCreateInfo) ||
            !apiLayerInfo->nextInfo ||
            apiLayerInfo->nextInfo->structType != XR_LOADER_INTERFACE_STRUCT_API_LAYER_NEXT_INFO ||
            apiLayerInfo->nextInfo->structVersion != XR_API_LAYER_NEXT_INFO_STRUCT_VERSION ||
            apiLayerInfo->nextInfo->structSize != sizeof(XrApiLayerNextInfo) ||
            apiLayerInfo->nextInfo->layerName != LayerName ||
            !apiLayerInfo->nextInfo->nextGetInstanceProcAddr ||
            !apiLayerInfo->nextInfo->nextCreateApiLayerInstance)
        {
            Log("xrCreateApiLayerInstance validation failed\n");
            return XR_ERROR_INITIALIZATION_FAILED;
        }

        // Store the next xrGetInstanceProcAddr to resolve the functions no handled by our layer.
        nextXrGetInstanceProcAddr = apiLayerInfo->nextInfo->nextGetInstanceProcAddr;

        // Call the chain to create the instance.
        const XrResult result = apiLayerInfo->nextInfo->nextCreateApiLayerInstance(instanceCreateInfo, apiLayerInfo, instance);
        if (result == XR_SUCCESS)
        {
            // Identify the application and load our configuration. Try by application first, then fallback to engines otherwise.
            config.Reset();
            if (!LoadConfiguration(instanceCreateInfo->applicationInfo.applicationName)) {
                LoadConfiguration(instanceCreateInfo->applicationInfo.engineName);
            }
            config.Dump();
        }

        DebugLog("<-- FOVModifier_xrCreateApiLayerInstance %d\n", result);

        return result;
    }

}

extern "C" {

    // Entry point for the loader.
    XrResult __declspec(dllexport) XRAPI_CALL FOVModifier_xrNegotiateLoaderApiLayerInterface(
        const XrNegotiateLoaderInfo* const loaderInfo,
        const char* const apiLayerName,
        XrNegotiateApiLayerRequest* const apiLayerRequest)
    {
        DebugLog("--> (early) FOVModifier_xrNegotiateLoaderApiLayerInterface\n");

        // Retrieve the path of the DLL.
        if (dllHome.empty())
        {
            HMODULE module;
            if (GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, (LPCSTR)&dllHome, &module))
            {
                char path[_MAX_PATH];
                GetModuleFileNameA(module, path, sizeof(path));
                dllHome = std::filesystem::path(path).parent_path().string();
            }
            else
            {
                // Falling back to loading config/writing logs to the current working directory.
                DebugLog("Failed to locate DLL\n");
            }            
        }

        // Start logging to file.
        if (!logStream.is_open())
        {
            std::string logFile = (std::filesystem::path(getenv("LOCALAPPDATA")) / std::filesystem::path(LayerName + ".log")).string();
            logStream.open(logFile, std::ios_base::ate);
            Log("dllHome is \"%s\"\n", dllHome.c_str());
        }

        DebugLog("--> FOVModifier_xrNegotiateLoaderApiLayerInterface\n");

        if (apiLayerName && apiLayerName != LayerName)
        {
            Log("Invalid apiLayerName \"%s\"\n", apiLayerName);
            return XR_ERROR_INITIALIZATION_FAILED;
        }

        if (!loaderInfo ||
            !apiLayerRequest ||
            loaderInfo->structType != XR_LOADER_INTERFACE_STRUCT_LOADER_INFO ||
            loaderInfo->structVersion != XR_LOADER_INFO_STRUCT_VERSION ||
            loaderInfo->structSize != sizeof(XrNegotiateLoaderInfo) ||
            apiLayerRequest->structType != XR_LOADER_INTERFACE_STRUCT_API_LAYER_REQUEST ||
            apiLayerRequest->structVersion != XR_API_LAYER_INFO_STRUCT_VERSION ||
            apiLayerRequest->structSize != sizeof(XrNegotiateApiLayerRequest) ||
            loaderInfo->minInterfaceVersion > XR_CURRENT_LOADER_API_LAYER_VERSION ||
            loaderInfo->maxInterfaceVersion < XR_CURRENT_LOADER_API_LAYER_VERSION ||
            loaderInfo->maxInterfaceVersion > XR_CURRENT_LOADER_API_LAYER_VERSION ||
            loaderInfo->maxApiVersion < XR_CURRENT_API_VERSION ||
            loaderInfo->minApiVersion > XR_CURRENT_API_VERSION)
        {
            Log("xrNegotiateLoaderApiLayerInterface validation failed\n");
            return XR_ERROR_INITIALIZATION_FAILED;
        }

        // Setup our layer to intercept OpenXR calls.
        apiLayerRequest->layerInterfaceVersion = XR_CURRENT_LOADER_API_LAYER_VERSION;
        apiLayerRequest->layerApiVersion = XR_CURRENT_API_VERSION;
        apiLayerRequest->getInstanceProcAddr = reinterpret_cast<PFN_xrGetInstanceProcAddr>(FOVModifier_xrGetInstanceProcAddr);
        apiLayerRequest->createApiLayerInstance = reinterpret_cast<PFN_xrCreateApiLayerInstance>(FOVModifier_xrCreateApiLayerInstance);

        DebugLog("<-- FOVModifier_xrNegotiateLoaderApiLayerInterface\n");

        Log("%s layer is active\n", LayerName.c_str());

        return XR_SUCCESS;
    }

}
