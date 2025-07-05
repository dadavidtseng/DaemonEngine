// ============================================
// IResourceLoader.hpp - 載入器介面
// ============================================
#pragma once
#include <memory>
#include <string>
#include <vector>

class IResource;

class IResourceLoader
{
public:
    virtual ~IResourceLoader() = default;
    
    // 檢查是否支援此檔案類型
    virtual bool CanLoad(const std::string& extension) const = 0;
    
    // 載入資源
    virtual std::shared_ptr<IResource> Load(const std::string& path) = 0;
    
    // 取得支援的副檔名列表
    virtual std::vector<std::string> GetSupportedExtensions() const = 0;
};