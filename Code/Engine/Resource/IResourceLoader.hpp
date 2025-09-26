//----------------------------------------------------------------------------------------------------
// IResourceLoader.hpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#pragma once
#include <memory>
#include <string>
#include <vector>

#include "Engine/Core/StringUtils.hpp"

//-Forward-Declaration--------------------------------------------------------------------------------
class IResource;

//----------------------------------------------------------------------------------------------------
class IResourceLoader
{
public:
    virtual ~IResourceLoader() = default;

    // 檢查是否支援此檔案類型
    virtual bool CanLoad(String const& extension) const = 0;

    // 載入資源
    virtual std::shared_ptr<IResource> Load(String const& path) = 0;

    // 取得支援的副檔名列表
    virtual std::vector<String> GetSupportedExtensions() const = 0;
};
