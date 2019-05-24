/*
 ***********************************************************************************************************************
 *
 *  Copyright (c) 2018-2019 Advanced Micro Devices, Inc. All Rights Reserved.
 *
 *  Permission is hereby granted, free of charge, to any person obtaining a copy
 *  of this software and associated documentation files (the "Software"), to deal
 *  in the Software without restriction, including without limitation the rights
 *  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 *  copies of the Software, and to permit persons to whom the Software is
 *  furnished to do so, subject to the following conditions:
 *
 *  The above copyright notice and this permission notice shall be included in all
 *  copies or substantial portions of the Software.
 *
 *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 *  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 *  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 *  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 *  SOFTWARE.
 *
 **********************************************************************************************************************/
/**
 ***********************************************************************************************************************
 * @file  llpcUtil.cpp
 * @brief LLPC source file: contains implementation of LLPC internal types and utility functions
 * (independent of LLVM use).
 ***********************************************************************************************************************
 */
#define DEBUG_TYPE "llpc-util"

#include "llpc.h"
#include "llpcDebug.h"
#include "llpcElfReader.h"
#include "llpcUtil.h"
#include "palPipelineAbi.h"

#if defined(_WIN32)
#include <direct.h>
#else
#include <sys/types.h>
#include <sys/stat.h>
#endif

namespace Llpc
{

// =====================================================================================================================
// Gets the name string of shader stage.
const char* GetShaderStageName(
    ShaderStage shaderStage)  // Shader stage
{
    const char* pName = nullptr;

    if (shaderStage == ShaderStageCopyShader)
    {
        pName = "copy";
    }
    else if (shaderStage < ShaderStageCount)
    {
        static const char* ShaderStageNames[] =
        {
            "vertex",
            "tessellation control",
            "tessellation evaluation",
            "geometry",
            "fragment",
            "compute",
        };

        pName = ShaderStageNames[static_cast<uint32_t>(shaderStage)];
    }
    else
    {
        pName = "bad";
    }

    return pName;
}

// =====================================================================================================================
// Gets name string of the abbreviation for the specified shader stage
const char* GetShaderStageAbbreviation(
    ShaderStage shaderStage,  // Shader stage
    bool        upper)        // Whether to use uppercase for the abbreviation (default is lowercase)
{
    const char* pAbbr = nullptr;

    if (shaderStage == ShaderStageCopyShader)
    {
        pAbbr = upper ? "COPY" : "Copy";
    }
    else if (shaderStage < ShaderStageCount)
    {
        if (upper)
        {
            static const char* ShaderStageAbbrs[] =
            {
                "VS",
                "TCS",
                "TES",
                "GS",
                "FS",
                "CS",
            };

            pAbbr = ShaderStageAbbrs[static_cast<uint32_t>(shaderStage)];
        }
        else
        {
            static const char* ShaderStageAbbrs[] =
            {
                "Vs",
                "Tcs",
                "Tes",
                "Gs",
                "Fs",
                "Cs",
            };

            pAbbr = ShaderStageAbbrs[static_cast<uint32_t>(shaderStage)];
        }
    }
    else
    {
        pAbbr = "Bad";
    }

    return pAbbr;
}

// =====================================================================================================================
// Translates shader stage to corresponding stage mask.
uint32_t ShaderStageToMask(
    ShaderStage stage)  // Shader stage
{
    LLPC_ASSERT((stage < ShaderStageCount) || (stage == ShaderStageCopyShader));
    return (1 << stage);
}

// =====================================================================================================================
// Create directory.
bool CreateDirectory(
    const char* pDir)  // [in] the path of directory
{
#if defined(_WIN32)
    int result = _mkdir(pDir);
    return (result == 0);
#else
    int result = mkdir(pDir, S_IRWXU);
    return (result == 0);
#endif
}

} // Llpc
