import pyMeow as pm
import requests, struct

OFFSETS_URL = "https://robloxoffsets.com/offsets.json"
resp = requests.get(OFFSETS_URL)
OFFSETS = resp.json()

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

class Instance:
    Address: int
    Process: ProcessHandler

    def __init__(self, process: ProcessHandler, address: int | None = None):
        self.Process = process
        self.Address = address if address else process.DataModel

    def __repr__(self):
        return self.Name

    @property
    def Name(self) -> str:
        Data = self.Process.ReadPointer(self.Address + self.Process.GetOffset("Name"))
        return self.Process.ReadString(Data)

    @property
    def ClassName(self) -> str:
        ClassDescriptor = self.Process.ReadPointer(
            self.Address + self.Process.GetOffset("ClassDescriptor")
        )
        ClassName = self.Process.ReadPointer(
            ClassDescriptor + self.Process.GetOffset("ClassDescriptorToClassName")
        )

        return self.Process.ReadString(ClassName)
    
    def GetChildren(self) -> list["Instance"]:
        ChildrenList = []
        ChildrenContainer = self.Process.ReadPointer(
            self.Address + self.Process.GetOffset("Children")
        )
        if not ChildrenContainer:
            return ChildrenList

        Start = self.Process.ReadPointer(ChildrenContainer)
        End = self.Process.ReadPointer(ChildrenContainer + 0x8)
        Current = Start

        while Current < End:
            ChildAddress = self.Process.ReadPointer(Current)
            if ChildAddress:
                ChildrenList.append(Instance(self.Process, ChildAddress))

            Current += 0x10

        return ChildrenList

    def FindFirstChild(self, name: str) -> "Instance":
        Children = self.Process.ReadPointer(
            self.Address + self.Process.GetOffset("Children")
        )
        ChildrenEnd = self.Process.ReadPointer(
            self.Address + self.Process.GetOffset("ChildrenEnd")
        )
        Start = self.Process.ReadPointer(Children)
        End = self.Process.ReadPointer(ChildrenEnd)
        Current = Start

        while Current != End:
            ChildAddress = self.Process.ReadPointer(Current)
            Child = Instance(self.Process, ChildAddress)
            if Child.Name == name:
                return Child
            Current += 0x10

        raise ValueError(f"Child '{name}' not found")
    
    def FindFirstChildWhichIsA(self, classname: str) -> "Instance":
        Children = self.Process.ReadPointer(
            self.Address + self.Process.GetOffset("Children")
        )
        ChildrenEnd = self.Process.ReadPointer(
            self.Address + self.Process.GetOffset("ChildrenEnd")
        )
        Start = self.Process.ReadPointer(Children)
        End = self.Process.ReadPointer(ChildrenEnd)
        Current = Start

        while Current != End:
            ChildAddress = self.Process.ReadPointer(Current)
            Child = Instance(self.Process, ChildAddress)
            if Child.ClassName == classname:
                return Child
            Current += 0x10

        raise ValueError(f"Child '{classname}' not found")
