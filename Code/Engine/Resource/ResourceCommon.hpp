// ============================================
// ResourceCommon.hpp - 共用類型定義
// ============================================
#pragma once
#include <string>
#include <memory>
#include <atomic>

// 資源類型枚舉
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

// 資源載入優先級
enum class ResourcePriority
{
    Low,
    Normal,
    High,
    Critical
};