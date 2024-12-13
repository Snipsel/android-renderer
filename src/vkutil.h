#pragma once
#include "include.h"

char const* str_vkcolorspace(VkColorSpaceKHR colorspace){
#define F(X) case VK_COLOR_SPACE_##X: return #X;
    switch(colorspace){
    F(SRGB_NONLINEAR_KHR)
    F(DISPLAY_P3_NONLINEAR_EXT)
    F(EXTENDED_SRGB_LINEAR_EXT)
    F(DISPLAY_P3_LINEAR_EXT)
    F(DCI_P3_NONLINEAR_EXT)
    F(BT709_LINEAR_EXT)
    F(BT709_NONLINEAR_EXT)
    F(BT2020_LINEAR_EXT)
    F(HDR10_ST2084_EXT)
    F(DOLBYVISION_EXT)
    F(HDR10_HLG_EXT)
    F(ADOBERGB_LINEAR_EXT)
    F(ADOBERGB_NONLINEAR_EXT)
    F(PASS_THROUGH_EXT)
    F(EXTENDED_SRGB_NONLINEAR_EXT)
    F(DISPLAY_NATIVE_AMD)
    default: return "???";
    }
#undef F
}

char const* str_vkformat(VkFormat fmt){
#define F(X) case VK_FORMAT_##X: return #X;
    switch(fmt){
    F(UNDEFINED)
    F(R4G4_UNORM_PACK8)
    F(R4G4B4A4_UNORM_PACK16)
    F(B4G4R4A4_UNORM_PACK16)
    F(R5G6B5_UNORM_PACK16)
    F(B5G6R5_UNORM_PACK16)
    F(R5G5B5A1_UNORM_PACK16)
    F(B5G5R5A1_UNORM_PACK16)
    F(A1R5G5B5_UNORM_PACK16)
    F(R8_UNORM)
    F(R8_SNORM)
    F(R8_USCALED)
    F(R8_SSCALED)
    F(R8_UINT)
    F(R8_SINT)
    F(R8_SRGB)
    F(R8G8_UNORM)
    F(R8G8_SNORM)
    F(R8G8_USCALED)
    F(R8G8_SSCALED)
    F(R8G8_UINT)
    F(R8G8_SINT)
    F(R8G8_SRGB)
    F(R8G8B8_UNORM)
    F(R8G8B8_SNORM)
    F(R8G8B8_USCALED)
    F(R8G8B8_SSCALED)
    F(R8G8B8_UINT)
    F(R8G8B8_SINT)
    F(R8G8B8_SRGB)
    F(B8G8R8_UNORM)
    F(B8G8R8_SNORM)
    F(B8G8R8_USCALED)
    F(B8G8R8_SSCALED)
    F(B8G8R8_UINT)
    F(B8G8R8_SINT)
    F(B8G8R8_SRGB)
    F(R8G8B8A8_UNORM)
    F(R8G8B8A8_SNORM)
    F(R8G8B8A8_USCALED)
    F(R8G8B8A8_SSCALED)
    F(R8G8B8A8_UINT)
    F(R8G8B8A8_SINT)
    F(R8G8B8A8_SRGB)
    F(B8G8R8A8_UNORM)
    F(B8G8R8A8_SNORM)
    F(B8G8R8A8_USCALED)
    F(B8G8R8A8_SSCALED)
    F(B8G8R8A8_UINT)
    F(B8G8R8A8_SINT)
    F(B8G8R8A8_SRGB)
    F(A8B8G8R8_UNORM_PACK32)
    F(A8B8G8R8_SNORM_PACK32)
    F(A8B8G8R8_USCALED_PACK32)
    F(A8B8G8R8_SSCALED_PACK32)
    F(A8B8G8R8_UINT_PACK32)
    F(A8B8G8R8_SINT_PACK32)
    F(A8B8G8R8_SRGB_PACK32)
    F(A2R10G10B10_UNORM_PACK32)
    F(A2R10G10B10_SNORM_PACK32)
    F(A2R10G10B10_USCALED_PACK32)
    F(A2R10G10B10_SSCALED_PACK32)
    F(A2R10G10B10_UINT_PACK32)
    F(A2R10G10B10_SINT_PACK32)
    F(A2B10G10R10_UNORM_PACK32)
    F(A2B10G10R10_SNORM_PACK32)
    F(A2B10G10R10_USCALED_PACK32)
    F(A2B10G10R10_SSCALED_PACK32)
    F(A2B10G10R10_UINT_PACK32)
    F(A2B10G10R10_SINT_PACK32)
    F(R16_UNORM)
    F(R16_SNORM)
    F(R16_USCALED)
    F(R16_SSCALED)
    F(R16_UINT)
    F(R16_SINT)
    F(R16_SFLOAT)
    F(R16G16_UNORM)
    F(R16G16_SNORM)
    F(R16G16_USCALED)
    F(R16G16_SSCALED)
    F(R16G16_UINT)
    F(R16G16_SINT)
    F(R16G16_SFLOAT)
    F(R16G16B16_UNORM)
    F(R16G16B16_SNORM)
    F(R16G16B16_USCALED)
    F(R16G16B16_SSCALED)
    F(R16G16B16_UINT)
    F(R16G16B16_SINT)
    F(R16G16B16_SFLOAT)
    F(R16G16B16A16_UNORM)
    F(R16G16B16A16_SNORM)
    F(R16G16B16A16_USCALED)
    F(R16G16B16A16_SSCALED)
    F(R16G16B16A16_UINT)
    F(R16G16B16A16_SINT)
    F(R16G16B16A16_SFLOAT)
    F(R32_UINT)
    F(R32_SINT)
    F(R32_SFLOAT)
    F(R32G32_UINT)
    F(R32G32_SINT)
    F(R32G32_SFLOAT)
    F(R32G32B32_UINT)
    F(R32G32B32_SINT)
    F(R32G32B32_SFLOAT)
    F(R32G32B32A32_UINT)
    F(R32G32B32A32_SINT)
    F(R32G32B32A32_SFLOAT)
    F(R64_UINT)
    F(R64_SINT)
    F(R64_SFLOAT)
    F(R64G64_UINT)
    F(R64G64_SINT)
    F(R64G64_SFLOAT)
    F(R64G64B64_UINT)
    F(R64G64B64_SINT)
    F(R64G64B64_SFLOAT)
    F(R64G64B64A64_UINT)
    F(R64G64B64A64_SINT)
    F(R64G64B64A64_SFLOAT)
    F(B10G11R11_UFLOAT_PACK32)
    F(E5B9G9R9_UFLOAT_PACK32)
    F(D16_UNORM)
    F(X8_D24_UNORM_PACK32)
    F(D32_SFLOAT)
    F(S8_UINT)
    F(D16_UNORM_S8_UINT)
    F(D24_UNORM_S8_UINT)
    F(D32_SFLOAT_S8_UINT)
    F(BC1_RGB_UNORM_BLOCK)
    F(BC1_RGB_SRGB_BLOCK)
    F(BC1_RGBA_UNORM_BLOCK)
    F(BC1_RGBA_SRGB_BLOCK)
    F(BC2_UNORM_BLOCK)
    F(BC2_SRGB_BLOCK)
    F(BC3_UNORM_BLOCK)
    F(BC3_SRGB_BLOCK)
    F(BC4_UNORM_BLOCK)
    F(BC4_SNORM_BLOCK)
    F(BC5_UNORM_BLOCK)
    F(BC5_SNORM_BLOCK)
    F(BC6H_UFLOAT_BLOCK)
    F(BC6H_SFLOAT_BLOCK)
    F(BC7_UNORM_BLOCK)
    F(BC7_SRGB_BLOCK)
    F(ETC2_R8G8B8_UNORM_BLOCK)
    F(ETC2_R8G8B8_SRGB_BLOCK)
    F(ETC2_R8G8B8A1_UNORM_BLOCK)
    F(ETC2_R8G8B8A1_SRGB_BLOCK)
    F(ETC2_R8G8B8A8_UNORM_BLOCK)
    F(ETC2_R8G8B8A8_SRGB_BLOCK)
    F(EAC_R11_UNORM_BLOCK)
    F(EAC_R11_SNORM_BLOCK)
    F(EAC_R11G11_UNORM_BLOCK)
    F(EAC_R11G11_SNORM_BLOCK)
    F(ASTC_4x4_UNORM_BLOCK)
    F(ASTC_4x4_SRGB_BLOCK)
    F(ASTC_5x4_UNORM_BLOCK)
    F(ASTC_5x4_SRGB_BLOCK)
    F(ASTC_5x5_UNORM_BLOCK)
    F(ASTC_5x5_SRGB_BLOCK)
    F(ASTC_6x5_UNORM_BLOCK)
    F(ASTC_6x5_SRGB_BLOCK)
    F(ASTC_6x6_UNORM_BLOCK)
    F(ASTC_6x6_SRGB_BLOCK)
    F(ASTC_8x5_UNORM_BLOCK)
    F(ASTC_8x5_SRGB_BLOCK)
    F(ASTC_8x6_UNORM_BLOCK)
    F(ASTC_8x6_SRGB_BLOCK)
    F(ASTC_8x8_UNORM_BLOCK)
    F(ASTC_8x8_SRGB_BLOCK)
    F(ASTC_10x5_UNORM_BLOCK)
    F(ASTC_10x5_SRGB_BLOCK)
    F(ASTC_10x6_UNORM_BLOCK)
    F(ASTC_10x6_SRGB_BLOCK)
    F(ASTC_10x8_UNORM_BLOCK)
    F(ASTC_10x8_SRGB_BLOCK)
    F(ASTC_10x10_UNORM_BLOCK)
    F(ASTC_10x10_SRGB_BLOCK)
    F(ASTC_12x10_UNORM_BLOCK)
    F(ASTC_12x10_SRGB_BLOCK)
    F(ASTC_12x12_UNORM_BLOCK)
    F(ASTC_12x12_SRGB_BLOCK)
    default: return "???";
    }
#undef F
}
