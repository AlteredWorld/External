#pragma once
#include "memory.h"
#include <any>
#include <set>
#include <map>
#include <functional>
#include <optional>

using FieldSpec = std::pair<std::string, std::string>;

extern const std::map<std::string, FieldSpec> DATA_MODEL_FIELDS;
extern const std::map<std::string, FieldSpec> PLAYER_FIELDS;
extern const std::map<std::string, FieldSpec> BASE_PART_FIELDS;
extern const std::map<std::string, std::string> PRIMITIVE_FLAG_BITS;
extern const std::map<std::string, FieldSpec> PRIMITIVE_FIELDS;
extern const std::map<std::string, FieldSpec> HUMANOID_FIELDS;
extern const std::map<std::string, FieldSpec> CAMERA_FIELDS;
extern const std::map<std::string, FieldSpec> SOUND_FIELDS;
extern const std::map<std::string, FieldSpec> LIGHTING_FIELDS;
extern const std::map<std::string, FieldSpec> SKY_FIELDS;
extern const std::map<std::string, FieldSpec> ATMOSPHERE_FIELDS;
extern const std::map<std::string, FieldSpec> PROXIMITY_FIELDS;
extern const std::map<std::string, FieldSpec> CLICK_DETECTOR_FIELDS;
extern const std::map<std::string, FieldSpec> BEAM_FIELDS;
extern const std::map<std::string, FieldSpec> PARTICLE_FIELDS;
extern const std::map<std::string, FieldSpec> MESH_PART_FIELDS;
extern const std::map<std::string, FieldSpec> SPECIAL_MESH_FIELDS;
extern const std::map<std::string, FieldSpec> DECAL_FIELDS;
extern const std::map<std::string, FieldSpec> TEXTURE_FIELDS;
extern const std::map<std::string, FieldSpec> SEAT_FIELDS;
extern const std::map<std::string, FieldSpec> VEHICLE_SEAT_FIELDS;
extern const std::map<std::string, FieldSpec> TOOL_FIELDS;
extern const std::map<std::string, FieldSpec> MODEL_FIELDS;
extern const std::map<std::string, FieldSpec> ATTACHMENT_FIELDS;
extern const std::map<std::string, FieldSpec> WELD_FIELDS;
extern const std::map<std::string, FieldSpec> WELD_CONSTRAINT_FIELDS;
extern const std::map<std::string, FieldSpec> ANIMATION_TRACK_FIELDS;
extern const std::map<std::string, FieldSpec> SURFACE_APPEARANCE_FIELDS;
extern const std::map<std::string, FieldSpec> SPAWN_LOCATION_FIELDS;
extern const std::map<std::string, FieldSpec> CLOTHING_FIELDS;
extern const std::map<std::string, FieldSpec> CHARACTER_MESH_FIELDS;
extern const std::map<std::string, FieldSpec> UNION_FIELDS;
extern const std::map<std::string, FieldSpec> GUI_FIELDS;
extern const FieldSpec SURFACE_GUI_ENABLED;

extern const std::set<std::string> BASE_PART_CLASSES;
extern const std::set<std::string> VALUE_CLASSES;
extern const std::set<std::string> GUI_CLASSES;
extern const std::set<std::string> WRITEABLE;

class Instance {
public:
    Instance() : hProcess(nullptr), Address(0) {}
    Instance(HANDLE hProcess, uintptr_t addr) : hProcess(hProcess), Address(addr) {}

    bool operator==(const Instance& o) const { return Address == o.Address; }
    bool operator!=(const Instance& o) const { return Address != o.Address; }
    explicit operator bool() const { return Address != 0; }

    std::string GetName() const;
    std::string GetClassName() const;
    std::string GetClassNameUnchecked() const;
    Instance GetParent() const;
    std::vector<Instance> GetChildren() const;
    std::vector<Instance> GetDescendantsList() const;
    std::function<void(std::function<void(const Instance&)>)> GetDescendants() const;
    Instance FindFirstChild(const std::string& name) const;
    Instance FindFirstChildOfClass(const std::string& className) const;
    Instance FindFirstChildWhichIsA(const std::string& className) const;
    Instance FindFirstChildByName(const std::string& name) const;
    Instance FindFirstAncestor(const std::string& name) const;
    Instance WaitForChild(const std::string& name, float timeout = 5.0f) const;
    bool IsA(const std::string& className) const;
    std::string GetFullName() const;

    std::vector<Instance> GetPlayers() const;

    // Access to raw address — useful for chaining with raw offsets
    uintptr_t GetAddress() const { return Address; }

    template <typename T>
    T GetField(const std::string& fieldName) const;

    bool HasField(const std::string& fieldName) const;

    template <typename T>
    bool SetField(const std::string& fieldName, const T& value);

    std::optional<Vector2> WorldToScreen(const Vector3& pos) const;
    std::optional<Vector3> GetPivot() const;
    std::optional<float> DistanceTo(const Instance& other) const;

    // Fluent API helpers — chainable, return *this
    Instance& WithInt(const std::string& fieldName, int value)  { SetField(fieldName, value); return *this; }
    Instance& WithFloat(const std::string& fieldName, float value) { SetField(fieldName, value); return *this; }
    Instance& WithBool(const std::string& fieldName, bool value)  { SetField(fieldName, value); return *this; }

protected:
    HANDLE hProcess;
    uintptr_t Address;

private:
    mutable std::string classNameCache;
    mutable bool hasClassCache = false;

    std::optional<std::any> ReadField(const FieldSpec& spec) const;
    bool WriteField(const FieldSpec& spec, const std::any& value) const;
    std::optional<std::any> ReadPrimitiveField(const FieldSpec& spec) const;
    bool WritePrimitiveField(const FieldSpec& spec, const std::any& value) const;
    std::optional<bool> ReadPrimitiveFlag(const std::string& bitPath) const;
    bool WritePrimitiveFlag(const std::string& bitPath, bool value) const;
    std::optional<std::any> ReadValue() const;
    bool WriteValue(const std::any& value) const;

    std::optional<std::any> DispatchGet(const std::string& name) const;
    bool DispatchSet(const std::string& name, const std::any& value) const;
};

template <typename T>
T Instance::GetField(const std::string& fieldName) const {
    auto val = DispatchGet(fieldName);
    if (val.has_value()) {
        try {
            return std::any_cast<T>(*val);
        } catch (...) {}
    }
    return T{};
}

template <typename T>
bool Instance::SetField(const std::string& fieldName, const T& value) {
    return DispatchSet(fieldName, std::any(value));
}
