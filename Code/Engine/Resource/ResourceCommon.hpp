// ResourceTypes.hpp - 資源類型定義
#pragma once
#include <string>
#include <memory>
#include <typeindex>

enum class ResourceType
{
    Model,
    Texture,
    Material,
    Shader,
    Sound,
    Light,
    Animation,
    Particle,
    Unknown
};

// 資源狀態
enum class ResourceState
{
    Unloaded,
    Loading,
    Loaded,
    Failed
};