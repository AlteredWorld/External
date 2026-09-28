#include "memory.h"
#include "instance.h"

#include <iostream>
#include <string>

int main() {
    LoadOffsets("offsets.json");

    ProcessInfo proc = OpenRobloxProcess();
    if (!proc.hProcess) {
        std::cerr << "Could not open RobloxPlayerBeta.exe\n";
        std::cin.get();
        return 1;
    }

    // ── DataModel ──────────────────────────────────────────────────────
    auto fakeOff  = GetOffset("FakeDataModel.Pointer");
    auto realOff  = GetOffset("FakeDataModel.RealDataModel");
    if (!fakeOff || !realOff) {
        std::cerr << "Missing FakeDataModel offsets\n";
        return 1;
    }

    auto fakeDM  = ReadPointer(proc.hProcess, proc.baseAddress + *fakeOff);
    if (!fakeDM) { std::cerr << "FakeDataModel pointer read failed\n"; return 1; }

    auto realDM  = ReadPointer(proc.hProcess, *fakeDM + *realOff);
    if (!realDM) { std::cerr << "RealDataModel pointer read failed\n"; return 1; }

    Instance dataModel(proc.hProcess, *realDM);
    std::cout << "DataModel: " << dataModel.GetClassName() << "\n\n";

    // ── Players ────────────────────────────────────────────────────────
    Instance players = dataModel.FindFirstChildOfClass("Players");
    if (!players) { std::cerr << "Players service not found\n"; return 1; }
    std::cout << "Players: 0x" << std::hex << players.GetAddress() << std::dec << "\n";

    // ── LocalPlayer ────────────────────────────────────────────────────
    auto lpOff = GetOffset("Player.LocalPlayer");
    if (!lpOff) { std::cerr << "Missing Player.LocalPlayer offset\n"; return 1; }

    auto lp = ReadPointer(proc.hProcess, players.GetAddress() + *lpOff);
    if (!lp) { std::cerr << "LocalPlayer pointer read failed\n"; return 1; }

    Instance localPlayer(proc.hProcess, *lp);
    std::cout << "LocalPlayer: " << localPlayer.GetName() << "\n\n";

    // ── Bullet count ───────────────────────────────────────────────────
    auto bullets = dataModel.FindFirstChild("ReplicatedStorage")
                       .FindFirstChild("Weapons")
                       .FindFirstChild("DesertEagle")
                       .FindFirstChild("Bullets");

    if (!bullets) { std::cerr << "Bullets not found\n"; return 1; }

    std::cout << "Bullets class: " << bullets.GetClassName() << "\n";

    if (bullets.GetClassName() == "IntValue") {
        int count = bullets.GetField<int>("Misc.Value");
        std::cout << "Bullets: " << count << "\n";

        bullets.SetField("Misc.Value", 99);
        std::cout << "Set to: " << bullets.GetField<int>("Misc.Value") << "\n";
    }

    CloseProcess(proc);

    std::cout << "\nPress Enter to exit...";
    std::cin.get();
    return 0;
}
