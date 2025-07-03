//----------------------------------------------------------------------------------------------------
// IResourceLoader.hpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#pragma once
#include <memory>
#include <string>
#include <vector>
#include "IResource.hpp"

class IResourceLoader
{
public:
    virtual ~IResourceLoader() = default;
    virtual bool CanLoad(const std::string& extension) const = 0;
    virtual std::shared_ptr<IResource> Load(const std::string& path) = 0;
    virtual std::vector<std::string> GetSupportedExtensions() const = 0;
};