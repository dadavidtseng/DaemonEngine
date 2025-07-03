// ResourceManager.cpp - 實現範例
#include "ResourceSubsystem.hpp"
#include <filesystem>

#include "IResource.hpp"

void ResourceSubsystem::Initialize(size_t numThreads)
{
    m_running = true;

    // 註冊預設載入器
    // RegisterLoader(std::make_unique<ObjModelLoader>());
    // RegisterLoader(std::make_unique<TextureLoader>());
    // RegisterLoader(std::make_unique<MaterialLoader>());
    // RegisterLoader(std::make_unique<SoundLoader>());

    // 創建工作執行緒
    for (size_t i = 0; i < numThreads; ++i)
    {
        m_loadingThreads.emplace_back([this]() {
            while (m_running)
            {
                std::unique_lock<std::mutex> lock(m_queueMutex);
                m_cv.wait(lock, [this]() { return !m_loadingQueue.empty() || !m_running; });

                if (!m_running) break;

                auto task = m_loadingQueue.front();
                m_loadingQueue.pop();
                lock.unlock();

                task();
            }
        });
    }
}

void ResourceSubsystem::Shutdown()
{
    m_running = false;
    m_cv.notify_all();

    for (auto& thread : m_loadingThreads)
    {
        if (thread.joinable())
        {
            thread.join();
        }
    }

    m_cache.Clear();
}

std::shared_ptr<IResource> ResourceSubsystem::LoadResource(const std::string& path)
{
    std::string ext = GetFileExtension(path);

    for (const auto& loader : m_loaders)
    {
        if (loader->CanLoad(ext))
        {
            return loader->Load(path);
        }
    }

    // return nullptr;
}

// 使用範例
void Example()
{
    auto& rm = ResourceSubsystem::GetInstance();
    rm.Initialize(4);

    // // 同步載入
    // auto modelHandle = rm.Load<ModelResource>("assets/models/character.obj");
    // if (modelHandle.IsValid())
    // {
    //     auto model = modelHandle.Get();
    //     // 使用模型...
    // }
    //
    // // 異步載入
    // auto futureTexture = rm.LoadAsync<TextureResource>("assets/textures/wall.png");
    // // 做其他事情...
    // auto textureHandle = futureTexture.get();
    //
    // // 批量載入
    // std::vector<std::string> modelPaths = {
    //     "assets/models/tree.obj",
    //     "assets/models/rock.obj",
    //     "assets/models/house.obj"
    // };
    // auto models = rm.LoadBatch<ModelResource>(modelPaths);
}