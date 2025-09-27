//----------------------------------------------------------------------------------------------------
// inspector/Schema.h - Stub file for missing V8 inspector protocol
// This is a minimal stub to allow compilation when the full inspector protocol is not available  
//----------------------------------------------------------------------------------------------------

#ifndef INSPECTOR_SCHEMA_H_
#define INSPECTOR_SCHEMA_H_

// This is a stub implementation for missing V8 inspector protocol files
// The actual Chrome DevTools Protocol implementation is handled manually in V8InspectorSubsystem

namespace v8_inspector {
namespace protocol {
namespace Schema {
namespace API {

// Minimal stub classes to satisfy the v8-inspector-protocol.h includes
class Domain {
    // Empty stub class
};

} // namespace API
} // namespace Schema
} // namespace protocol
} // namespace v8_inspector

#endif // INSPECTOR_SCHEMA_H_