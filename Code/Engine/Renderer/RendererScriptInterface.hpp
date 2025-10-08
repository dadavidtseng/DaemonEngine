//----------------------------------------------------------------------------------------------------
// RendererScriptInterface.hpp
// Script interface for Renderer subsystem - exposes rendering APIs to JavaScript
//----------------------------------------------------------------------------------------------------
#pragma once
//----------------------------------------------------------------------------------------------------
#include "Engine/Script/IScriptableObject.hpp"
#include "Engine/Core/Rgba8.hpp"
//----------------------------------------------------------------------------------------------------
#include <memory>
#include <unordered_map>
#include <vector>

//----------------------------------------------------------------------------------------------------
// Forward Declarations
//----------------------------------------------------------------------------------------------------
class Renderer;
class Texture;
struct Vertex_PCU;

//----------------------------------------------------------------------------------------------------
/**
 * RendererScriptInterface
 *
 * Exposes Renderer functionality to JavaScript for entity rendering.
 * Provides methods used by Prop::Render() including:
 * - SetModelConstants (transform + color)
 * - SetBlendMode, SetRasterizerMode, SetSamplerMode, SetDepthMode
 * - BindTexture, BindShader
 * - DrawVertexArray
 *
 * Usage from JavaScript:
 *   renderer.setModelConstants(transform, color);
 *   renderer.setBlendMode("OPAQUE");
 *   renderer.bindShader("Data/Shaders/Default");
 *   renderer.drawVertexArray(vertexArrayHandle);
 */
//----------------------------------------------------------------------------------------------------
class RendererScriptInterface : public IScriptableObject
{
public:
    explicit RendererScriptInterface(Renderer* renderer);
    ~RendererScriptInterface() override = default;

    // IScriptableObject interface
    void                          InitializeMethodRegistry() override;
    ScriptMethodResult              CallMethod(String const& methodName, ScriptArgs const& args) override;
    std::vector<ScriptMethodInfo>   GetAvailableMethods() const override;
    std::vector<String>             GetAvailableProperties() const override;
    std::any                        GetProperty(String const& propertyName) const override;
    bool                            SetProperty(String const& propertyName, std::any const& value) override;

private:
    // Rendering state methods
    ScriptMethodResult ExecuteSetModelConstants(ScriptArgs const& args);
    ScriptMethodResult ExecuteSetBlendMode(ScriptArgs const& args);
    ScriptMethodResult ExecuteSetRasterizerMode(ScriptArgs const& args);
    ScriptMethodResult ExecuteSetSamplerMode(ScriptArgs const& args);
    ScriptMethodResult ExecuteSetDepthMode(ScriptArgs const& args);

    // Resource binding methods
    ScriptMethodResult ExecuteBindTexture(ScriptArgs const& args);
    ScriptMethodResult ExecuteBindShader(ScriptArgs const& args);

    // Drawing methods
    ScriptMethodResult ExecuteDrawVertexArray(ScriptArgs const& args);

    // Vertex array management
    ScriptMethodResult ExecuteCreateVertexArray(ScriptArgs const& args);
    ScriptMethodResult ExecuteAddVertex(ScriptArgs const& args);
    ScriptMethodResult ExecuteAddVertexBatch(ScriptArgs const& args);

    // Helper methods for enum conversion
    int  StringToBlendMode(String const& modeStr) const;
    int  StringToRasterizerMode(String const& modeStr) const;
    int  StringToSamplerMode(String const& modeStr) const;
    int  StringToDepthMode(String const& modeStr) const;


private:
    Renderer*                                            m_renderer = nullptr;
    std::unordered_map<String, std::vector<Vertex_PCU>> m_vertexArrays;  // Handle -> vertex data
    String                                               m_currentVertexArrayHandle;
    int                                                  m_nextVertexArrayId = 0;
};
