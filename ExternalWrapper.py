import pyMeow as pm
import struct, time, requests

OFFSETS_URL = "https://robloxoffsets.com/offsets.json"
OFFSETS = requests.get(OFFSETS_URL).json()
OFFSETS["CameraOffset"] = "0x140"
OFFSETS["AssemblyAngularVelocity"] = "0xfc"
OFFSETS["AssemblyLinearVelocity"] = "0xf0"

class Vector2:
    def __repr__(self):
        return f"Vector2({self.X}, {self.Y})"

    def __init__(self, x, y):
        self.X = x
        self.Y = y

    def __add__(self, v):
        return Vector2(self.X + v.X, self.Y + v.Y)

    def __sub__(self, v):
        return Vector2(self.X - v.X, self.Y - v.Y)

    def __mul__(self, s):
        return Vector2(self.X * s, self.Y * s)

    def __truediv__(self, s):
        return Vector2(self.X / s, self.Y / s)

    def Magnitude(self):
        return (self.X ** 2 + self.Y ** 2) ** 0.5

    def Normal(self):
        m = self.Magnitude()
        return Vector2(self.X / m, self.Y / m) if m else Vector2(0, 0)


class Vector3:
    def __repr__(self):
        return f"Vector3({self.X}, {self.Y}, {self.Z})"

    def __init__(self, x, y, z):
        self.X = x
        self.Y = y
        self.Z = z

    def __add__(self, v):
        return Vector3(self.X + v.X, self.Y + v.Y, self.Z + v.Z)

    def __sub__(self, v):
        return Vector3(self.X - v.X, self.Y - v.Y, self.Z - v.Z)

    def __mul__(self, s):
        return Vector3(self.X * s, self.Y * s, self.Z * s)

    def __truediv__(self, s):
        return Vector3(self.X / s, self.Y / s, self.Z / s)

    def DotProduct(self, v):
        return self.X * v.X + self.Y * v.Y + self.Z * v.Z

    def CrossProduct(self, v):
        return Vector3(
            self.Y * v.Z - self.Z * v.Y,
            self.Z * v.X - self.X * v.Z,
            self.X * v.Y - self.Y * v.X
        )

    def Magnitude(self):
        return (self.X ** 2 + self.Y ** 2 + self.Z ** 2) ** 0.5

    def Normal(self):
        m = self.Magnitude()
        return Vector3(self.X / m, self.Y / m, self.Z / m) if m else Vector3(0, 0, 0)

    def Distance(self, v):
        return (self - v).Magnitude()


class CFrame:
    def __init__(self, matrix):
        self.matrix = matrix

    def __repr__(self):
        return f"CFrame({self.matrix})"

    @property
    def Position(self):
        return Vector3(self.matrix[3], self.matrix[7], self.matrix[11])

    def RightVector(self):
        return Vector3(self.matrix[0], self.matrix[1], self.matrix[2])

    def UpVector(self):
        return Vector3(self.matrix[4], self.matrix[5], self.matrix[6])

    def LookVector(self):
        return Vector3(self.matrix[8], self.matrix[9], self.matrix[10])

    def ToMatrix(self):
        return self.matrix


class ProcessHandler:
    def __init__(self):
        self.ProcessName = "RobloxPlayerBeta.exe"
        self.Process = pm.open_process(self.ProcessName)
        self.Module = pm.get_module(self.Process, self.ProcessName)
        self.Address = self.Module["base"]

        self.FakeDataModel = self.Address + self.GetOffset("FakeDataModelPointer")
        self.FakeDataModel = self.ReadPointer(self.FakeDataModel)

        self.DataModel = self.FakeDataModel + self.GetOffset("FakeDataModelToDataModel")
        self.DataModel = self.ReadPointer(self.DataModel)

        self.DataModelInstance = Instance(self)

        self.VisualEngine = self.Address + self.GetOffset("VisualEnginePointer")
        self.VisualEngine = self.ReadPointer(self.VisualEngine)

    def GetOffset(self, name: str) -> int:
        return int(OFFSETS[name], 16)

    def ReadMemory(self, Address: int, Size: int) -> bytes:
        return pm.r_bytes(self.Process, Address, Size)


    def ReadPointer(self, Address: int) -> int:
        Buffer = self.ReadMemory(Address, 8)
        return int.from_bytes(Buffer, "little")


    def ReadInt(self, Address: int) -> int:
        Buffer = self.ReadMemory(Address, 4)
        return int.from_bytes(Buffer, "little", signed=True)


    def Read_Int64(self, Address: int) -> int:
        Buffer = self.ReadMemory(Address, 8)
        return struct.unpack("q", Buffer)[0]


    def ReadFloat(self, Address: int) -> float:
        Buffer = self.ReadMemory(Address, 4)
        return struct.unpack("f", Buffer)[0]


    def ReadFloat64(self, Address: int) -> float:
        Buffer = self.ReadMemory(Address, 8)
        return struct.unpack("d", Buffer)[0]


    def ReadBool(self, Address: int) -> bool:
        Buffer = self.ReadMemory(Address, 1)
        return Buffer[0] != 0


    def ReadString(self, Address: int) -> str:
        if not Address:
            return ""

        Length = self.ReadInt(Address + 0x18)
        if Length <= 0 or Length > 1000:
            return ""

        if Length >= 16:
            Address = self.ReadPointer(Address)
            if not Address:
                return ""

        chars = []
        for i in range(Length):
            c = self.ReadMemory(Address + i, 1)[0]
            if c == 0:
                break
            chars.append(chr(c))

        return "".join(chars)


    def ReadVector2(self, Address: int):
        x = self.ReadFloat(Address)
        y = self.ReadFloat(Address + 4)
        return Vector2(x, y)


    def ReadVector3(self, Address: int):
        x = self.ReadFloat(Address)
        y = self.ReadFloat(Address + 4)
        z = self.ReadFloat(Address + 8)
        return Vector3(x, y, z)


    def ReadMatrix4x4(self, Address: int):
        return [self.ReadFloat(Address + i * 4) for i in range(16)]


    def ReadCFrame(self, Address: int):
        matrix = [self.ReadFloat(Address + i * 4) for i in range(12)]
        return CFrame(matrix)


    def WriteMemory(self, Address: int, Data: bytes):
        pm.w_bytes(self.Process, Address, Data)


    def WritePointer(self, Address: int, Value: int):
        self.WriteMemory(Address, Value.to_bytes(8, "little"))


    def WriteInt(self, Address: int, Value: int):
        self.WriteMemory(Address, Value.to_bytes(4, "little", signed=True))


    def Write_Int64(self, Address: int, Value: int):
        self.WriteMemory(Address, struct.pack("q", Value))


    def WriteFloat(self, Address: int, Value: float):
        self.WriteMemory(Address, struct.pack("f", Value))


    def WriteFloat64(self, Address: int, Value: float):
        self.WriteMemory(Address, struct.pack("d", Value))


    def WriteBool(self, Address: int, Value: bool):
        self.WriteMemory(Address, (b"\x01" if Value else b"\x00"))


    def WriteString(self, Address: int, Value: str):
        if not Address:
            return

        pm.w_string(self.Process, Address, Value)

    def WriteVector2(self, Address: int, vec):
        self.WriteFloat(Address, vec.X)
        self.WriteFloat(Address + 4, vec.Y)


    def WriteVector3(self, Address: int, vec):
        self.WriteFloat(Address, vec.X)
        self.WriteFloat(Address + 4, vec.Y)
        self.WriteFloat(Address + 8, vec.Z)


    def WriteMatrix4x4(self, Address: int, matrix):
        for i in range(16):
            self.WriteFloat(Address + i * 4, matrix[i])


    def WriteCFrame(self, Address: int, cf):
        matrix = cf.ToMatrix()
        for i in range(12):
            self.WriteFloat(Address + i * 4, matrix[i])


class Instance:
    Address: int
    Process: ProcessHandler

    def __init__(self, process: ProcessHandler, address: int | None = None):
        self.Process = process
        self.Address = address if address else process.DataModel

    def __repr__(self):
        try:
            return str(self.Name)
        except:
            return f"Instance({hex(self.Address)})"

    @property
    def Name(self) -> str:
        data = self.Process.ReadPointer(
            self.Address + self.Process.GetOffset("Name")
        )
        name = self.Process.ReadString(data)
        return name.decode() if isinstance(name, bytes) else name

    @property
    def ClassName(self) -> str:
        desc = self.Process.ReadPointer(
            self.Address + self.Process.GetOffset("ClassDescriptor")
        )
        name = self.Process.ReadPointer(
            desc + self.Process.GetOffset("ClassDescriptorToClassName")
        )
        s = self.Process.ReadString(name)
        return s.decode() if isinstance(s, bytes) else s

    @property
    def Value(self):
        offset = self.Process.GetOffset("Value")

        match self.ClassName:
            case "StringValue":
                return self.Process.ReadString(self.Address + offset)

            case "IntValue":
                return self.Process.ReadInt(self.Address + offset)

            case "NumberValue":
                return self.Process.ReadFloat64(self.Address + offset)

            case "BoolValue":
                return self.Process.ReadBool(self.Address + offset)

            case "Vector3Value":
                return self.Process.ReadVector3(self.Address + offset)

            case "ObjectValue":
                ptr = self.Process.ReadPointer(self.Address + offset)
                return Instance(self.Process, ptr)

    @Value.setter
    def Value(self, new_value):
        offset = self.Process.GetOffset("Value")
        addr = self.Address + offset

        match self.ClassName:
            case "StringValue":
                self.Process.WriteString(addr, new_value)

            case "IntValue":
                self.Process.WriteInt(addr, new_value)

            case "NumberValue":
                self.Process.WriteFloat64(addr, new_value)

            case "BoolValue":
                self.Process.WriteBool(addr, new_value)

            case "Vector3Value":
                self.Process.WriteVector3(addr, new_value)

            case "ObjectValue":
                if isinstance(new_value, Instance):
                    self.Process.WritePointer(addr, new_value.Address)
                else:
                    self.Process.WritePointer(addr, new_value)

    @property
    def Players(self) -> "Instance":
        return self.Process.DataModelInstance.FindFirstChildWhichIsA("Players")
    
    @property
    def LocalPlayer(self) -> "Instance":
        Data = self.Process.ReadPointer(
            self.Players.Address + self.Process.GetOffset("LocalPlayer")
        )

        return Instance(self.Process, Data)
    
    @property
    def Character(self) -> "Instance":
        Data = self.Process.ReadPointer(
            self.Address + self.Process.GetOffset("ModelInstance")
        )

        return Instance(self.Process, Data)

    @property
    def Primitive(self):
        addr = self.Process.ReadPointer(
            self.Address + self.Process.GetOffset("Primitive")
        )
        return addr or 0
    
    @property
    def Anchored(self):
        return self.Process.ReadBool(
            self.Primitive + self.Process.GetOffset("Anchored")
        )
    
    @Anchored.setter
    def Anchored(self, Value):
        return self.Process.WriteBool(
            self.Primitive + self.Process.GetOffset("Anchored"),
            Value
        )
    
    @property
    def Transparency(self):
        return self.Process.ReadFloat(
            self.Primitive + self.Process.GetOffset("Transparency")
        )
    
    @Transparency.setter
    def Transparency(self, Value):
        return self.Process.WriteFloat(
            self.Primitive + self.Process.GetOffset("Transparency"),
            Value
        )

    
    @property
    def CanCollide(self):
        return self.Process.ReadBool(
            self.Primitive + self.Process.GetOffset("CanCollide")
        )
    
    @CanCollide.setter
    def CanCollide(self, Value):
        return self.Process.WriteBool(
            self.Primitive + self.Process.GetOffset("CanCollide"),
            Value
        )

    @property
    def Size(self):
        return self.Process.ReadVector3(
            self.Primitive + self.Process.GetOffset("PartSize")
        )
    
    @Size.setter
    def Size(self, vec):
        return self.Process.WriteVector3(
            self.Primitive + self.Process.GetOffset("PartSize"),
            vec
        )

    @property
    def Velocity(self):
        return self.Process.ReadVector3(
            self.Primitive + self.Process.GetOffset("Velocity")
        )
    
    @Velocity.setter
    def Velocity(self, vec):
        return self.Process.WriteVector3(
            self.Primitive + self.Process.GetOffset("Velocity"),
            vec
        )
    
    @property
    def AssemblyLinearVelocity(self):
        return self.Process.ReadVector3(
            self.Primitive + self.Process.GetOffset("AssemblyLinearVelocity")
        )
    
    @AssemblyLinearVelocity.setter
    def AssemblyLinearVelocity(self, vec):
        return self.Process.WriteVector3(
            self.Primitive + self.Process.GetOffset("AssemblyLinearVelocity"),
            vec
        )

    @property
    def Position(self):
        return self.Process.ReadVector3(
            self.Primitive + self.Process.GetOffset("Position")
        )
    
    @Position.setter
    def Position(self, vec):
        return self.Process.WriteVector3(
            self.Primitive + self.Process.GetOffset("Position"),
            vec
        )
    
    @property
    def Team(self):
        return self.Process.ReadString(
            self.Address + self.Process.GetOffset("Team")
        )

    def GetChildren(self) -> list["Instance"]:
        children = []

        try:
            container = self.Process.ReadPointer(
                self.Address + self.Process.GetOffset("Children")
            )
            if not container:
                return children

            start = self.Process.ReadPointer(container)
            end = self.Process.ReadPointer(container + 0x8)

            if not start or not end:
                return children

            current = start
            while current < end:
                child_addr = self.Process.ReadPointer(current)
                if child_addr:
                    children.append(Instance(self.Process, child_addr))
                current += 0x10

        except:
            return children

        return children

    def FindFirstChild(self, name: str) -> "Instance":
        for child in self.GetChildren():
            if child.Name == name:
                return child
        raise ValueError(f"Child '{name}' not found")

    def FindFirstChildWhichIsA(self, classname: str) -> "Instance":
        for child in self.GetChildren():
            if child.ClassName == classname:
                return child
        raise ValueError(f"Child class '{classname}' not found")

    def WorldToScreen(self, position: Vector3):
        vm_addr = self.Process.VisualEngine + self.Process.GetOffset("viewmatrix")
        matrix = self.Process.ReadMatrix4x4(vm_addr)

        m00, m01, m02, m03 = matrix[0], matrix[1], matrix[2], matrix[3]
        m10, m11, m12, m13 = matrix[4], matrix[5], matrix[6], matrix[7]
        m20, m21, m22, m23 = matrix[8], matrix[9], matrix[10], matrix[11]
        m30, m31, m32, m33 = matrix[12], matrix[13], matrix[14], matrix[15]

        x = position.X * m00 + position.Y * m01 + position.Z * m02 + m03
        y = position.X * m10 + position.Y * m11 + position.Z * m12 + m13
        w = position.X * m30 + position.Y * m31 + position.Z * m32 + m33

        if w <= 0.01:
            return None

        inv_w = 1.0 / w

        screen_x = x * inv_w
        screen_y = y * inv_w

        if screen_x < -1 or screen_x > 1 or screen_y < -1 or screen_y > 1:
            return None

        return Vector2(
            (screen_x + 1) * 0.5,
            (1 - screen_y) * 0.5
                    )
