//----------------------------------------------------------------------------------------------------
// ResourceCommon.hpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#pragma once

//----------------------------------------------------------------------------------------------------
enum class ResourceType
{
    Unknown,
    Model,
    Texture,
    Shader,
    Material,
    Audio,
    Font,
    Animation,
    Particle
};

// 資源狀態
enum class ResourceState
{
    Unloaded,    // 未載入
    Loading,     // 載入中
    Loaded,      // 已載入
    Failed,      // 載入失敗
    Unloading    // 卸載中
};

enum class ResourcePriority
{
    Low,
    Normal,
    High,
    Critical
};
