//----------------------------------------------------------------------------------------------------
// InputSystem.cpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#include "Engine/Input/InputSystem.hpp"
//----------------------------------------------------------------------------------------------------
#include "Engine/Core/EngineCommon.hpp"
#include "Engine/Core/LogSubsystem.hpp"
#include "Engine/Platform/Window.hpp"
#include "Engine/Core/StringUtils.hpp"
//----------------------------------------------------------------------------------------------------
#include <thread>
#include <chrono>

//----------------------------------------------------------------------------------------------------
InputSystem* g_input = nullptr;

//----------------------------------------------------------------------------------------------------
InputSystem::InputSystem(sInputSystemConfig const& config)
	: m_config(config)
	, m_nextJobId(1)
{
}

//----------------------------------------------------------------------------------------------------
void InputSystem::Startup()
{
    SubscribeEventCallbackFunction("OnWindowKeyPressed", OnWindowKeyPressed);
    SubscribeEventCallbackFunction("OnWindowKeyReleased", OnWindowKeyReleased);

    for (int controllerIndex = 0; controllerIndex < NUM_XBOX_CONTROLLERS; ++controllerIndex)
    {
        m_controllers[controllerIndex].m_id = controllerIndex;
    }
}

//----------------------------------------------------------------------------------------------------
void InputSystem::Shutdown()
{
	// No thread cleanup needed - threads are detached
}

//----------------------------------------------------------------------------------------------------
void InputSystem::BeginFrame()
{
    for (int controllerIndex = 0; controllerIndex < NUM_XBOX_CONTROLLERS; ++controllerIndex)
    {
        m_controllers[controllerIndex].Update();
    }

    // Check if our hidden mode matches Windows cursor state
    static bool cursorHidden     = false;
    bool const  shouldHideCursor = m_cursorState.m_cursorMode == eCursorMode::FPS;

    if (shouldHideCursor != cursorHidden)
    {
        while (ShowCursor(!shouldHideCursor) >= 0 && shouldHideCursor)
        {
        }
        while (ShowCursor(!shouldHideCursor) < 0 && !shouldHideCursor)
        {
        }
        cursorHidden = shouldHideCursor;
    }

    // Save off the previous cursor client position from last frame.
    IntVec2 const previousCursorClientPosition = m_cursorState.m_cursorClientPosition;

    // Get the current cursor client position from Windows.
    POINT currentCursorPosition;
    GetCursorPos(&currentCursorPosition);
    ScreenToClient(GetActiveWindow(), &currentCursorPosition);
    m_cursorState.m_cursorClientPosition.x = currentCursorPosition.x;
    m_cursorState.m_cursorClientPosition.y = currentCursorPosition.y;

    // If we are in relative mode
    if (m_cursorState.m_cursorMode == eCursorMode::FPS)
    {
        // Calculate our cursor client delta
        m_cursorState.m_cursorClientDelta = m_cursorState.m_cursorClientPosition - previousCursorClientPosition;

        // Set the Windows cursor position back to the center of our client region
        int const clientX = (int)Window::s_mainWindow->GetClientDimensions().x;
        int const clientY = (int)Window::s_mainWindow->GetClientDimensions().y;
        POINT     center  = {clientX / 2, clientY / 2};
        ClientToScreen(GetActiveWindow(), &center);
        SetCursorPos(center.x, center.y);

        // Get the Windows cursor position again and save that as our current cursor client position.
        POINT currentCursorPositionX;
        GetCursorPos(&currentCursorPositionX);
        ScreenToClient(GetActiveWindow(), &currentCursorPositionX);
        m_cursorState.m_cursorClientPosition.x = currentCursorPositionX.x;
        m_cursorState.m_cursorClientPosition.y = currentCursorPositionX.y;
    }
    else
    {
        m_cursorState.m_cursorClientDelta = IntVec2::ZERO;
    }

    // Enqueue cursor update for JS worker thread (every frame)
    if (m_frameEventQueue)
    {
        FrameEvent evt;
        evt.type      = eFrameEventType::CursorUpdate;
        evt.cursor.x  = static_cast<float>(m_cursorState.m_cursorClientPosition.x);
        evt.cursor.y  = static_cast<float>(m_cursorState.m_cursorClientPosition.y);
        evt.cursor.dx = static_cast<float>(m_cursorState.m_cursorClientDelta.x);
        evt.cursor.dy = static_cast<float>(m_cursorState.m_cursorClientDelta.y);
        m_frameEventQueue->Submit(evt);
    }
}

//----------------------------------------------------------------------------------------------------
void InputSystem::EndFrame()
{
	auto now = std::chrono::steady_clock::now();

	// Phase 6a: Process active key hold jobs
	auto it = m_activeKeyHolds.begin();
	while (it != m_activeKeyHolds.end())
	{
		auto& [jobId, job] = *it;

		// Step 1: Check if it's time to press the key (and hasn't been pressed yet)
		if (!job.hasPressed && !job.isCancelled && now >= job.startTime)
		{
			// Time to press this key
			INPUT inputDown = {};
			inputDown.type = INPUT_KEYBOARD;
			inputDown.ki.wVk = job.keyCode;
			inputDown.ki.wScan = job.scanCode;
			inputDown.ki.dwFlags = 0;  // 0 = KEY_DOWN
			inputDown.ki.time = 0;
			inputDown.ki.dwExtraInfo = 0;

			UINT result = SendInput(1, &inputDown, sizeof(INPUT));

			// CRITICAL: Update internal key state directly (SendInput doesn't trigger our WM_KEYDOWN handler)
			HandleKeyPressed(job.keyCode);

			job.hasPressed = true;  // Mark as pressed

			DAEMON_LOG(LogScript, eLogVerbosity::Log,
				StringFormat("InputSystem: [EndFrame] Pressed key for job {}, result={} (keyCode={})",
							 jobId, result, job.keyCode));
		}

		// Step 2: Check if job is cancelled or time to release
		if (job.isCancelled || now >= job.releaseTime)
		{
			if (!job.isCancelled && job.hasPressed)
			{
				// Time to release this key (only if it was actually pressed)
				INPUT inputUp = {};
				inputUp.type = INPUT_KEYBOARD;
				inputUp.ki.wVk = job.keyCode;
				inputUp.ki.wScan = job.scanCode;
				inputUp.ki.dwFlags = KEYEVENTF_KEYUP;
				inputUp.ki.time = 0;
				inputUp.ki.dwExtraInfo = 0;

				UINT result = SendInput(1, &inputUp, sizeof(INPUT));

				// CRITICAL: Update internal key state directly (SendInput doesn't trigger our WM_KEYUP handler)
				HandleKeyReleased(job.keyCode);

				DAEMON_LOG(LogScript, eLogVerbosity::Log,
					StringFormat("InputSystem: [EndFrame] Released key for job {}, result={} (keyCode={})",
								 jobId, result, job.keyCode));
			}
			else if (job.isCancelled)
			{
				// If cancelled after pressing, release the key state
				if (job.hasPressed)
				{
					HandleKeyReleased(job.keyCode);
				}

				DAEMON_LOG(LogScript, eLogVerbosity::Log,
					StringFormat("InputSystem: [EndFrame] Cancelled job {} (keyCode={}, wasPressed={})",
								 jobId, job.keyCode, job.hasPressed));
			}

			// Mark job as completed and move to completed jobs list
			m_completedJobs[jobId] = now;
			it = m_activeKeyHolds.erase(it);
		}
		else
		{
			++it;
		}
	}

	// Phase 6a: Clean up completed jobs older than 30 seconds
	auto completedIt = m_completedJobs.begin();
	while (completedIt != m_completedJobs.end())
	{
		const auto& [completedJobId, completedTime] = *completedIt;
		if (now - completedTime > std::chrono::seconds(30))
		{
			completedIt = m_completedJobs.erase(completedIt);
		}
		else
		{
			++completedIt;
		}
	}

	for (int keyCode = 0; keyCode < NUM_KEYCODES; ++keyCode)
	{
		m_keyStates[keyCode].m_wasKeyDownLastFrame = m_keyStates[keyCode].m_isKeyDown;
	}
}

bool InputSystem::WasKeyJustPressed(unsigned char const keyCode) const
{
    return
        m_keyStates[keyCode].m_isKeyDown &&
        !m_keyStates[keyCode].m_wasKeyDownLastFrame;
}

//----------------------------------------------------------------------------------------------------
bool InputSystem::WasKeyJustReleased(unsigned char const keyCode) const
{
    return
        !m_keyStates[keyCode].m_isKeyDown &&
        m_keyStates[keyCode].m_wasKeyDownLastFrame;
}

//----------------------------------------------------------------------------------------------------
bool InputSystem::IsKeyDown(unsigned char const keyCode) const
{
    return m_keyStates[keyCode].m_isKeyDown;
}

//----------------------------------------------------------------------------------------------------
void InputSystem::HandleKeyPressed(unsigned char const keyCode)
{
    m_keyStates[keyCode].m_isKeyDown = true;

    // Enqueue event for JS worker thread (if FrameEventQueue is connected)
    if (m_frameEventQueue)
    {
        FrameEvent evt;
        evt.type        = eFrameEventType::KeyDown;
        evt.key.keyCode = keyCode;
        m_frameEventQueue->Submit(evt);
    }
}

//----------------------------------------------------------------------------------------------------
void InputSystem::HandleKeyReleased(unsigned char const keyCode)
{
    m_keyStates[keyCode].m_isKeyDown = false;

    // Enqueue event for JS worker thread (if FrameEventQueue is connected)
    if (m_frameEventQueue)
    {
        FrameEvent evt;
        evt.type        = eFrameEventType::KeyUp;
        evt.key.keyCode = keyCode;
        m_frameEventQueue->Submit(evt);
    }
}

//----------------------------------------------------------------------------------------------------
void InputSystem::SetFrameEventQueue(FrameEventQueue* queue)
{
    m_frameEventQueue = queue;
}

//----------------------------------------------------------------------------------------------------
XboxController const& InputSystem::GetController(int const controllerID)
{
    return m_controllers[controllerID];
}

//----------------------------------------------------------------------------------------------------
// In pointer mode, the cursor should be visible, freely able to move, and not
// locked to the window. In FPS mode, the cursor should be hidden, reset to the
// center of the window each frame, and record the delta each frame.
//
void InputSystem::SetCursorMode(eCursorMode const mode)
{
    m_cursorState.m_cursorMode = mode;
}

//----------------------------------------------------------------------------------------------------
// Returns the current frame cursor delta in pixels, relative to the client
// region. This is how much the cursor moved last frame before it was reset
// to the center of the screen. Only valid in FPS mode, will be zero otherwise.
//
Vec2 InputSystem::GetCursorClientDelta() const
{
    switch (m_cursorState.m_cursorMode)
    {
    case eCursorMode::POINTER: return Vec2::ZERO;
    case eCursorMode::FPS: return static_cast<Vec2>(m_cursorState.m_cursorClientDelta);
    }

    return Vec2::ZERO;
}

//----------------------------------------------------------------------------------------------------
// Returns the cursor position, in pixels relative to the client region.
//
Vec2 InputSystem::GetCursorClientPosition() const
{
    return static_cast<Vec2>(m_cursorState.m_cursorClientPosition);
}

//----------------------------------------------------------------------------------------------------
// Returns the cursor position, normalized to the range [0, 1], relative
// to the client region, with the y-axis inverted to map from Windows
// conventions to game screen camera conventions.
//
Vec2 InputSystem::GetCursorNormalizedPosition() const
{
    RECT clientRect;
    GetClientRect(GetActiveWindow(), &clientRect);

    Vec2 const  clientPosition = Vec2(m_cursorState.m_cursorClientPosition);
    float const normalizedX    = clientPosition.x / static_cast<float>(clientRect.right);
    float const normalizedY    = clientPosition.y / static_cast<float>(clientRect.bottom);

    Vec2 cursorPosition = Vec2(normalizedX, 1.f - normalizedY);

    return cursorPosition;
}

//----------------------------------------------------------------------------------------------------
STATIC bool InputSystem::OnWindowKeyPressed(EventArgs& args)
{
    // if (g_theDevConsole == nullptr)
    // {
    //     ERROR_RECOVERABLE("g_theDevConsole is nullptr")
    // }

    if (g_input == nullptr)
    {
        return false;
    }

    int const           value   = args.GetValue("OnWindowKeyPressed", -1);
    unsigned char const keyCode = static_cast<unsigned char>(value);
    g_input->HandleKeyPressed(keyCode);

    return true;
}

//----------------------------------------------------------------------------------------------------
STATIC bool InputSystem::OnWindowKeyReleased(EventArgs& args)
{
    // if (g_theDevConsole == nullptr)
    // {
    //     return false;
    // }

    if (g_input == nullptr)
    {
        return false;
    }

    // if (g_theDevConsole->IsOpen())
    // {
    //     return false;
    // }

    int const           value   = args.GetValue("OnWindowKeyReleased", -1);
    unsigned char const keyCode = static_cast<unsigned char>(value);
    g_input->HandleKeyReleased(keyCode);

    return true;
}

//----------------------------------------------------------------------------------------------------
// Phase 6a: KADI Development Tools - Input Injection Implementation
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------
void InputSystem::InjectKeyPress(unsigned char keyCode, int durationMs)
{
    DAEMON_LOG(LogScript, eLogVerbosity::Log,
        StringFormat("InputSystem: Injecting key press for keyCode={}, duration={}ms", keyCode, durationMs));

    // Convert virtual key code to scan code
    UINT scanCode = MapVirtualKeyA(keyCode, MAPVK_VK_TO_VSC);

    // Prepare key down event
    INPUT inputDown = {};
    inputDown.type = INPUT_KEYBOARD;
    inputDown.ki.wVk = keyCode;
    inputDown.ki.wScan = static_cast<WORD>(scanCode);
    inputDown.ki.dwFlags = 0;  // Key down
    inputDown.ki.time = 0;
    inputDown.ki.dwExtraInfo = 0;

    // Prepare key up event
    INPUT inputUp = {};
    inputUp.type = INPUT_KEYBOARD;
    inputUp.ki.wVk = keyCode;
    inputUp.ki.wScan = static_cast<WORD>(scanCode);
    inputUp.ki.dwFlags = KEYEVENTF_KEYUP;
    inputUp.ki.time = 0;
    inputUp.ki.dwExtraInfo = 0;

    // Send key down
    UINT result = SendInput(1, &inputDown, sizeof(INPUT));
    if (result != 1)
    {
        DAEMON_LOG(LogScript, eLogVerbosity::Error,
            StringFormat("InputSystem: SendInput failed for key down (keyCode={})", keyCode));
        return;
    }

    // CRITICAL: Update internal key state directly (SendInput doesn't trigger our WM_KEYDOWN handler)
    HandleKeyPressed(keyCode);

    // Wait for duration
    if (durationMs > 0)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(durationMs));
    }

    // Send key up
    result = SendInput(1, &inputUp, sizeof(INPUT));
    if (result != 1)
    {
        DAEMON_LOG(LogScript, eLogVerbosity::Error,
            StringFormat("InputSystem: SendInput failed for key up (keyCode={})", keyCode));
        return;
    }

    // CRITICAL: Update internal key state directly (SendInput doesn't trigger our WM_KEYUP handler)
    HandleKeyReleased(keyCode);

    DAEMON_LOG(LogScript, eLogVerbosity::Log,
        StringFormat("InputSystem: Key press injection completed for keyCode={}", keyCode));
}

//----------------------------------------------------------------------------------------------------
uint32_t InputSystem::InjectKeyHold(unsigned char keyCode, int durationMs, bool repeat)
{
	UNUSED(repeat); // Parameter kept for backward compatibility but not used in current implementation

	// Generate unique job ID
	uint32_t jobId = m_nextJobId++;

	DAEMON_LOG(LogScript, eLogVerbosity::Log,
		StringFormat("InputSystem: InjectKeyHold - jobId={}, keyCode={}, duration={}ms (ENHANCED V5)",
					 jobId, keyCode, durationMs));

	// Convert virtual key code to scan code
	UINT scanCode = MapVirtualKeyA(keyCode, MAPVK_VK_TO_VSC);

	// Prepare key down event
	INPUT inputDown = {};
	inputDown.type = INPUT_KEYBOARD;
	inputDown.ki.wVk = keyCode;
	inputDown.ki.wScan = static_cast<WORD>(scanCode);
	inputDown.ki.dwFlags = 0;
	inputDown.ki.time = 0;
	inputDown.ki.dwExtraInfo = 0;

	// Send key down IMMEDIATELY
	UINT result = SendInput(1, &inputDown, sizeof(INPUT));
	if (result != 1)
	{
		DAEMON_LOG(LogScript, eLogVerbosity::Error,
			StringFormat("InputSystem: SendInput failed for key down (jobId={}, keyCode={})", jobId, keyCode));
		return 0;  // Return 0 to indicate failure
	}

	// CRITICAL: Update internal key state directly (SendInput doesn't trigger our WM_KEYDOWN handler)
	HandleKeyPressed(keyCode);

	// Create job tracking record
	auto startTime = std::chrono::steady_clock::now();
	auto releaseTime = startTime + std::chrono::milliseconds(durationMs);

	sKeyHoldJob job;
	job.jobId = jobId;
	job.keyCode = keyCode;
	job.scanCode = static_cast<WORD>(scanCode);
	job.startTime = startTime;
	job.releaseTime = releaseTime;
	job.completedTime = std::chrono::steady_clock::time_point{};
	job.totalDurationMs = durationMs;
	job.isCancelled = false;
	job.hasPressed = true;  // KEY_DOWN already sent immediately above

	m_activeKeyHolds[jobId] = job;

	DAEMON_LOG(LogScript, eLogVerbosity::Log,
		StringFormat("InputSystem: Key down sent, job {} created, will release after {}ms (keyCode={})",
					 jobId, durationMs, keyCode));

	return jobId;  // Return job ID immediately
}

//----------------------------------------------------------------------------------------------------
// Phase 6a: Enhanced multi-key sequence injection
uint32_t InputSystem::InjectKeySequence(std::vector<sKeySequenceItem> keySequence)
{
	// Generate unique job ID for the sequence
	uint32_t primaryJobId = m_nextJobId++;

	DAEMON_LOG(LogScript, eLogVerbosity::Log,
		StringFormat("InputSystem: InjectKeySequence - jobId={}, keyCount={}",
			primaryJobId, keySequence.size()));

	auto sequenceStartTime = std::chrono::steady_clock::now();
	std::vector<uint32_t> jobIds;

	// Process each key in the sequence
	for (const auto& keyItem : keySequence)
	{
		// Generate individual job ID for this key
		uint32_t keyJobId = m_nextJobId++;
		jobIds.push_back(keyJobId);

		// Calculate timing for this key
		auto keyPressTime = sequenceStartTime + std::chrono::milliseconds(keyItem.delayMs);
		auto keyReleaseTime = sequenceStartTime + std::chrono::milliseconds(keyItem.delayMs + keyItem.durationMs);

		// Convert virtual key code to scan code
		UINT scanCode = MapVirtualKeyA(keyItem.keyCode, MAPVK_VK_TO_VSC);

		// Create individual key job
		sKeyHoldJob job;
		job.jobId = keyJobId;
		job.keyCode = keyItem.keyCode;
		job.scanCode = static_cast<WORD>(scanCode);
		job.startTime = keyPressTime;
		job.releaseTime = keyReleaseTime;
		job.completedTime = std::chrono::steady_clock::time_point{};
		job.totalDurationMs = keyItem.durationMs;
		job.isCancelled = false;
		job.hasPressed = false;  // Initialize as not pressed yet

		// Add to active jobs with delayed timing
		m_activeKeyHolds[keyJobId] = job;

		DAEMON_LOG(LogScript, eLogVerbosity::Log,
			StringFormat("InputSystem: Key {} added to sequence - delay={}ms, duration={}ms, job={}",
				static_cast<int>(keyItem.keyCode), keyItem.delayMs, keyItem.durationMs, keyJobId));
	}

	// Store sequence metadata for tracking (optional: could add sequence tracking)
	// For now, return the primary job ID for sequence tracking
	DAEMON_LOG(LogScript, eLogVerbosity::Log,
		StringFormat("InputSystem: Key sequence {} created with {} individual keys",
				primaryJobId, keySequence.size()));

	return primaryJobId;
}

sToolJobStatus InputSystem::GetKeyHoldStatus(uint32_t jobId) const
{
	sToolJobStatus status;
	status.jobId = jobId;
	status.toolType = "keyhold";
	
	// Check active jobs
	auto activeIt = m_activeKeyHolds.find(jobId);
	if (activeIt != m_activeKeyHolds.end())
	{
		const auto& job = activeIt->second;
		status.status = job.isCancelled ? eToolJobStatus::Cancelled : eToolJobStatus::Active;
		
		auto now = std::chrono::steady_clock::now();
		auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - job.startTime).count();
		auto remaining = std::chrono::duration_cast<std::chrono::milliseconds>(job.releaseTime - now).count();
		
		status.metadata["keyCode"] = std::to_string(job.keyCode);
		status.metadata["elapsedMs"] = std::to_string(elapsed);
		status.metadata["remainingMs"] = std::to_string(remaining > 0 ? remaining : 0);
		status.metadata["totalDurationMs"] = std::to_string(job.totalDurationMs);
		return status;
	}
	
	// Check completed jobs
	auto completedIt = m_completedJobs.find(jobId);
	if (completedIt != m_completedJobs.end())
	{
		status.status = eToolJobStatus::Completed;
		status.metadata["completed"] = "true";
		return status;
	}
	
	// Job not found
	status.status = eToolJobStatus::NotFound;
	status.metadata["error"] = "Job not found";
	return status;
}

//----------------------------------------------------------------------------------------------------
bool InputSystem::CancelKeyHold(uint32_t jobId)
{
	auto it = m_activeKeyHolds.find(jobId);
	if (it != m_activeKeyHolds.end())
	{
		// Mark job as cancelled - EndFrame will handle the actual key release
		it->second.isCancelled = true;
		
		DAEMON_LOG(LogScript, eLogVerbosity::Log,
			StringFormat("InputSystem: Cancelled job {} (keyCode={})", jobId, it->second.keyCode));
		
		return true;
	}
	
	return false;
}

//----------------------------------------------------------------------------------------------------
std::vector<sToolJobStatus> InputSystem::ListActiveKeyHolds() const
{
	std::vector<sToolJobStatus> activeJobs;
	activeJobs.reserve(m_activeKeyHolds.size());
	
	for (const auto& [jobId, job] : m_activeKeyHolds)
	{
		if (!job.isCancelled)
		{
			activeJobs.push_back(GetKeyHoldStatus(jobId));
		}
	}
	
	return activeJobs;
}