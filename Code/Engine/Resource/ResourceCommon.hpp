//----------------------------------------------------------------------------------------------------
// ResourceCommon.hpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#pragma once

//----------------------------------------------------------------------------------------------------
enum class eResourceType
{
    Unknown,
    Model,
    TEXTURE,
    SHADER,
    Material,
    AUDIO,
    FONT,
    Animation
};

// 資源狀態
enum class eResourceState
{
    Unloaded,    // 未載入
    Loading,     // 載入中
    Loaded,      // 已載入
    Failed,      // 載入失敗
    Unloading    // 卸載中
};

enum class eResourcePriority
{
    Low,
    Normal,
    High,
    Critical
};
