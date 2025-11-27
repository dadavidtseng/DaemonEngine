[Root Directory](../../../CLAUDE.md) > [Code](../../) > [ThirdParty](..) > **json**

# JSON Library Documentation (nlohmann/json)

## Library Overview

SimpleMiner uses the nlohmann/json library (also known as "JSON for Modern C++") for all JSON parsing and serialization. This is a header-only C++11 library providing intuitive JSON manipulation with STL-like interface.

**Location:** `C:\p4\Personal\SD\Engine\Code\ThirdParty\json\json.hpp`

**Assignment 7 Usage:** Primary library for all JSON-based registries (blocks, items, recipes) replacing XML format.

## Basic Usage

### Including the Library
```cpp
#include "ThirdParty/json/json.hpp"

using json = nlohmann::json;
```

### Parsing JSON from File
```cpp
// Read JSON file
std::ifstream file("Data/Definitions/BlockDefinitions.json");
json jsonData = json::parse(file);

// Access elements
std::string blockName = jsonData["blocks"][0]["name"];
bool isSolid = jsonData["blocks"][0]["solid"];
```

### Parsing JSON from String
```cpp
std::string jsonString = R"({"name": "stone", "solid": true})";
json obj = json::parse(jsonString);
```

### Creating JSON Objects
```cpp
json block;
block["name"] = "stone";
block["solid"] = true;
block["opaque"] = true;
block["topSprite"] = {1, 0};  // Array syntax
```

### Accessing Values
```cpp
// Direct access (throws if key doesn't exist)
std::string name = j["name"];

// Safe access with default
std::string name = j.value("name", "unknown");
bool solid = j.value("solid", false);

// Check if key exists
if (j.contains("name")) {
    // Key exists
}
```

### Iterating Over Arrays
```cpp
for (auto& block : jsonData["blocks"]) {
    std::string name = block["name"];
    bool solid = block["solid"];
    // Process block...
}
```

### Type Checking
```cpp
if (j.is_object()) { /* ... */ }
if (j.is_array()) { /* ... */ }
if (j.is_string()) { /* ... */ }
if (j.is_number()) { /* ... */ }
if (j.is_boolean()) { /* ... */ }
if (j.is_null()) { /* ... */ }
```

## Assignment 7 Integration Examples

### BlockDefinitions.json Parsing
```cpp
std::ifstream blockFile("Data/Definitions/BlockDefinitions.json");
json blockData = json::parse(blockFile);

for (auto& blockJson : blockData["blocks"]) {
    BlockDefinition* def = new BlockDefinition();
    def->m_name = blockJson["name"];
    def->m_isVisible = blockJson["visible"];
    def->m_isSolid = blockJson["solid"];
    def->m_isOpaque = blockJson["opaque"];

    auto topSprite = blockJson["topSprite"];
    def->m_topSpriteCoords = Vec2(topSprite[0], topSprite[1]);

    blockRegistry.Register(def->m_name, def);
}
```

### ItemDefinitions.json Parsing
```cpp
json itemData = json::parse(std::ifstream("Data/Definitions/ItemDefinitions.json"));

for (auto& itemJson : itemData["items"]) {
    ItemDefinition* def = new ItemDefinition();
    def->m_name = itemJson["name"];
    def->m_displayName = itemJson["displayName"];
    def->m_maxStackSize = itemJson["maxStackSize"];

    std::string typeStr = itemJson["type"];
    if (typeStr == "block") def->m_itemType = ItemType::BLOCK;
    else if (typeStr == "tool") def->m_itemType = ItemType::TOOL;

    if (def->m_itemType == ItemType::TOOL) {
        def->m_miningSpeed = itemJson["miningSpeed"];
        def->m_durability = itemJson["durability"];
    }

    itemRegistry.Register(def->m_name, def);
}
```

### Recipe.json Parsing
```cpp
json recipeData = json::parse(std::ifstream("Data/Definitions/Recipes.json"));

for (auto& recipeJson : recipeData["recipes"]) {
    Recipe* recipe = new Recipe();
    recipe->m_type = recipeJson["type"]; // "shaped" or "shapeless"

    if (recipe->m_type == "shaped") {
        auto pattern = recipeJson["pattern"];
        for (int row = 0; row < pattern.size(); row++) {
            for (int col = 0; col < pattern[row].size(); col++) {
                std::string itemName = pattern[row][col];
                recipe->m_inputPattern[row][col] = itemRegistry.GetID(itemName);
            }
        }
    } else { // shapeless
        for (auto& ingredient : recipeJson["ingredients"]) {
            std::string itemName = ingredient;
            recipe->m_ingredients.push_back(itemRegistry.GetID(itemName));
        }
    }

    auto result = recipeJson["result"];
    recipe->m_resultItem = itemRegistry.GetID(result["item"]);
    recipe->m_resultCount = result["count"];

    recipeRegistry.Register(recipe);
}
```

### Inventory Serialization (Save/Load)
```cpp
// Serialize inventory to JSON
json SaveInventory(Inventory const& inventory) {
    json inv;
    inv["slots"] = json::array();

    for (int i = 0; i < 36; i++) {
        ItemStack const& stack = inventory.GetSlot(i);
        if (stack.IsEmpty()) {
            inv["slots"].push_back(nullptr);
        } else {
            json slot;
            slot["itemID"] = stack.m_itemID;
            slot["quantity"] = stack.m_quantity;
            if (stack.m_durability > 0) {
                slot["durability"] = stack.m_durability;
            }
            inv["slots"].push_back(slot);
        }
    }

    return inv;
}

// Deserialize inventory from JSON
void LoadInventory(Inventory& inventory, json const& inv) {
    for (int i = 0; i < inv["slots"].size() && i < 36; i++) {
        if (inv["slots"][i].is_null()) {
            inventory.SetSlot(i, ItemStack());  // Empty stack
        } else {
            auto slot = inv["slots"][i];
            ItemStack stack;
            stack.m_itemID = slot["item ID"];
            stack.m_quantity = slot["quantity"];
            stack.m_durability = slot.value("durability", 0);
            inventory.SetSlot(i, stack);
        }
    }
}
```

## Common Pitfalls

### 1. Accessing Non-Existent Keys
```cpp
// BAD: Throws exception if key doesn't exist
std::string name = j["name"];

// GOOD: Use value() with default
std::string name = j.value("name", "unknown");

// GOOD: Check before accessing
if (j.contains("name")) {
    std::string name = j["name"];
}
```

### 2. Type Mismatches
```cpp
// BAD: Assuming type without checking
int value = j["value"];  // Throws if value is not an integer

// GOOD: Check type first
if (j["value"].is_number()) {
    int value = j["value"];
}
```

### 3. Array Access
```cpp
// BAD: Direct indexing without size check
auto first = j["items"][0];  // Throws if array is empty

// GOOD: Check size first
if (j["items"].is_array() && !j["items"].empty()) {
    auto first = j["items"][0];
}
```

## Performance Tips

1. **Parse Once**: Parse JSON files once at startup, don't re-parse every frame
2. **Use References**: Use `auto&` when iterating to avoid copies
3. **Reserve Capacity**: Pre-allocate vectors before filling from JSON arrays
4. **Validate Early**: Validate JSON schema at load time, not at access time

## Error Handling

```cpp
try {
    json j = json::parse(file);
    // Process JSON...
} catch (json::parse_error& e) {
    // Handle parsing errors
    std::cerr << "JSON parse error: " << e.what() << std::endl;
} catch (json::type_error& e) {
    // Handle type mismatch errors
    std::cerr << "JSON type error: " << e.what() << std::endl;
}
```

## Documentation

- **Official Docs**: https://json.nlohmann.me/
- **GitHub**: https://github.com/nlohmann/json
- **Header Location**: `Engine\Code\ThirdParty\json\json.hpp`

## Changelog

- 2025-11-24: **Initial JSON library documentation created for Assignment 7**
  - Primary JSON library for A7 registry system (blocks, items, recipes)
  - Replacing XML format with JSON for all SimpleMiner data definitions
  - Added A7-specific usage examples for Block/Item/Recipe parsing
  - Added inventory serialization examples for save/load system
