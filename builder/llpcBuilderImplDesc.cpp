/*
 ***********************************************************************************************************************
 *
 *  Copyright (c) 2019 Advanced Micro Devices, Inc. All Rights Reserved.
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
 * @file  llpcBuilderImplDesc.cpp
 * @brief LLPC source file: implementation of Builder methods for descriptor loads
 ***********************************************************************************************************************
 */
#include "llpcBuilderImpl.h"
#include "llpcContext.h"
#include "llpcInternal.h"

#include "llvm/IR/Intrinsics.h"

#define DEBUG_TYPE "llpc-builder-impl-desc"

using namespace Llpc;
using namespace llvm;

// =====================================================================================================================
// Create a load of a buffer descriptor.
Value* BuilderImplDesc::CreateLoadBufferDesc(
    uint32_t      descSet,          // Descriptor set
    uint32_t      binding,          // Descriptor binding
    Value*        pDescIndex,       // [in] Descriptor index
    bool          isNonUniform,     // Whether the descriptor index is non-uniform
    Type* const   pPointeeTy,       // [in] Type that the returned pointer should point to.
    const Twine&  instName)         // [in] Name to give instruction(s)
{
    LLPC_ASSERT(pPointeeTy != nullptr);

    Instruction* const pInsertPos = &*GetInsertPoint();
    pDescIndex = ScalarizeIfUniform(pDescIndex, isNonUniform);

    // TODO: This currently creates a call to the llpc.descriptor.* function. A future commit will change it to
    // look up the descSet/binding and generate the code directly.
    auto pBufDescLoadCall = EmitCall(pInsertPos->getModule(),
                                     LlpcName::DescriptorLoadBuffer,
                                     VectorType::get(getInt32Ty(), 4),
                                     {
                                         getInt32(descSet),
                                         getInt32(binding),
                                         pDescIndex,
                                     },
                                     NoAttrib,
                                     pInsertPos);
    pBufDescLoadCall->setName(instName);

    pBufDescLoadCall = EmitCall(pInsertPos->getModule(),
                                LlpcName::LateLaunderFatPointer,
                                getInt8Ty()->getPointerTo(ADDR_SPACE_BUFFER_FAT_POINTER),
                                pBufDescLoadCall,
                                Attribute::ReadNone,
                                pInsertPos);

    return CreateBitCast(pBufDescLoadCall, GetBufferDescTy(pPointeeTy));
}

// =====================================================================================================================
// Add index onto pointer to image/sampler/texelbuffer/F-mask array of descriptors.
Value* BuilderImplDesc::CreateIndexDescPtr(
    Value*        pDescPtr,           // [in] Descriptor pointer, as returned by this function or one of
                                      //    the CreateGet*DescPtr methods
    Value*        pIndex,             // [in] Index value
    bool          isNonUniform,       // Whether the descriptor index is non-uniform
    const Twine&  instName)           // [in] Name to give instruction(s)
{
    if (pIndex != getInt32(0))
    {
        pIndex = ScalarizeIfUniform(pIndex, isNonUniform);
        std::string name = LlpcName::DescriptorIndex;
        AddTypeMangling(pDescPtr->getType(), {}, name);
        pDescPtr = EmitCall(GetInsertBlock()->getParent()->getParent(),
                            name,
                            pDescPtr->getType(),
                            {
                                pDescPtr,
                                pIndex,
                            },
                            NoAttrib,
                            &*GetInsertPoint());
        pDescPtr->setName(instName);
    }
    return pDescPtr;
}

// =====================================================================================================================
// Load image/sampler/texelbuffer/F-mask descriptor from pointer.
// Returns <8 x i32> descriptor for image or F-mask, or <4 x i32> descriptor for sampler or texel buffer.
Value* BuilderImplDesc::CreateLoadDescFromPtr(
    Value*        pDescPtr,           // [in] Descriptor pointer, as returned by CreateIndexDescPtr or one of
                                      //    the CreateGet*DescPtr methods
    const Twine&  instName)           // [in] Name to give instruction(s)
{
    std::string name = LlpcName::DescriptorLoadFromPtr;
    AddTypeMangling(pDescPtr->getType(), {}, name);
    auto pDesc = EmitCall(GetInsertBlock()->getParent()->getParent(),
                          name,
                          cast<StructType>(pDescPtr->getType())->getElementType(0)->getPointerElementType(),
                          pDescPtr,
                          NoAttrib,
                          &*GetInsertPoint());
    pDesc->setName(instName);
    return pDesc;
}

// =====================================================================================================================
// Create a pointer to sampler descriptor. Returns a value of the type returned by GetSamplerDescPtrTy.
Value* BuilderImplDesc::CreateGetSamplerDescPtr(
    uint32_t      descSet,          // Descriptor set
    uint32_t      binding,          // Descriptor binding
    const Twine&  instName)         // [in] Name to give instruction(s)
{
    // This currently creates calls to the llpc.descriptor.* functions. A future commit will change it to
    // look up the descSet/binding and generate the code directly.
    auto pDescPtr = EmitCall(GetInsertBlock()->getParent()->getParent(),
                             LlpcName::DescriptorGetSamplerPtr,
                             GetSamplerDescPtrTy(),
                             {
                                 getInt32(descSet),
                                 getInt32(binding),
                             },
                             NoAttrib,
                             &*GetInsertPoint());
    pDescPtr->setName(instName);
    return pDescPtr;
}

// =====================================================================================================================
// Create a pointer to image descriptor. Returns a value of the type returned by GetImageDescPtrTy.
Value* BuilderImplDesc::CreateGetImageDescPtr(
    uint32_t      descSet,          // Descriptor set
    uint32_t      binding,          // Descriptor binding
    const Twine&  instName)         // [in] Name to give instruction(s)
{
    // This currently creates calls to the llpc.descriptor.* functions. A future commit will change it to
    // look up the descSet/binding and generate the code directly.
    auto pDescPtr = EmitCall(GetInsertBlock()->getParent()->getParent(),
                             LlpcName::DescriptorGetResourcePtr,
                             GetImageDescPtrTy(),
                             {
                                 getInt32(descSet),
                                 getInt32(binding),
                             },
                             NoAttrib,
                             &*GetInsertPoint());
    pDescPtr->setName(instName);
    return pDescPtr;
}

// =====================================================================================================================
// Create a pointer to texel buffer descriptor. Returns a value of the type returned by GetTexelBufferDescPtrTy.
Value* BuilderImplDesc::CreateGetTexelBufferDescPtr(
    uint32_t      descSet,          // Descriptor set
    uint32_t      binding,          // Descriptor binding
    const Twine&  instName)         // [in] Name to give instruction(s)
{
    // This currently creates calls to the llpc.descriptor.* functions. A future commit will change it to
    // look up the descSet/binding and generate the code directly.
    auto pDescPtr = EmitCall(GetInsertBlock()->getParent()->getParent(),
                             LlpcName::DescriptorGetTexelBufferPtr,
                             GetTexelBufferDescPtrTy(),
                             {
                                 getInt32(descSet),
                                 getInt32(binding),
                             },
                             NoAttrib,
                             &*GetInsertPoint());
    pDescPtr->setName(instName);
    return pDescPtr;
}

// =====================================================================================================================
// Create a pointer to F-mask descriptor. Returns a value of the type returned by GetFmaskDescPtrTy.
Value* BuilderImplDesc::CreateGetFmaskDescPtr(
    uint32_t      descSet,          // Descriptor set
    uint32_t      binding,          // Descriptor binding
    const Twine&  instName)         // [in] Name to give instruction(s)
{
    // This currently creates calls to the llpc.descriptor.* functions. A future commit will change it to
    // look up the descSet/binding and generate the code directly.
    auto pDescPtr = EmitCall(GetInsertBlock()->getParent()->getParent(),
                             LlpcName::DescriptorGetFmaskPtr,
                             GetFmaskDescPtrTy(),
                             {
                                 getInt32(descSet),
                                 getInt32(binding),
                             },
                             NoAttrib,
                             &*GetInsertPoint());
    pDescPtr->setName(instName);
    return pDescPtr;
}

// =====================================================================================================================
// Create a load of the push constants table pointer.
// This returns a pointer to the ResourceMappingNodeType::PushConst resource in the top-level user data table.
Value* BuilderImplDesc::CreateLoadPushConstantsPtr(
    Type*         pPushConstantsTy, // [in] Type of the push constants table that the returned pointer will point to
    const Twine&  instName)         // [in] Name to give instruction(s)
{
    auto pPushConstantsPtrTy = PointerType::get(pPushConstantsTy, ADDR_SPACE_CONST);
    // TODO: This currently creates a call to the llpc.descriptor.* function. A future commit will change it to
    // generate the code directly.
    Instruction* pInsertPos = &*GetInsertPoint();
    auto pPushConstantsLoadCall = EmitCall(pInsertPos->getModule(),
                                           LlpcName::DescriptorLoadSpillTable,
                                           pPushConstantsPtrTy,
                                           {},
                                           NoAttrib,
                                           pInsertPos);
    pPushConstantsLoadCall->setName(instName);
    return pPushConstantsLoadCall;
}

// =====================================================================================================================
// Scalarize a value (pass it through readfirstlane) if uniform
Value* BuilderImplDesc::ScalarizeIfUniform(
    Value*  pValue,       // [in] 32-bit integer value to scalarize
    bool    isNonUniform) // Whether value is marked as non-uniform
{
    LLPC_ASSERT(pValue->getType()->isIntegerTy(32));
    if ((isNonUniform == false) && (isa<Constant>(pValue) == false))
    {
        // NOTE: GFX6 encounters GPU hang with this optimization enabled. So we should skip it.
        if (getContext().GetGfxIpVersion().major > 6)
        {
            pValue = CreateIntrinsic(Intrinsic::amdgcn_readfirstlane, {}, pValue);
        }
    }
    return pValue;
}

// =====================================================================================================================
// Create a buffer length query based on the specified descriptor.
Value* BuilderImplDesc::CreateGetBufferDescLength(
    Value* const  pBufferDesc,      // [in] The buffer descriptor to query.
    const Twine&  instName)         // [in] Name to give instruction(s).
{
    // In future this should become a full LLVM intrinsic, but for now we patch in a late intrinsic that is cleaned up
    // in patch buffer op.
    Instruction* const pInsertPos = &*GetInsertPoint();
    std::string callName = LlpcName::LateBufferLength;
    AddTypeMangling(nullptr, pBufferDesc, callName);
    return EmitCall(pInsertPos->getModule(),
                    callName,
                    getInt32Ty(),
                    pBufferDesc,
                    Attribute::ReadNone,
                    pInsertPos);
}

