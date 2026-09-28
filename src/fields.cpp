#include "instance.h"

// ---------------------------------------------------------------------------
// Data model field specifications — {fieldName, typeString}
// Each map key is the class name, value is {offsetPath, typeString}.
// These are intentionally empty placeholders. Fill them in from your
// offsets.json keys or a separate config file.
// ---------------------------------------------------------------------------

const std::map<std::string, FieldSpec> DATA_MODEL_FIELDS = {};
const std::map<std::string, FieldSpec> PLAYER_FIELDS = {};
const std::map<std::string, FieldSpec> BASE_PART_FIELDS = {
    {"Position",     {"Primitive.Position",   "vector3"}},
    {"CFrame",       {"Primitive.CFrame",     "cframe"}},
    {"Size",         {"Primitive.Size",       "vector3"}},
    {"Anchored",     {"Primitive.Anchored",   "bool"}},
    {"Transparency", {"Primitive.Transparency","float"}},
    {"CanCollide",   {"Primitive.CanCollide", "bool"}},
    {"CastShadow",   {"Primitive.CastShadow", "bool"}},
    {"Locked",       {"Primitive.Locked",     "bool"}},
    {"Health",       {"Primitive.Health",     "float"}},
};
const std::map<std::string, FieldSpec> PRIMITIVE_FIELDS = {
    {"Position",   {"Primitive.Position",   "vector3"}},
    {"CFrame",     {"Primitive.CFrame",     "cframe"}},
    {"Size",       {"Primitive.Size",       "vector3"}},
    {"Anchored",   {"Primitive.Anchored",   "bool"}},
    {"Transparency",{"Primitive.Transparency","float"}},
    {"CanCollide", {"Primitive.CanCollide", "bool"}},
    {"CastShadow", {"Primitive.CastShadow", "bool"}},
    {"Locked",     {"Primitive.Locked",     "bool"}},
    {"Health",     {"Primitive.Health",     "float"}},
};
const std::map<std::string, FieldSpec> HUMANOID_FIELDS = {
    {"Health",      {"Humanoid.Health",    "float"}},
    {"MaxHealth",   {"Humanoid.MaxHealth", "float"}},
    {"WalkSpeed",   {"Humanoid.WalkSpeed", "int"}},
    {"JumpPower",   {"Humanoid.JumpPower", "int"}},
};
const std::map<std::string, FieldSpec> CAMERA_FIELDS = {
    {"FieldOfView", {"Camera.FieldOfView", "float"}},
    {"CFrame",      {"Camera.CFrame",      "cframe"}},
};
const std::map<std::string, FieldSpec> SOUND_FIELDS = {
    {"Volume",  {"Sound.Volume",  "float"}},
    {"Pitch",   {"Sound.Pitch",   "float"}},
    {"Playing", {"Sound.Playing", "bool"}},
};
const std::map<std::string, FieldSpec> LIGHTING_FIELDS = {
    {"Ambient", {"Lighting.Ambient",      "color3"}},
    {"Brightness", {"Lighting.Brightness", "float"}},
    {"ExposureSpeed", {"Lighting.ExposureSpeed", "float"}},
};
const std::map<std::string, FieldSpec> SKY_FIELDS = {
    {"SkyboxBk", {"Sky.SkyboxBk",  "string"}},
    {"SkyboxDn", {"Sky.SkyboxDn",  "string"}},
    {"SkyboxFt", {"Sky.SkyboxFt",  "string"}},
    {"SkyboxLf", {"Sky.SkyboxLf",  "string"}},
    {"SkyboxRt", {"Sky.SkyboxRt",  "string"}},
    {"SkyboxUp", {"Sky.SkyboxUp",  "string"}},
};
const std::map<std::string, FieldSpec> ATMOSPHERE_FIELDS = {};
const std::map<std::string, FieldSpec> PROXIMITY_FIELDS = {};
const std::map<std::string, FieldSpec> CLICK_DETECTOR_FIELDS = {};
const std::map<std::string, FieldSpec> BEAM_FIELDS = {};
const std::map<std::string, FieldSpec> PARTICLE_FIELDS = {};
const std::map<std::string, FieldSpec> MESH_PART_FIELDS = {};
const std::map<std::string, FieldSpec> SPECIAL_MESH_FIELDS = {};
const std::map<std::string, FieldSpec> DECAL_FIELDS = {
    {"Texture", {"Decal.Texture", "string"}},
    {"Transparency", {"Decal.Transparency", "float"}},
};
const std::map<std::string, FieldSpec> TEXTURE_FIELDS = {};
const std::map<std::string, FieldSpec> SEAT_FIELDS = {
    {"MaxUsers", {"Seat.MaxUsers", "int"}},
    {"SitPosition", {"Seat.SitPosition", "vector3"}},
};
const std::map<std::string, FieldSpec> VEHICLE_SEAT_FIELDS = {
    {"MaxUsers", {"VehicleSeat.MaxUsers", "int"}},
    {"SteerAngle", {"VehicleSeat.SteerAngle", "float"}},
    {"Speed", {"VehicleSeat.Speed", "float"}},
};
const std::map<std::string, FieldSpec> TOOL_FIELDS = {};
const std::map<std::string, FieldSpec> MODEL_FIELDS = {};
const std::map<std::string, FieldSpec> ATTACHMENT_FIELDS = {
    {"CFrame", {"Attachment.CFrame", "cframe"}},
};
const std::map<std::string, FieldSpec> WELD_FIELDS = {};
const std::map<std::string, FieldSpec> WELD_CONSTRAINT_FIELDS = {};
const std::map<std::string, FieldSpec> ANIMATION_TRACK_FIELDS = {
    {"Weight", {"AnimationTrack.Weight", "float"}},
    {"TimePosition", {"AnimationTrack.TimePosition", "float"}},
};
const std::map<std::string, FieldSpec> SURFACE_APPEARANCE_FIELDS = {};
const std::map<std::string, FieldSpec> SPAWN_LOCATION_FIELDS = {};
const std::map<std::string, FieldSpec> CLOTHING_FIELDS = {};
const std::map<std::string, FieldSpec> CHARACTER_MESH_FIELDS = {};
const std::map<std::string, FieldSpec> UNION_FIELDS = {};
const std::map<std::string, FieldSpec> GUI_FIELDS = {
    {"Position",  {"Gui.Position",  "vector2"}},
    {"Size",      {"Gui.Size",      "vector2"}},
    {"AnchorPoint", {"Gui.AnchorPoint", "vector2"}},
    {"AutomaticSize", {"Gui.AutomaticSize", "string"}},
    {"BackgroundTransparency", {"Gui.BackgroundTransparency", "float"}},
    {"BorderSizePixel", {"Gui.BorderSizePixel", "int"}},
    {"Active",    {"Gui.Active",    "bool"}},
    {"Visible",   {"Gui.Visible",   "bool"}},
    {"ZIndex",    {"Gui.ZIndex",    "int"}},
};

const FieldSpec SURFACE_GUI_ENABLED = {"SurfaceGui.Enabled", "bool"};

// ---------------------------------------------------------------------------
// Class name sets
// ---------------------------------------------------------------------------

const std::set<std::string> BASE_PART_CLASSES = {
    "Part", "BasePart", "WedgePart", "CornerPart", "TrussPart",
    "PrismPart", "PipePart", "SplinePart", "ModifiablePart",
    "Union", "BooleanOperation", "MeshPart",
};

const std::set<std::string> VALUE_CLASSES = {
    "IntValue", "BoolValue", "StringValue", "DoubleValue",
    "FloatValue", "NumberValue", "Color3Value", "CFrameValue",
    "Vector3Value", "Vector2Value", "BrickColorValue",
    "ObjectValue", "InstanceValue",
};

const std::set<std::string> GUI_CLASSES = {
    "GuiObject", "Frame", "TextLabel", "TextButton", "ImageLabel",
    "ImageButton", "ScrollingFrame", "CanvasGroup", "UIListLayout",
    "UIAspectRatioConstraint", "UIGridLayout", "UIPageLayout",
    "UICorner", "UIStroke", "UIPadding", "ScreenGui",
    "BillboardGui", "SurfaceGui",
};

const std::set<std::string> WRITEABLE = {
    "Part", "Model", "BasePart", "Player", "Character",
};

const std::map<std::string, std::string> PRIMITIVE_FLAG_BITS = {
    {"CastShadow", "0x0"},
    {"CanCollide", "0x1"},
    {"Anchored",   "0x2"},
    {"Locked",     "0x3"},
};
