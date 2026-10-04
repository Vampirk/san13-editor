#include "core/LinuxMemoryDriver.hpp"
#include "game/GameVersion.hpp"
#include "game/OfficerManager.hpp"
#include "game/ForceManager.hpp"
#include "game/SettlementManager.hpp"
#include "game/CityManager.hpp"
#include <iostream>
#include <iomanip>

using namespace San13::Core;
using namespace San13::Game;

int main() {
    LinuxMemoryDriver driver;
    if (!driver.Attach("San13PK_kr.exe") && !driver.Attach("san13pk")) return 1;
    uintptr_t rootBase = 0;
    std::string verDesc;
    if (!GameVersionDetector::DetectRootBase(driver, rootBase, verDesc)) return 1;

    OfficerManager offMgr;
    offMgr.Initialize(&driver, rootBase);
    ForceManager::Initialize(&driver, rootBase);
    CityManager cityMgr;
    cityMgr.Initialize(&driver, rootBase);
    SettlementManager setMgr;
    setMgr.Initialize(&driver, rootBase);

    uint16_t testIds[] = { 333, 83, 18, 118, 610, 94, 480, 635, 355 };
    const char* names[] = { "조조", "하후돈", "원소", "안량", "유비", "관우", "동탁", "여포", "손견" };

    std::cout << "=== [무장의 실제 force_id 및 소속 세력] ===\n";
    for (int i = 0; i < 9; i++) {
        OfficerRecord rec;
        if (offMgr.LoadOfficer(testIds[i], rec)) {
            std::cout << std::setw(8) << names[i] << " (ID " << testIds[i] << "): "
                      << "force_id = " << rec.force_id
                      << " (" << ForceManager::GetForceName(rec.force_id) << ")"
                      << ", status_flag = " << (int)rec.status_flag
                      << ", rank = " << (int)rec.rank
                      << ", city_id = " << (int)rec.city_id
                      << "\n";
        }
    }

    std::cout << "\n=== [도시 0~10 의 소속 정보 및 무장/세력] ===\n";
    for (int cid = 0; cid < 10; cid++) {
        CityRecord crec;
        if (cityMgr.LoadCity(cid, crec)) {
            // 도시 raw 구조체에서 세력/군주 오프셋 찾아보기
            uintptr_t cAddr = rootBase + CityMemoryLayout::CHUNK_OFFSET + (cid * CityMemoryLayout::OBJECT_STRIDE);
            uint16_t w4 = 0, w6 = 0, w8 = 0, w0a = 0, w10 = 0, w12 = 0;
            driver.Read<uint16_t>(cAddr + 0x04, w4);
            driver.Read<uint16_t>(cAddr + 0x06, w6);
            driver.Read<uint16_t>(cAddr + 0x08, w8);
            driver.Read<uint16_t>(cAddr + 0x0A, w0a);
            driver.Read<uint16_t>(cAddr + 0x10, w10);
            driver.Read<uint16_t>(cAddr + 0x12, w12);
            std::cout << "도시 " << cid << " [" << crec.name << "]: "
                      << "+0x04=" << w4 << " "
                      << "+0x06=" << w6 << " "
                      << "+0x08=" << w8 << " "
                      << "+0x0A=" << w0a << " "
                      << "+0x10=" << w10 << " "
                      << "+0x12=" << w12 << " | "
                      << "소속무장수=" << crec.officer_count << "\n";
        }
    }

    std::cout << "\n=== [부락 0~10 의 원시 데이터 덤프] ===\n";
    for (int sid = 0; sid < 10; sid++) {
        uintptr_t sAddr = rootBase + SettlementMemoryLayout::CHUNK_OFFSET + (sid * SettlementMemoryLayout::OBJECT_STRIDE);
        uint8_t buf[28];
        driver.ReadRaw(sAddr, buf, 28);
        SettlementRecord srec;
        setMgr.LoadSettlement(sid, srec);
        std::cout << "부락 " << sid << " [" << srec.name << "]: ";
        for (int b = 0; b < 28; b++) {
            std::cout << std::hex << std::setw(2) << std::setfill('0') << (int)buf[b] << " ";
        }
        std::cout << std::dec << std::setfill(' ') << "\n";
    }

    return 0;
}
