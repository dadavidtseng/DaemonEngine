//----------------------------------------------------------------------------------------------------
// ObjModelLoader.cpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#include "Engine/Resource/ObjModelLoader.hpp"
#include <filesystem>
#include <sstream>

#include "Engine/Core/ErrorWarningAssert.hpp"
#include "Engine/Core/FileUtils.hpp"
#include "Engine/Core/StringUtils.hpp"
#include "Engine/Core/Time.hpp"
#include "Engine/Renderer/Vertex_PCUTBN.hpp"
#include "Engine/Math/MathUtils.hpp"
#include "Engine/Resource/IResource.hpp"
#include "Engine/Resource/ModelResource.hpp"

//----------------------------------------------------------------------------------------------------
bool ObjModelLoader::Load(String const&      fileName,
                          VertexList_PCUTBN& out_vertexes,
                          IndexList&         out_indexes,
                          bool&              out_hasNormals,
                          bool&              out_hasUVs,
                          Mat44 const&       transform /*= Mat44() */) noexcept
{
    double loadStartTime = GetCurrentTimeSeconds();

    out_vertexes.clear();
    out_indexes.clear();
    out_hasNormals = false;
    out_hasUVs     = false;

    std::string rawObjFile;
    FileReadToString(rawObjFile, fileName);

    // 預先分配記憶體空間以減少重新分配
    out_vertexes.reserve(10000);
    out_indexes.reserve(30000);

    std::vector<Vec3>                 vertPositions;
    std::vector<Vec3>                 normals;
    std::vector<Vec2>                 textureCoords;
    std::unordered_map<String, Rgba8> materialMap;

    // 預先分配空間
    vertPositions.reserve(5000);
    normals.reserve(5000);
    textureCoords.reserve(5000);
    materialMap.reserve(50);

    Rgba8* curRgba8   = nullptr;
    int    numOfFaces = 0;

    DebuggerPrintf("-------------------------------------\n");
    DebuggerPrintf("Loaded .obj file %s\n", fileName.c_str());
    double startTime = GetCurrentTimeSeconds();

    // 使用string_view來避免不必要的字串複製
    std::istringstream stream(rawObjFile);
    std::string        line;

    while (std::getline(stream, line))
    {
        if (line.empty() || line[0] == '#') continue;

        std::istringstream lineStream(line);
        std::string        prefix;
        lineStream >> prefix;

        if (prefix == "v")
        {
            float x, y, z;
            lineStream >> x >> y >> z;
            vertPositions.emplace_back(x, y, z);
        }
        else if (prefix == "vn")
        {
            float x, y, z;
            lineStream >> x >> y >> z;
            normals.emplace_back(x, y, z);
        }
        else if (prefix == "vt")
        {
            float u, v;
            lineStream >> u >> v;
            textureCoords.emplace_back(u, v);
        }
        else if (prefix == "f")
        {
            numOfFaces++;
            std::vector<std::string> faceVertices;
            std::string              vertex;

            while (lineStream >> vertex)
            {
                faceVertices.push_back(vertex);
            }

            if (faceVertices.size() < 3) continue;

            // 解析頂點的 lambda 函數
            auto parseVertex = [&](const std::string& vertexStr) -> Vertex_PCUTBN {
                Vertex_PCUTBN      vert;
                std::istringstream vertStream(vertexStr);
                std::string        posIdx, uvIdx, normalIdx;

                std::getline(vertStream, posIdx, '/');
                std::getline(vertStream, uvIdx, '/');
                std::getline(vertStream, normalIdx);

                // 位置索引
                int pos         = std::stoi(posIdx) - 1;
                vert.m_position = vertPositions[pos];

                // UV座標
                if (!uvIdx.empty())
                {
                    out_hasUVs         = true;
                    int uv             = std::stoi(uvIdx) - 1;
                    vert.m_uvTexCoords = textureCoords[uv];
                }

                // 法線處理
                if (!normalIdx.empty() && !normals.empty())
                {
                    // OBJ 檔案提供了法線，直接使用
                    out_hasNormals = true;
                    int normal     = std::stoi(normalIdx) - 1;
                    vert.m_normal  = normals[normal];
                }
                // 如果沒有法線，會在後面計算三角形法線

                // 顏色
                vert.m_color = curRgba8 ? *curRgba8 : Rgba8::WHITE;

                return vert;
            };

            // 三角化多邊形（扇形三角化）
            Vertex_PCUTBN firstVert = parseVertex(faceVertices[0]);

            for (size_t i = 1; i < faceVertices.size() - 1; ++i)
            {
                Vertex_PCUTBN vert1 = parseVertex(faceVertices[i]);
                Vertex_PCUTBN vert2 = parseVertex(faceVertices[i + 1]);

                // **重要：如果沒有法線資料，計算三角形法線**
                if (normals.empty())
                {
                    // 計算三角形的兩個邊向量
                    Vec3 edge1 = vert1.m_position - firstVert.m_position;
                    Vec3 edge2 = vert2.m_position - firstVert.m_position;

                    // 使用叉積計算法線（右手座標系）
                    Vec3 triangleNormal = CrossProduct3D(edge1, edge2).GetNormalized();

                    // 將計算出的法線指派給三個頂點
                    firstVert.m_normal = triangleNormal;
                    vert1.m_normal     = triangleNormal;
                    vert2.m_normal     = triangleNormal;

                    out_hasNormals = true;
                }

                // 添加三角形
                out_indexes.push_back(static_cast<unsigned int>(out_vertexes.size()));
                out_vertexes.push_back(firstVert);
                out_indexes.push_back(static_cast<unsigned int>(out_vertexes.size()));
                out_vertexes.push_back(vert1);
                out_indexes.push_back(static_cast<unsigned int>(out_vertexes.size()));
                out_vertexes.push_back(vert2);
            }
        }
        else if (prefix == "mtllib")
        {
            std::string mtlFile;
            lineStream >> mtlFile;

            char drive[300], dir[300], fname[300], ext[300];
            _splitpath_s(fileName.c_str(), drive, 300, dir, 300, fname, 300, ext, 300);
            std::string materialPath = std::string(drive) + dir + mtlFile;

            if (!ObjModelLoader::LoadMaterial(materialPath, materialMap))
            {
                return false;
            }
        }
        else if (prefix == "usemtl")
        {
            std::string materialName;
            lineStream >> materialName;

            auto iter = materialMap.find(materialName);
            curRgba8  = (iter != materialMap.end()) ? &iter->second : nullptr;
        }
    }

    // 在 Load 函數的最後，變換矩陣應用之前添加：
    if (normals.empty() && out_hasNormals)
    {
        // 如果我們計算了法線，進行平滑化處理
        std::unordered_map<Vec3, std::vector<Vec3>, Vec3Hasher> positionToNormals;

        // 收集每個位置的所有法線
        for (const auto& vertex : out_vertexes)
        {
            positionToNormals[vertex.m_position].push_back(vertex.m_normal);
        }

        // 計算每個位置的平均法線
        std::unordered_map<Vec3, Vec3, Vec3Hasher> positionToAverageNormal;
        for (const auto& pair : positionToNormals)
        {
            Vec3 sum = Vec3::ZERO;
            for (const Vec3& normal : pair.second)
            {
                sum += normal;
            }
            positionToAverageNormal[pair.first] = sum.GetNormalized();
        }

        // 更新所有頂點的法線
        for (auto& vertex : out_vertexes)
        {
            vertex.m_normal = positionToAverageNormal[vertex.m_position];
        }
    }

    // 應用變換矩陣
    if (transform != Mat44())
    {
        for (auto& vertex : out_vertexes)
        {
            vertex.m_position = transform.TransformPosition3D(vertex.m_position);
        }
    }

    double endTime = GetCurrentTimeSeconds();

    DebuggerPrintf("                            positions: %d  uvs: %d  normals: %d  faces: %d\n",
                   vertPositions.size(), textureCoords.size(), normals.size(), numOfFaces);
    DebuggerPrintf("                            vertexes: %d triangles: %d time: %fs\n",
                   out_vertexes.size(), out_indexes.size() / 3, startTime - loadStartTime);
    DebuggerPrintf("Created CPU mesh            time: %fs\n", endTime - startTime);

    return true;
}

// 進一步優化版本：使用更快的字串解析
bool ObjModelLoader::LoadMaterial(std::string const& path, std::unordered_map<std::string, Rgba8>& materialMap) noexcept
{
    materialMap.clear();

    std::string rawMtlFile;
    FileReadToString(rawMtlFile, path);
    //
    // if (rawMtlFile.empty())
    // {
    //     return false;
    // }

    // 現在可以使用 reserve() 了！
    materialMap.reserve(50);  // 根據預期材質數量調整

    std::string        currentMtlName;
    std::istringstream stream(rawMtlFile);
    std::string        line;

    while (std::getline(stream, line))
    {
        // 跳過空行和註解
        if (line.empty() || line[0] == '#') continue;

        // 移除行尾的空白字符
        while (!line.empty() && std::isspace(line.back()))
        {
            line.pop_back();
        }

        // 快速檢查前綴
        if (line.length() >= 6 && line.substr(0, 6) == "newmtl")
        {
            // 找到第一個空格後的材質名稱
            size_t spacePos = line.find(' ');
            if (spacePos != std::string::npos && spacePos + 1 < line.length())
            {
                currentMtlName = line.substr(spacePos + 1);
                // 移除材質名稱前後的空白
                currentMtlName.erase(0, currentMtlName.find_first_not_of(" \t"));
                currentMtlName.erase(currentMtlName.find_last_not_of(" \t") + 1);
            }
        }
        else if (line.length() >= 2 && line.substr(0, 2) == "Kd")
        {
            if (!currentMtlName.empty())
            {
                // 使用istringstream解析顏色值
                std::istringstream colorStream(line.substr(2)); // 跳過 "Kd"
                float              r, g, b;

                if (colorStream >> r >> g >> b)
                {
                    // 直接構造Rgba8對象並插入
                    materialMap[currentMtlName] = Rgba8(
                        DenormalizeByte(r),
                        DenormalizeByte(g),
                        DenormalizeByte(b),
                        255  // Alpha值
                    );
                }
            }
        }
    }

    return true;
}

bool ObjModelLoader::CanLoad(String const& extension) const
{
    std::string ext = extension;
    std::transform(ext.begin(), ext.end(), ext.begin(),
                   [](unsigned char const c) { return static_cast<char>(std::tolower(c)); });
    return ext == ".obj";
}

std::vector<std::string> ObjModelLoader::GetSupportedExtensions() const
{
    return {".obj", ".OBJ"};
}

std::shared_ptr<IResource> ObjModelLoader::Load(const std::string& path)
{
    auto modelResource = std::make_shared<ModelResource>(path);

    // 直接呼叫 Load，它會使用靜態的 Load 方法
    if (modelResource->Load())
    {
        return modelResource;
    }

    return nullptr;
}
