//----------------------------------------------------------------------------------------------------
// SpriteAnimDefinition.cpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#include "Engine/Renderer/SpriteAnimDefinition.hpp"

#include <algorithm>

#include "Engine/Core/ErrorWarningAssert.hpp"
#include "Engine/Renderer/SpriteSheet.hpp"

//----------------------------------------------------------------------------------------------------
SpriteAnimDefinition::SpriteAnimDefinition(SpriteSheet const&            sheet,
                                           int const                     startSpriteIndex,
                                           int const                     endSpriteIndex,
                                           float const                   framesPerSecond,
                                           eSpriteAnimPlaybackType const playbackType)
    : m_spriteSheet(sheet),
      m_startSpriteIndex(startSpriteIndex),
      m_endSpriteIndex(endSpriteIndex),
      m_framesPerSecond(framesPerSecond),
      m_playbackType(playbackType)
{
}

//----------------------------------------------------------------------------------------------------
SpriteDefinition const& SpriteAnimDefinition::GetSpriteDefAtTime(float const seconds) const
{
    // 計算總幀數
    int const totalFrames     = m_endSpriteIndex - m_startSpriteIndex + 1;
    int const totalFrameCount = static_cast<int>(seconds * m_framesPerSecond);

    // printf("totalFrames: %d, totalFrameCount: %d\n", totalFrames, totalFrameCount);

    int currentFrame;

    // 根據播放類型選擇動畫邏輯
    switch (m_playbackType)
    {
    case eSpriteAnimPlaybackType::ONCE:
        currentFrame = std::min(totalFrameCount, totalFrames - 1);
        break;

    case eSpriteAnimPlaybackType::LOOP:
        currentFrame = totalFrameCount % totalFrames;
        break;

    case eSpriteAnimPlaybackType::PINGPONG:
        {
            // 計算單個往返週期的長度
            int pingpongCycleLength = (totalFrames * 2) - 2; // 去掉最後一幀的重複

            // 限制在單個往返週期範圍內
            int pingpongFrame = totalFrameCount % pingpongCycleLength;

            if (pingpongFrame >= totalFrames)
            {
                // 進入返回部分：計算倒退幀
                currentFrame = (totalFrames - 2) - (pingpongFrame - totalFrames);
            }
            else
            {
                // 正向部分
                currentFrame = pingpongFrame;
            }
            break;
        }

    default:
        ERROR_AND_DIE("Unknown SpriteAnimPlaybackType")
        break;
    }

    // 計算對應的精靈索引
    int const spriteIndex = m_startSpriteIndex + currentFrame;

    // 調試輸出
    // printf("currentFrame: %d, spriteIndex: %d\n", currentFrame, spriteIndex);

    // 返回對應的 SpriteDefinition
    return m_spriteSheet.GetSpriteDef(spriteIndex);
}

//----------------------------------------------------------------------------------------------------
int SpriteAnimDefinition::GetTotalFrameInCycle() const
{
    int const totalFrames = m_endSpriteIndex - m_startSpriteIndex + 1;

    switch (m_playbackType)
    {
    case eSpriteAnimPlaybackType::ONCE:
    case eSpriteAnimPlaybackType::LOOP:
        return totalFrames;

    case eSpriteAnimPlaybackType::PINGPONG:
        return (totalFrames * 2) - 2; // 前後各一次，中間的幀不重複

    default: ERROR_AND_DIE("Unknown SpriteAnimPlaybackType")
    }
}

//----------------------------------------------------------------------------------------------------
float SpriteAnimDefinition::GetDuration() const
{
    int const totalFramesInCycle = GetTotalFrameInCycle();

    return static_cast<float>(totalFramesInCycle) / m_framesPerSecond;
}
