//----------------------------------------------------------------------------------------------------
// InputScriptInterface.cpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#include "Engine/Input/InputScriptInterface.hpp"
//----------------------------------------------------------------------------------------------------
#include "Engine/Core/EngineCommon.hpp"
#include "Engine/Core/ErrorWarningAssert.hpp"
#include "Engine/Core/LogSubsystem.hpp"
#include "Engine/Input/InputSystem.hpp"
#include "Engine/Script/ScriptTypeExtractor.hpp"

//----------------------------------------------------------------------------------------------------
InputScriptInterface::InputScriptInterface(InputSystem* inputSystem)
    : m_inputSystem(inputSystem)
{
    if (!m_inputSystem)
    {
        ERROR_AND_DIE("InputScriptInterface: InputSystem pointer cannot be null")
    }

    // Initialize method registry for efficient dispatch
    InputScriptInterface::InitializeMethodRegistry();
}

//----------------------------------------------------------------------------------------------------
void InputScriptInterface::InitializeMethodRegistry()
{
    m_methodRegistry["isKeyPressed"]              = [this](ScriptArgs const& args) { return ExecuteIsKeyPressed(args); };
    m_methodRegistry["wasKeyJustPressed"]         = [this](ScriptArgs const& args) { return ExecuteWasKeyJustPressed(args); };
    m_methodRegistry["wasKeyJustReleased"]        = [this](ScriptArgs const& args) { return ExecuteWasKeyJustReleased(args); };
    m_methodRegistry["getMousePosition"]          = [this](ScriptArgs const& args) { return ExecuteGetMousePosition(args); };
    m_methodRegistry["isMouseButtonPressed"]      = [this](ScriptArgs const& args) { return ExecuteIsMouseButtonPressed(args); };
    m_methodRegistry["wasMouseButtonJustPressed"] = [this](ScriptArgs const& args) { return ExecuteWasMouseButtonJustPressed(args); };
    m_methodRegistry["getMouseDelta"]             = [this](ScriptArgs const& args) { return ExecuteGetMouseDelta(args); };
    m_methodRegistry["isControllerConnected"]     = [this](ScriptArgs const& args) { return ExecuteIsControllerConnected(args); };
    m_methodRegistry["getControllerAxis"]         = [this](ScriptArgs const& args) { return ExecuteGetControllerAxis(args); };
    m_methodRegistry["isControllerButtonPressed"] = [this](ScriptArgs const& args) { return ExecuteIsControllerButtonPressed(args); };

    // === LEGACY METHODS (for backward compatibility) ===
    m_methodRegistry["isKeyDown"]               = [this](ScriptArgs const& args) { return ExecuteIsKeyDown(args); };
    m_methodRegistry["getCursorClientDelta"]    = [this](ScriptArgs const& args) { return ExecuteGetCursorClientDelta(args); };
    m_methodRegistry["getCursorClientPosition"] = [this](ScriptArgs const& args) { return ExecuteGetCursorClientPosition(args); };
    m_methodRegistry["getController"]           = [this](ScriptArgs const& args) { return ExecuteGetController(args); };
    m_methodRegistry["setCursorMode"]           = [this](ScriptArgs const& args) { return ExecuteSetCursorMode(args); };
}

//----------------------------------------------------------------------------------------------------
std::vector<ScriptMethodInfo> InputScriptInterface::GetAvailableMethods() const
{
    return {
        // === NEW DAEMON.INPUT API (following security whitelist) ===
        ScriptMethodInfo("isKeyPressed",
                         "Check if a key is currently being held down",
                         {"int"},
                         "bool"),

        ScriptMethodInfo("wasKeyJustPressed",
                         "Check if a key was just pressed this frame",
                         {"int"},
                         "bool"),

        ScriptMethodInfo("wasKeyJustReleased",
                         "Check if a key was just released this frame",
                         {"int"},
                         "bool"),

        ScriptMethodInfo("getMousePosition",
                         "Get current mouse position as {x, y} object",
                         {},
                         "object"),

        ScriptMethodInfo("isMouseButtonPressed",
                         "Check if a mouse button is currently pressed",
                         {"int"},
                         "bool"),

        ScriptMethodInfo("wasMouseButtonJustPressed",
                         "Check if a mouse button was just pressed this frame",
                         {"int"},
                         "bool"),

        ScriptMethodInfo("getMouseDelta",
                         "Get mouse movement delta since last frame as {x, y} object",
                         {},
                         "object"),

        ScriptMethodInfo("isControllerConnected",
                         "Check if a controller is connected",
                         {"int"},
                         "bool"),

        ScriptMethodInfo("getControllerAxis",
                         "Get controller axis value (-1.0 to 1.0)",
                         {"int", "int"},
                         "number"),

        ScriptMethodInfo("isControllerButtonPressed",
                         "Check if a controller button is pressed",
                         {"int", "int"},
                         "bool"),

        // === LEGACY METHODS (for backward compatibility) ===
        ScriptMethodInfo("isKeyDown",
                         "Check if a key is currently being held down (legacy)",
                         {"int"},
                         "bool"),

        ScriptMethodInfo("getCursorClientDelta",
                         "Get cursor client delta (legacy)",
                         {},
                         "object")
    };
}

//----------------------------------------------------------------------------------------------------
std::vector<String> InputScriptInterface::GetAvailableProperties() const
{
    DAEMON_LOG(LogScript, eLogVerbosity::Warning, StringFormat("(IScriptableObject::GetAvailableProperties) There is no available properties defined!"));

    return {
        "input",             // The input sub-object
        "cursorDelta",       // Legacy properties
        "cursorPosition"
    };
}

//----------------------------------------------------------------------------------------------------
ScriptMethodResult InputScriptInterface::CallMethod(String const&     methodName,
                                                    ScriptArgs const& args)
{
    try
    {
        // Use method registry for O(1) lookup instead of O(n) if-else chain
        auto it = m_methodRegistry.find(methodName);
        if (it != m_methodRegistry.end())
        {
            return it->second(args);
        }

        return ScriptMethodResult::Error("Unknown method: " + methodName);
    }
    catch (std::exception const& e)
    {
        return ScriptMethodResult::Error("Method execution failed: " + String(e.what()));
    }
}

//----------------------------------------------------------------------------------------------------
std::any InputScriptInterface::GetProperty(const String& propertyName) const
{
    if (propertyName == "input")
    {
        // Return a string representation of the input object
        // This approach might need to be changed depending on how the scripting system handles nested objects
        return String("{ "
            "isKeyPressed: function(keyCode) { return daemon['input.isKeyPressed'](keyCode); }, "
            "wasKeyJustPressed: function(keyCode) { return daemon['input.wasKeyJustPressed'](keyCode); }, "
            "wasKeyJustReleased: function(keyCode) { return daemon['input.wasKeyJustReleased'](keyCode); }, "
            "getMousePosition: function() { return daemon['input.getMousePosition'](); }, "
            "isMouseButtonPressed: function(button) { return daemon['input.isMouseButtonPressed'](button); }, "
            "wasMouseButtonJustPressed: function(button) { return daemon['input.wasMouseButtonJustPressed'](button); }, "
            "getMouseDelta: function() { return daemon['input.getMouseDelta'](); }, "
            "isControllerConnected: function(index) { return daemon['input.isControllerConnected'](index); }, "
            "getControllerAxis: function(index, axis) { return daemon['input.getControllerAxis'](index, axis); }, "
            "isControllerButtonPressed: function(index, button) { return daemon['input.isControllerButtonPressed'](index, button); } "
            "}");
    }
    else if (propertyName == "cursorDelta")
    {
        Vec2 delta = m_inputSystem->GetCursorClientDelta();
        return String("{ x: " + std::to_string(delta.x) + ", y: " + std::to_string(delta.y) + " }");
    }
    else if (propertyName == "cursorPosition")
    {
        Vec2 position = m_inputSystem->GetCursorClientPosition();
        return String("{ x: " + std::to_string(position.x) + ", y: " + std::to_string(position.y) + " }");
    }

    return std::any{};
}

//----------------------------------------------------------------------------------------------------
bool InputScriptInterface::SetProperty(const String& propertyName, const std::any& value)
{
    // InputSystem 目前沒有可設定的屬性
    UNUSED(propertyName)
    UNUSED(value)
    return false;
}

//----------------------------------------------------------------------------------------------------
ScriptMethodResult InputScriptInterface::ExecuteIsKeyDown(ScriptArgs const& args)
{
    auto result = ScriptTypeExtractor::ValidateArgCount(args, 1, "isKeyDown");
    if (!result.success) return result;

    try
    {
        int  keyCode = ScriptTypeExtractor::ExtractInt(args[0]);
        bool isDown  = m_inputSystem->IsKeyDown((char)keyCode);
        return ScriptMethodResult::Success(isDown);
    }
    catch (const std::exception& e)
    {
        return ScriptMethodResult::Error("檢查按鍵狀態失敗: " + String(e.what()));
    }
}

//----------------------------------------------------------------------------------------------------
ScriptMethodResult InputScriptInterface::ExecuteWasKeyJustPressed(ScriptArgs const& args)
{
    auto result = ScriptTypeExtractor::ValidateArgCount(args, 1, "wasKeyJustPressed");
    if (!result.success) return result;

    try
    {
        int  keyCode    = ScriptTypeExtractor::ExtractInt(args[0]);
        bool wasPressed = m_inputSystem->WasKeyJustPressed((char)keyCode);
        return ScriptMethodResult::Success(wasPressed);
    }
    catch (const std::exception& e)
    {
        return ScriptMethodResult::Error("檢查按鍵剛按下狀態失敗: " + String(e.what()));
    }
}

//----------------------------------------------------------------------------------------------------
ScriptMethodResult InputScriptInterface::ExecuteWasKeyJustReleased(ScriptArgs const& args)
{
    auto result = ScriptTypeExtractor::ValidateArgCount(args, 1, "wasKeyJustReleased");
    if (!result.success) return result;

    try
    {
        int  keyCode     = ScriptTypeExtractor::ExtractInt(args[0]);
        bool wasReleased = m_inputSystem->WasKeyJustReleased((char)keyCode);
        return ScriptMethodResult::Success(wasReleased);
    }
    catch (const std::exception& e)
    {
        return ScriptMethodResult::Error("檢查按鍵剛釋放狀態失敗: " + String(e.what()));
    }
}

//----------------------------------------------------------------------------------------------------
ScriptMethodResult InputScriptInterface::ExecuteGetCursorClientDelta(ScriptArgs const& args)
{
    auto result = ScriptTypeExtractor::ValidateArgCount(args, 0, "getCursorClientDelta");
    if (!result.success) return result;

    try
    {
        Vec2   delta    = m_inputSystem->GetCursorClientDelta();
        String deltaStr = "{ x: " + std::to_string(delta.x) + ", y: " + std::to_string(delta.y) + " }";
        return ScriptMethodResult::Success(deltaStr);
    }
    catch (const std::exception& e)
    {
        return ScriptMethodResult::Error("取得游標移動增量失敗: " + String(e.what()));
    }
}

//----------------------------------------------------------------------------------------------------
ScriptMethodResult InputScriptInterface::ExecuteGetCursorClientPosition(ScriptArgs const& args)
{
    auto result = ScriptTypeExtractor::ValidateArgCount(args, 0, "getCursorPosition");
    if (!result.success) return result;

    try
    {
        Vec2   position    = m_inputSystem->GetCursorClientPosition();
        String positionStr = "{ x: " + std::to_string(position.x) + ", y: " + std::to_string(position.y) + " }";
        return ScriptMethodResult::Success(positionStr);
    }
    catch (const std::exception& e)
    {
        return ScriptMethodResult::Error("取得游標位置失敗: " + String(e.what()));
    }
}

//----------------------------------------------------------------------------------------------------
ScriptMethodResult InputScriptInterface::ExecuteGetController(ScriptArgs const& args)
{
    auto result = ScriptTypeExtractor::ValidateArgCount(args, 1, "getController");
    if (!result.success) return result;

    try
    {
        int                   controllerIndex = ScriptTypeExtractor::ExtractInt(args[0]);
        XboxController const& controller      = m_inputSystem->GetController(controllerIndex);
        UNUSED(controller)
        // 簡化的控制器狀態回傳
        String controllerStr = "{ index: " + std::to_string(controllerIndex) + ", connected: true }";
        return ScriptMethodResult::Success(controllerStr);
    }
    catch (const std::exception& e)
    {
        return ScriptMethodResult::Error("取得控制器失敗: " + String(e.what()));
    }
}

//----------------------------------------------------------------------------------------------------
ScriptMethodResult InputScriptInterface::ExecuteSetCursorMode(ScriptArgs const& args)
{
    auto result = ScriptTypeExtractor::ValidateArgCount(args, 1, "setCursorMode");
    if (!result.success) return result;

    try
    {
        int         mode       = ScriptTypeExtractor::ExtractInt(args[0]);
        eCursorMode cursorMode = static_cast<eCursorMode>(mode);
        m_inputSystem->SetCursorMode(cursorMode);
        return ScriptMethodResult::Success(String("游標模式已設定為: " + std::to_string(mode)));
    }
    catch (const std::exception& e)
    {
        return ScriptMethodResult::Error("設定游標模式失敗: " + String(e.what()));
    }
}

//----------------------------------------------------------------------------------------------------
// === NEW DAEMON.INPUT API IMPLEMENTATIONS ===
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
ScriptMethodResult InputScriptInterface::ExecuteIsKeyPressed(ScriptArgs const& args)
{
    auto result = ScriptTypeExtractor::ValidateArgCount(args, 1, "isKeyPressed");
    if (!result.success) return result;

    try
    {
        int keyCode = ScriptTypeExtractor::ExtractInt(args[0]);
        if (!ValidateKeyCode(keyCode))
        {
            return ScriptMethodResult::Error("Invalid key code: " + std::to_string(keyCode));
        }

        bool isPressed = m_inputSystem->IsKeyDown(static_cast<unsigned char>(keyCode));
        return ScriptMethodResult::Success(isPressed);
    }
    catch (const std::exception& e)
    {
        return ScriptMethodResult::Error("Failed to check key pressed state: " + String(e.what()));
    }
}

//----------------------------------------------------------------------------------------------------
ScriptMethodResult InputScriptInterface::ExecuteGetMousePosition(ScriptArgs const& args)
{
    auto result = ScriptTypeExtractor::ValidateArgCount(args, 0, "getMousePosition");
    if (!result.success) return result;

    try
    {
        Vec2   position    = m_inputSystem->GetCursorClientPosition();
        String positionStr = "{ x: " + std::to_string(position.x) + ", y: " + std::to_string(position.y) + " }";
        return ScriptMethodResult::Success(positionStr);
    }
    catch (const std::exception& e)
    {
        return ScriptMethodResult::Error("Failed to get mouse position: " + String(e.what()));
    }
}

//----------------------------------------------------------------------------------------------------
ScriptMethodResult InputScriptInterface::ExecuteIsMouseButtonPressed(ScriptArgs const& args)
{
    auto result = ScriptTypeExtractor::ValidateArgCount(args, 1, "isMouseButtonPressed");
    if (!result.success) return result;

    try
    {
        int button = ScriptTypeExtractor::ExtractInt(args[0]);
        if (!ValidateMouseButton(button))
        {
            return ScriptMethodResult::Error("Invalid mouse button: " + std::to_string(button));
        }

        unsigned char keyCode = KEYCODE_LEFT_MOUSE;
        switch (button)
        {
        case 0: keyCode = KEYCODE_LEFT_MOUSE;
            break;
        case 1: keyCode = KEYCODE_RIGHT_MOUSE;
            break;
        case 2:
            // Middle mouse button not supported in current InputSystem
            return ScriptMethodResult::Error("Middle mouse button not supported");
        default:
            return ScriptMethodResult::Error("Invalid mouse button: " + std::to_string(button));
        }

        bool isPressed = m_inputSystem->IsKeyDown(keyCode);
        return ScriptMethodResult::Success(isPressed);
    }
    catch (const std::exception& e)
    {
        return ScriptMethodResult::Error("Failed to check mouse button state: " + String(e.what()));
    }
}

//----------------------------------------------------------------------------------------------------
ScriptMethodResult InputScriptInterface::ExecuteWasMouseButtonJustPressed(ScriptArgs const& args)
{
    auto result = ScriptTypeExtractor::ValidateArgCount(args, 1, "wasMouseButtonJustPressed");
    if (!result.success) return result;

    try
    {
        int button = ScriptTypeExtractor::ExtractInt(args[0]);
        if (!ValidateMouseButton(button))
        {
            return ScriptMethodResult::Error("Invalid mouse button: " + std::to_string(button));
        }

        unsigned char keyCode = KEYCODE_LEFT_MOUSE;
        switch (button)
        {
        case 0: keyCode = KEYCODE_LEFT_MOUSE;
            break;
        case 1: keyCode = KEYCODE_RIGHT_MOUSE;
            break;
        case 2:
            return ScriptMethodResult::Error("Middle mouse button not supported");
        default:
            return ScriptMethodResult::Error("Invalid mouse button: " + std::to_string(button));
        }

        bool wasPressed = m_inputSystem->WasKeyJustPressed(keyCode);
        return ScriptMethodResult::Success(wasPressed);
    }
    catch (const std::exception& e)
    {
        return ScriptMethodResult::Error("Failed to check mouse button just pressed: " + String(e.what()));
    }
}

//----------------------------------------------------------------------------------------------------
ScriptMethodResult InputScriptInterface::ExecuteGetMouseDelta(ScriptArgs const& args)
{
    auto result = ScriptTypeExtractor::ValidateArgCount(args, 0, "getMouseDelta");
    if (!result.success) return result;

    try
    {
        Vec2   delta    = m_inputSystem->GetCursorClientDelta();
        String deltaStr = "{ x: " + std::to_string(delta.x) + ", y: " + std::to_string(delta.y) + " }";
        return ScriptMethodResult::Success(deltaStr);
    }
    catch (const std::exception& e)
    {
        return ScriptMethodResult::Error("Failed to get mouse delta: " + String(e.what()));
    }
}

//----------------------------------------------------------------------------------------------------
ScriptMethodResult InputScriptInterface::ExecuteIsControllerConnected(ScriptArgs const& args)
{
    auto result = ScriptTypeExtractor::ValidateArgCount(args, 1, "isControllerConnected");
    if (!result.success) return result;

    try
    {
        int controllerIndex = ScriptTypeExtractor::ExtractInt(args[0]);
        if (!ValidateControllerIndex(controllerIndex))
        {
            return ScriptMethodResult::Error("Invalid controller index: " + std::to_string(controllerIndex));
        }

        const XboxController& controller  = m_inputSystem->GetController(controllerIndex);
        bool                  isConnected = controller.IsConnected();
        return ScriptMethodResult::Success(isConnected);
    }
    catch (const std::exception& e)
    {
        return ScriptMethodResult::Error("Failed to check controller connection: " + String(e.what()));
    }
}

//----------------------------------------------------------------------------------------------------
ScriptMethodResult InputScriptInterface::ExecuteGetControllerAxis(ScriptArgs const& args)
{
    auto result = ScriptTypeExtractor::ValidateArgCount(args, 2, "getControllerAxis");
    if (!result.success) return result;

    try
    {
        int controllerIndex = ScriptTypeExtractor::ExtractInt(args[0]);
        int axis            = ScriptTypeExtractor::ExtractInt(args[1]);

        if (!ValidateControllerIndex(controllerIndex))
        {
            return ScriptMethodResult::Error("Invalid controller index: " + std::to_string(controllerIndex));
        }

        if (!ValidateControllerAxis(axis))
        {
            return ScriptMethodResult::Error("Invalid controller axis: " + std::to_string(axis));
        }

        const XboxController& controller = m_inputSystem->GetController(controllerIndex);
        if (!controller.IsConnected())
        {
            return ScriptMethodResult::Success(0.0f);
        }

        float axisValue = 0.0f;
        switch (axis)
        {
        case 0: // LEFT_STICK_X
            axisValue = controller.GetLeftStick().GetPosition().x;
            break;
        case 1: // LEFT_STICK_Y
            axisValue = controller.GetLeftStick().GetPosition().y;
            break;
        case 2: // RIGHT_STICK_X
            axisValue = controller.GetRightStick().GetPosition().x;
            break;
        case 3: // RIGHT_STICK_Y
            axisValue = controller.GetRightStick().GetPosition().y;
            break;
        case 4: // LEFT_TRIGGER
            axisValue = controller.GetLeftTrigger();
            break;
        case 5: // RIGHT_TRIGGER
            axisValue = controller.GetRightTrigger();
            break;
        default:
            return ScriptMethodResult::Error("Invalid axis: " + std::to_string(axis));
        }

        return ScriptMethodResult::Success(axisValue);
    }
    catch (const std::exception& e)
    {
        return ScriptMethodResult::Error("Failed to get controller axis: " + String(e.what()));
    }
}

//----------------------------------------------------------------------------------------------------
ScriptMethodResult InputScriptInterface::ExecuteIsControllerButtonPressed(ScriptArgs const& args)
{
    auto result = ScriptTypeExtractor::ValidateArgCount(args, 2, "isControllerButtonPressed");
    if (!result.success) return result;

    try
    {
        int controllerIndex = ScriptTypeExtractor::ExtractInt(args[0]);
        int button          = ScriptTypeExtractor::ExtractInt(args[1]);

        if (!ValidateControllerIndex(controllerIndex))
        {
            return ScriptMethodResult::Error("Invalid controller index: " + std::to_string(controllerIndex));
        }

        if (!ValidateControllerButton(button))
        {
            return ScriptMethodResult::Error("Invalid controller button: " + std::to_string(button));
        }

        const XboxController& controller = m_inputSystem->GetController(controllerIndex);
        if (!controller.IsConnected())
        {
            return ScriptMethodResult::Success(false);
        }

        // Map our button enum to XboxButtonID
        eXboxButtonID buttonId = XBOX_BUTTON_INVALID;
        switch (button)
        {
        case 0: buttonId = XBOX_BUTTON_A;
            break;
        case 1: buttonId = XBOX_BUTTON_B;
            break;
        case 2: buttonId = XBOX_BUTTON_X;
            break;
        case 3: buttonId = XBOX_BUTTON_Y;
            break;
        case 4: buttonId = XBOX_BUTTON_LSHOULDER;
            break;
        case 5: buttonId = XBOX_BUTTON_RSHOULDER;
            break;
        case 6: buttonId = XBOX_BUTTON_BACK;
            break;
        case 7: buttonId = XBOX_BUTTON_START;
            break;
        case 8: buttonId = XBOX_BUTTON_LTHUMB;
            break;
        case 9: buttonId = XBOX_BUTTON_RTHUMB;
            break;
        case 10: buttonId = XBOX_BUTTON_DPAD_UP;
            break;
        case 11: buttonId = XBOX_BUTTON_DPAD_DOWN;
            break;
        case 12: buttonId = XBOX_BUTTON_DPAD_LEFT;
            break;
        case 13: buttonId = XBOX_BUTTON_DPAD_RIGHT;
            break;
        default:
            return ScriptMethodResult::Error("Invalid button: " + std::to_string(button));
        }

        bool isPressed = controller.IsButtonDown(buttonId);
        return ScriptMethodResult::Success(isPressed);
    }
    catch (const std::exception& e)
    {
        return ScriptMethodResult::Error("Failed to check controller button: " + String(e.what()));
    }
}

//----------------------------------------------------------------------------------------------------
// === VALIDATION AND SECURITY METHODS ===
//----------------------------------------------------------------------------------------------------

bool InputScriptInterface::ValidateKeyCode(int keyCode) const
{
    // Allow standard ASCII range and common special keys
    return (keyCode >= 0 && keyCode <= 255);
}

bool InputScriptInterface::ValidateMouseButton(int button) const
{
    // 0 = Left, 1 = Right, 2 = Middle (not fully supported)
    return (button >= 0 && button <= 2);
}

bool InputScriptInterface::ValidateControllerIndex(int index) const
{
    // Xbox controllers 0-3
    return (index >= 0 && index < NUM_XBOX_CONTROLLERS);
}

bool InputScriptInterface::ValidateControllerAxis(int axis) const
{
    // 0-5: LEFT_STICK_X, LEFT_STICK_Y, RIGHT_STICK_X, RIGHT_STICK_Y, LEFT_TRIGGER, RIGHT_TRIGGER
    return (axis >= 0 && axis <= 5);
}

bool InputScriptInterface::ValidateControllerButton(int button) const
{
    // 0-13: A, B, X, Y, LShoulder, RShoulder, Back, Start, LThumb, RThumb, DPad_Up, DPad_Down, DPad_Left, DPad_Right
    return (button >= 0 && button <= 13);
}
