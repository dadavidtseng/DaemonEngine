//-----------------------------------------------------------------------------------------------
// SpriteAnimDefinition.cpp
//

//-----------------------------------------------------------------------------------------------
#include "Engine/Renderer/SpriteAnimDefinition.hpp"

#include <algorithm>

#include "SpriteSheet.hpp"
#include "Engine/Core/ErrorWarningAssert.hpp"

SpriteAnimDefinition::SpriteAnimDefinition(SpriteSheet const& sheet, int const startSpriteIndex, int const endSpriteIndex, float const framesPerSecond, SpriteAnimPlaybackType const playbackType)
    : m_spriteSheet(sheet),
      m_startSpriteIndex(startSpriteIndex),
      m_endSpriteIndex(endSpriteIndex),
      m_framesPerSecond(framesPerSecond),
      m_playbackType(playbackType)
{
}
SpriteDefinition const& SpriteAnimDefinition::GetSpriteDefAtTime(float seconds) const
{
    // 計算總幀數
    int totalFrames = m_endSpriteIndex - m_startSpriteIndex + 1;
    int totalFrameCount = static_cast<int>(seconds * m_framesPerSecond);

    // printf("totalFrames: %d, totalFrameCount: %d\n", totalFrames, totalFrameCount);

    int currentFrame = 0;

    // 根據播放類型選擇動畫邏輯
    switch (m_playbackType)
    {
        case SpriteAnimPlaybackType::ONCE:
            currentFrame = std::min(totalFrameCount, totalFrames - 1);
        break;

        case SpriteAnimPlaybackType::LOOP:
            currentFrame = totalFrameCount % totalFrames;
        break;

        case SpriteAnimPlaybackType::PINGPONG:
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
            ERROR_AND_DIE("Unknown SpriteAnimPlaybackType");
        break;
    }

    // 計算對應的精靈索引
    int spriteIndex = m_startSpriteIndex + currentFrame;

    // 調試輸出
    // printf("currentFrame: %d, spriteIndex: %d\n", currentFrame, spriteIndex);

    // 返回對應的 SpriteDefinition
    return m_spriteSheet.GetSpriteDef(spriteIndex);
}

int SpriteAnimDefinition::GetTotalFrameInCycle()
{
    return 0;
}

float SpriteAnimDefinition::GetDuration()
{
    return 0.f;
}

