#include "src/instance.h"
#include "src/memory.h"

#include <chrono>
#include <algorithm>

// ---------------------------------------------------------------------------
// Helper: get FieldSpec from any of the known maps
// ---------------------------------------------------------------------------
namespace {
    std::optional<FieldSpec> FindFieldSpec(const std::string& className,
                                           const std::string& fieldName) {
        static const std::vector<const std::map<std::string, FieldSpec>*> allMaps = {
            &DATA_MODEL_FIELDS,
            &PLAYER_FIELDS,
            &BASE_PART_FIELDS,
            &PRIMITIVE_FIELDS,
            &HUMANOID_FIELDS,
            &CAMERA_FIELDS,
            &SOUND_FIELDS,
            &LIGHTING_FIELDS,
            &SKY_FIELDS,
            &ATMOSPHERE_FIELDS,
            &PROXIMITY_FIELDS,
            &CLICK_DETECTOR_FIELDS,
            &BEAM_FIELDS,
            &PARTICLE_FIELDS,
            &MESH_PART_FIELDS,
            &SPECIAL_MESH_FIELDS,
            &DECAL_FIELDS,
            &TEXTURE_FIELDS,
            &SEAT_FIELDS,
            &VEHICLE_SEAT_FIELDS,
            &TOOL_FIELDS,
            &MODEL_FIELDS,
            &ATTACHMENT_FIELDS,
            &WELD_FIELDS,
            &WELD_CONSTRAINT_FIELDS,
            &ANIMATION_TRACK_FIELDS,
            &SURFACE_APPEARANCE_FIELDS,
            &SPAWN_LOCATION_FIELDS,
            &CLOTHING_FIELDS,
            &CHARACTER_MESH_FIELDS,
            &UNION_FIELDS,
            &GUI_FIELDS,
        };

        // 1) Check exact class match
        for (const auto* mp : allMaps) {
            auto it = mp->find(className);
            if (it != mp->end()) {
                auto fit = it->second.second.empty()
                    ? std::nullopt
                    : std::find_if(it->second.second.begin(), it->second.second.end(),
                        [&fieldName](const auto& kv) { return kv.first == fieldName; });
                // Fields map is actually map<fieldName, type>. Let's re-check.
            }
        }

        // Simpler approach: iterate all maps looking for fieldName
        // Actually the maps are map<className, FieldSpec> where FieldSpec = {name, type}
        // OR map<fieldName, type> depending on the map. Let's use a more direct strategy.
        return std::nullopt;
    }
}

// ---------------------------------------------------------------------------
// Instance method implementations
// ---------------------------------------------------------------------------

std::string Instance::GetClassName() const {
    if (hasClassCache) return classNameCache;
    classNameCache = GetInstanceClass(hProcess, Address);
    hasClassCache = true;
    return classNameCache;
}

std::string Instance::GetClassNameUnchecked() const {
    return GetInstanceClass(hProcess, Address);
}

std::string Instance::GetName() const {
    return GetInstanceName(hProcess, Address);
}

Instance Instance::GetParent() const {
    // The parent pointer offset inside Instance
    auto parentOff = GetOffset("Instance.Parent");
    if (!parentOff) return Instance();

    auto parentAddr = ReadPointer(hProcess, Address + *parentOff);
    if (!parentAddr) return Instance();

    return Instance(hProcess, *parentAddr);
}

std::vector<Instance> Instance::GetChildren() const {
    auto raw = GetChildPointers(hProcess, Address);
    std::vector<Instance> result;
    result.reserve(raw.size());
    for (uintptr_t addr : raw) {
        result.emplace_back(hProcess, addr);
    }
    return result;
}

// ---------------------------------------------------------------------------
// Recursive descendant collection
// ---------------------------------------------------------------------------
static void _collectDescendants(HANDLE hProc, uintptr_t parent,
                                std::vector<Instance>& out) {
    for (uintptr_t child : GetChildPointers(hProc, parent)) {
        out.emplace_back(hProc, child);
        _collectDescendants(hProc, child, out);
    }
}

std::vector<Instance> Instance::GetDescendantsList() const {
    std::vector<Instance> result;
    _collectDescendants(hProcess, Address, result);
    return result;
}

std::function<void(std::function<void(const Instance&)>)> Instance::GetDescendants() const {
    std::vector<Instance> list = GetDescendantsList();
    return [list](std::function<void(const Instance&)> fn) {
        for (const auto& inst : list) fn(inst);
    };
}

// ---------------------------------------------------------------------------
// Find methods
// ---------------------------------------------------------------------------

static uintptr_t _findChildByName(HANDLE hProc, uintptr_t parent, const std::string& name) {
    for (uintptr_t child : GetChildPointers(hProc, parent)) {
        if (GetInstanceName(hProc, child) == name) return child;
    }
    return 0;
}

static uintptr_t _findChildByClass(HANDLE hProc, uintptr_t parent, const std::string& cls) {
    for (uintptr_t child : GetChildPointers(hProc, parent)) {
        if (GetInstanceClass(hProc, child) == cls) return child;
    }
    return 0;
}

Instance Instance::FindFirstChild(const std::string& name) const {
    if (!name.empty()) return FindFirstChildByName(name);
    auto children = GetChildren();
    if (!children.empty()) return children[0];
    return Instance();
}

Instance Instance::FindFirstChildOfClass(const std::string& className) const {
    auto addr = _findChildByClass(hProcess, Address, className);
    return addr ? Instance(hProcess, addr) : Instance();
}

Instance Instance::FindFirstChildWhichIsA(const std::string& className) const {
    // Check self first, then children
    if (GetClassName() == className) return *this;
    auto addr = _findChildByClass(hProcess, Address, className);
    return addr ? Instance(hProcess, addr) : Instance();
}

Instance Instance::FindFirstChildByName(const std::string& name) const {
    auto addr = _findChildByName(hProcess, Address, name);
    return addr ? Instance(hProcess, addr) : Instance();
}

Instance Instance::FindFirstAncestor(const std::string& name) const {
    Instance parent = GetParent();
    while (parent) {
        if (parent.GetName() == name) return parent;
        parent = parent.GetParent();
    }
    return Instance();
}

Instance Instance::WaitForChild(const std::string& name, float timeout) const {
    auto start = std::chrono::steady_clock::now();
    auto deadline = start + std::chrono::milliseconds(static_cast<int>(timeout * 1000));

    while (std::chrono::steady_clock::now() < deadline) {
        Instance child = FindFirstChild(name);
        if (child) return child;
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    return Instance();
}

// ---------------------------------------------------------------------------
// IsA — checks against known class name sets and exact match
// ---------------------------------------------------------------------------

static std::string _getClassHierarchy(const std::string& className) {
    // Simple exact match for now; can be extended with a hierarchy map
    // by loading from offsets.json or a separate hierarchy JSON.
    return className;
}

bool Instance::IsA(const std::string& className) const {
    std::string myClass = GetClassName();

    if (myClass == className) return true;

    // Check known sets
    if (BASE_PART_CLASSES.count(className)) {
        for (const auto& bc : BASE_PART_CLASSES) {
            if (myClass == bc) return true;
        }
    }
    if (GUI_CLASSES.count(className)) {
        for (const auto& gc : GUI_CLASSES) {
            if (myClass == gc) return true;
        }
    }
    if (VALUE_CLASSES.count(className)) {
        for (const auto& vc : VALUE_CLASSES) {
            if (myClass == vc) return true;
        }
    }

    // Fallback: check parent chain for IsA-like behavior
    // (e.g., Part IsA "BasePart", Part IsA "Instance")
    // For now this is basic — a full hierarchy would need a map.
    if (className == "Instance") return true;

    return myClass == className;
}

// ---------------------------------------------------------------------------
// GetFullName
// ---------------------------------------------------------------------------

std::string Instance::GetFullName() const {
    // Walk up the tree collecting names, then reverse
    std::vector<std::string> parts;
    Instance cur = *this;
    while (cur) {
        parts.push_back(cur.GetName());
        cur = cur.GetParent();
    }
    std::reverse(parts.begin(), parts.end());

    std::string result;
    for (size_t i = 0; i < parts.size(); ++i) {
        if (i > 0) result += ".";
        result += parts[i];
    }
    return result;
}

// ---------------------------------------------------------------------------
// GetPlayers — get all Player instances from Players service
// ---------------------------------------------------------------------------

std::vector<Instance> Instance::GetPlayers() const {
    Instance players = FindFirstChildOfClass("Players");
    if (!players) return {};
    return players.GetChildren();
}

// ---------------------------------------------------------------------------
// DispatchGet / DispatchSet — route field reads based on className + fieldName
// ---------------------------------------------------------------------------

std::optional<std::any> Instance::DispatchGet(const std::string& name) const {
    std::string cls = GetClassName();

    // 1) Check value classes — read from the "Value" field
    if (VALUE_CLASSES.count(cls)) {
        auto v = ReadValue();
        return v;
    }

    // 2) Check each field map for this class
    static const std::vector<const std::map<std::string, FieldSpec>*> allMaps = {
        &DATA_MODEL_FIELDS,
        &PLAYER_FIELDS,
        &BASE_PART_FIELDS,
        &PRIMITIVE_FIELDS,
        &HUMANOID_FIELDS,
        &CAMERA_FIELDS,
        &SOUND_FIELDS,
        &LIGHTING_FIELDS,
        &SKY_FIELDS,
        &ATMOSPHERE_FIELDS,
        &PROXIMITY_FIELDS,
        &CLICK_DETECTOR_FIELDS,
        &BEAM_FIELDS,
        &PARTICLE_FIELDS,
        &MESH_PART_FIELDS,
        &SPECIAL_MESH_FIELDS,
        &DECAL_FIELDS,
        &TEXTURE_FIELDS,
        &SEAT_FIELDS,
        &VEHICLE_SEAT_FIELDS,
        &TOOL_FIELDS,
        &MODEL_FIELDS,
        &ATTACHMENT_FIELDS,
        &WELD_FIELDS,
        &WELD_CONSTRAINT_FIELDS,
        &ANIMATION_TRACK_FIELDS,
        &SURFACE_APPEARANCE_FIELDS,
        &SPAWN_LOCATION_FIELDS,
        &CLOTHING_FIELDS,
        &CHARACTER_MESH_FIELDS,
        &UNION_FIELDS,
        &GUI_FIELDS,
    };

    for (const auto* mp : allMaps) {
        auto it = mp->find(cls);
        if (it == mp->end()) continue;

        const auto& field = it->second;
        if (field.first == name) {
            return ReadPrimitiveField(field);
        }
    }

    return std::nullopt;
}

bool Instance::DispatchSet(const std::string& name, const std::any& value) const {
    std::string cls = GetClassName();

    if (VALUE_CLASSES.count(cls)) {
        return WriteValue(value);
    }

    static const std::vector<const std::map<std::string, FieldSpec>*> allMaps = {
        &DATA_MODEL_FIELDS,
        &PLAYER_FIELDS,
        &BASE_PART_FIELDS,
        &PRIMITIVE_FIELDS,
        &HUMANOID_FIELDS,
        &CAMERA_FIELDS,
        &SOUND_FIELDS,
        &LIGHTING_FIELDS,
        &SKY_FIELDS,
        &ATMOSPHERE_FIELDS,
        &PROXIMITY_FIELDS,
        &CLICK_DETECTOR_FIELDS,
        &BEAM_FIELDS,
        &PARTICLE_FIELDS,
        &MESH_PART_FIELDS,
        &SPECIAL_MESH_FIELDS,
        &DECAL_FIELDS,
        &TEXTURE_FIELDS,
        &SEAT_FIELDS,
        &VEHICLE_SEAT_FIELDS,
        &TOOL_FIELDS,
        &MODEL_FIELDS,
        &ATTACHMENT_FIELDS,
        &WELD_FIELDS,
        &WELD_CONSTRAINT_FIELDS,
        &ANIMATION_TRACK_FIELDS,
        &SURFACE_APPEARANCE_FIELDS,
        &SPAWN_LOCATION_FIELDS,
        &CLOTHING_FIELDS,
        &CHARACTER_MESH_FIELDS,
        &UNION_FIELDS,
        &GUI_FIELDS,
    };

    for (const auto* mp : allMaps) {
        auto it = mp->find(cls);
        if (it == mp->end()) continue;

        const auto& field = it->second;
        if (field.first == name) {
            return WritePrimitiveField(field, value);
        }
    }

    return false;
}

// ---------------------------------------------------------------------------
// Helper: HasField
// ---------------------------------------------------------------------------

bool Instance::HasField(const std::string& fieldName) const {
    return DispatchGet(fieldName).has_value();
}

// ---------------------------------------------------------------------------
// ReadField / WriteField — generic any-based field access
// ---------------------------------------------------------------------------

std::optional<std::any> Instance::ReadField(const FieldSpec& spec) const {
    return ReadPrimitiveField(spec);
}

bool Instance::WriteField(const FieldSpec& spec, const std::any& value) const {
    return WritePrimitiveField(spec, value);
}

// ---------------------------------------------------------------------------
// Primitive field read/write
// ---------------------------------------------------------------------------

static std::string _resolveType(const FieldSpec& spec) {
    return spec.second; // type string from FieldSpec
}

std::optional<std::any> Instance::ReadPrimitiveField(const FieldSpec& spec) const {
    std::string type = _resolveType(spec);

    // spec.first is the fieldName, but we need the actual offset path
    // The FieldSpec type string encodes the offset path.
    // e.g. type = "BasePart.Position" → we resolve to offsets and read
    auto offset = GetOffset(spec.first); // the fieldName serves as offset path key
    if (!offset) return std::nullopt;

    uintptr_t addr = Address + *offset;

    if (type == "int")         { auto v = ReadInt(hProcess, addr);    return v ? std::any(*v) : std::nullopt; }
    if (type == "int64")       { auto v = ReadInt64(hProcess, addr);  return v ? std::any(*v) : std::nullopt; }
    if (type == "float")       { auto v = ReadFloat(hProcess, addr);  return v ? std::any(*v) : std::nullopt; }
    if (type == "double")      { auto v = ReadDouble(hProcess, addr); return v ? std::any(*v) : std::nullopt; }
    if (type == "bool")        { auto v = ReadBool(hProcess, addr);   return v ? std::any(*v) : std::nullopt; }
    if (type == "string")      { auto v = ReadString(hProcess, addr); return v ? std::any(*v) : std::nullopt; }
    if (type == "vector2")     { auto v = ReadVector2(hProcess, addr); return v ? std::any(*v) : std::nullopt; }
    if (type == "vector3")     { auto v = ReadVector3(hProcess, addr); return v ? std::any(*v) : std::nullopt; }
    if (type == "color3")      { auto v = ReadColor3(hProcess, addr);  return v ? std::any(*v) : std::nullopt; }
    if (type == "cframe")      { auto v = ReadCFrame(hProcess, addr);  return v ? std::any(*v) : std::nullopt; }

    return std::nullopt;
}

bool Instance::WritePrimitiveField(const FieldSpec& spec, const std::any& value) const {
    std::string type = _resolveType(spec);
    auto offset = GetOffset(spec.first);
    if (!offset) return false;
    uintptr_t addr = Address + *offset;

    if (type == "int")         return WriteInt(hProcess, addr, std::any_cast<int>(value));
    if (type == "int64")       return WriteInt64(hProcess, addr, std::any_cast<int64_t>(value));
    if (type == "float")       return WriteFloat(hProcess, addr, std::any_cast<float>(value));
    if (type == "double")      return WriteDouble(hProcess, addr, std::any_cast<double>(value));
    if (type == "bool")        return WriteBool(hProcess, addr, std::any_cast<bool>(value));
    if (type == "vector3")     return WriteVector3(hProcess, addr, std::any_cast<Vector3>(value));
    if (type == "color3")      return WriteColor3(hProcess, addr, std::any_cast<Color3>(value));
    if (type == "cframe")      return WriteCFrame(hProcess, addr, std::any_cast<CFrame>(value));

    // Strings need special handling via WriteString, but FieldSpec for strings
    // would use "string" type — that's typically a different code path
    if (type == "string") {
        auto strVal = std::any_cast<std::string>(value);
        return WriteString(hProcess, addr, strVal);
    }

    return false;
}

// ---------------------------------------------------------------------------
// Primitive flag read/write (bitfield)
// ---------------------------------------------------------------------------

std::optional<bool> Instance::ReadPrimitiveFlag(const std::string& bitPath) const {
    auto primOff = GetOffset("BasePart.Primitive");
    if (!primOff) return std::nullopt;

    auto flagOff = GetOffset(bitPath);
    if (!flagOff) return std::nullopt;

    return ReadBool(hProcess, Address + *primOff + *flagOff);
}

bool Instance::WritePrimitiveFlag(const std::string& bitPath, bool value) const {
    auto primOff = GetOffset("BasePart.Primitive");
    if (!primOff) return false;

    auto flagOff = GetOffset(bitPath);
    if (!flagOff) return false;

    return WriteBool(hProcess, Address + *primOff + *flagOff, value);
}

// ---------------------------------------------------------------------------
// Value class read/write (IntValue, BoolValue, etc.)
// ---------------------------------------------------------------------------

std::optional<std::any> Instance::ReadValue() const {
    auto valueOff = GetOffset("Misc.Value");
    if (!valueOff) return std::nullopt;

    uintptr_t valueAddr = Address + *valueOff;

    std::string cls = GetClassName();
    if (cls == "IntValue")      { auto v = ReadInt(hProcess, valueAddr);    return v ? std::any(*v) : std::nullopt; }
    if (cls == "BoolValue")     { auto v = ReadBool(hProcess, valueAddr);   return v ? std::any(*v) : std::nullopt; }
    if (cls == "StringValue")   { auto v = ReadString(hProcess, valueAddr); return v ? std::any(*v) : std::nullopt; }
    if (cls == "DoubleValue")   { auto v = ReadDouble(hProcess, valueAddr); return v ? std::any(*v) : std::nullopt; }
    if (cls == "FloatValue")    { auto v = ReadFloat(hProcess, valueAddr);  return v ? std::any(*v) : std::nullopt; }

    return std::nullopt;
}

bool Instance::WriteValue(const std::any& value) const {
    auto valueOff = GetOffset("Misc.Value");
    if (!valueOff) return false;

    uintptr_t valueAddr = Address + *valueOff;

    std::string cls = GetClassName();
    if (cls == "IntValue")      return WriteInt(hProcess, valueAddr, std::any_cast<int>(value));
    if (cls == "BoolValue")     return WriteBool(hProcess, valueAddr, std::any_cast<bool>(value));
    if (cls == "StringValue")   return WriteString(hProcess, valueAddr, std::any_cast<std::string>(value));
    if (cls == "DoubleValue")   return WriteDouble(hProcess, valueAddr, std::any_cast<double>(value));
    if (cls == "FloatValue")    return WriteFloat(hProcess, valueAddr, std::any_cast<float>(value));

    return false;
}

// ---------------------------------------------------------------------------
// WorldToScreen (member)
// ---------------------------------------------------------------------------

std::optional<Vector2> Instance::WorldToScreen(const Vector3& pos) const {
    // Requires a VisualEngine offset — caller passes via a separate function
    // unless we can discover it from the instance chain.
    // For now, expose the standalone function.
    return WorldToScreen(hProcess, pos, 0, nullptr);
}

// ---------------------------------------------------------------------------
// GetPivot — returns CFrame position of BasePart
// ---------------------------------------------------------------------------

std::optional<Vector3> Instance::GetPivot() const {
    if (!IsA("BasePart")) return std::nullopt;

    auto primOff = GetOffset("BasePart.Primitive");
    auto posOff  = GetOffset("Primitive.Position");
    if (!primOff || !posOff) return std::nullopt;

    auto prim = ReadPointer(hProcess, Address + *primOff);
    if (!prim) return std::nullopt;

    return ReadVector3(hProcess, *prim + *posOff);
}

// ---------------------------------------------------------------------------
// DistanceTo
// ---------------------------------------------------------------------------

std::optional<float> Instance::DistanceTo(const Instance& other) const {
    auto posA = GetPivot();
    auto posB = other.GetPivot();
    if (!posA || !posB) return std::nullopt;

    return posA->Distance(*posB);
}
