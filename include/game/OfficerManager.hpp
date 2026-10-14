#pragma once

#include "core/IMemoryDriver.hpp"
#include "game/Officer.hpp"
#include "game/Bounds.hpp"
#include "game/Protagonist.hpp"
#include <vector>
#include <memory>

namespace San13::Game {

/**
 * @brief 800명 무장 데이터의 읽기/쓰기 및 비즈니스 로직을 총괄하는 도메인 매니저
 */
class OfficerManager {
public:
    OfficerManager();
    ~OfficerManager();

    /**
     * @brief 매니저 초기화 및 루트 주소 바인딩
     */
    bool Initialize(Core::IMemoryDriver* driver, uintptr_t rootBase);

    /**
     * @brief [EDGE-25] 실시간 인게임 시나리오 일자 (년/월/일) 읽기
     */
    bool GetGameDate(uint16_t& outYear, uint16_t& outMonth, uint16_t& outDay);

    /**
     * @brief 특정 무장 ID의 데이터 읽기
     */
    bool LoadOfficer(uint16_t id, OfficerRecord& outRecord);

    /**
     * @brief 무장 데이터 일괄 파싱 및 로드 (includeExtended=true 시 결혼 자녀 및 신무장 슬롯 자동 스캔)
     */
    bool LoadAllOfficers(std::vector<OfficerRecord>& outRecords, bool includeExtended = true);

    /**
     * @brief 특정 무장의 스탯 블록 전체 쓰기 (Bounds 검증 적용)
     */
    bool WriteStats(uint16_t id, const San13OfficerLiveStats& stats, ClampingPolicy policy = ClampingPolicy::SafeVanilla);

    /**
     * @brief 개별 능력치 수정 헬퍼 (기본치 + 현재치 동시 동기화)
     */
    bool SetLeadership(uint16_t id, int64_t val, ClampingPolicy policy = ClampingPolicy::SafeVanilla);
    bool SetWar(uint16_t id, int64_t val, ClampingPolicy policy = ClampingPolicy::SafeVanilla);
    bool SetIntel(uint16_t id, int64_t val, ClampingPolicy policy = ClampingPolicy::SafeVanilla);
    bool SetPolitics(uint16_t id, int64_t val, ClampingPolicy policy = ClampingPolicy::SafeVanilla);

    /**
     * @brief 병종 적성 수정 (Strict Guard 적용: 0~3 강제 마스킹)
     */
    bool SetAptitudes(uint16_t id, int64_t spear, int64_t horse, int64_t bow);

    /**
     * @brief 사망년도 수정 (수명 연장/단축)
     */
    bool SetDeathYear(uint16_t id, uint16_t deathYear);

    /**
     * @brief 수명 연장 (+addYears)
     */
    bool ExtendLife(uint16_t id, int addYears);

    /**
     * @brief 보유 전법 설정 (+0x24, 0~140)
     */
    bool SetTactic(uint16_t id, uint8_t tacticId);

    /**
     * @brief 얼굴 CG 번호 (Face ID) 설정 (+0x08)
     */
    bool SetFaceId(uint16_t id, uint16_t faceId);

    /**
     * @brief 성격 설정 (+0x4E: 0소심, 1냉정, 2대담, 3저돌)
     */
    bool SetPersonality(uint16_t id, uint8_t personality);

    /**
     * @brief 20대 특기 레벨 일괄 설정 (+0x26 ~ +0x39, 각 0~9)
     */
    bool SetSkills(uint16_t id, const uint8_t skills[20]);

    /**
     * @brief 무장 8대 성향/기호/가치관 설정 (+0x50 ~ +0x58)
     */
    bool SetCharacter(uint16_t id, const San13OfficerCharacter& character);

    /**
     * @brief 의리/용애/재애 실적 쓰기 (+0x58 int16, +0x5A uint16, +0x5C uint16, BUG-06)
     */
    bool SetValueMerits(uint16_t id, int16_t justice, uint16_t courage, uint16_t talent);

    /**
     * @brief 무장 가족 및 인연 관계 설정 (+0x40 ~ +0x4D: 부친, 모친, 배우자 1~3, 의형제, 원수)
     */
    bool SetRelations(uint16_t id, uint16_t father, uint16_t mother, uint16_t spouse1, uint16_t swornBrother,
                      uint16_t spouse2 = 0xFFFF, uint16_t spouse3 = 0xFFFF, uint16_t rival = 0xFFFF);

    /**
     * @brief 의형제 3인 슬롯 설정 (+0x1296, +0x1298, +0x129A 및 헤더 +0x4A)
     */
    bool SetSwornBrothers(uint16_t id, const uint16_t brothers[3]);

    /**
     * @brief 친애 무장 6인 슬롯 설정 (+0x1284 ~ +0x128E)
     */
    bool SetFriends(uint16_t id, const uint16_t friends[6]);

    /**
     * @brief 원수 3인 슬롯 설정 (+0x129C, +0x129E, +0x12A0 및 헤더 +0x4C)
     */
    bool SetRivals(uint16_t id, const uint16_t rivals[3]);

    /**
     * @brief 다중 무장 스탯 일괄 가산/감산 (Batch Delta)
     */
    bool BatchApplyStatsDelta(const std::vector<uint16_t>& ids,
                              int dLead, int dWar, int dIntel, int dPol,
                              ClampingPolicy policy = ClampingPolicy::SafeVanilla);

    /**
     * @brief 다중 무장 스탯 일괄 설정 (Batch Set)
     */
    bool BatchApplyStatsSet(const std::vector<uint16_t>& ids,
                            int lead, int war, int intel, int pol,
                            bool setLead, bool setWar, bool setIntel, bool setPol,
                            ClampingPolicy policy = ClampingPolicy::SafeVanilla);

    /**
     * @brief 다중 무장 병종 적성 일괄 설정 (Strict Guard 적용: 0~3)
     */
    bool BatchApplyAptitudes(const std::vector<uint16_t>& ids, int spear, int horse, int bow);

    /**
     * @brief 다중 무장 수명 일괄 연장
     */
    bool BatchExtendLife(const std::vector<uint16_t>& ids, int addYears);

    /**
     * @brief 다중 무장 상태 스냅샷 일괄 캡처 (롤백/트랜잭션용)
     */
    bool CreateSnapshots(const std::vector<uint16_t>& ids, std::vector<OfficerSnapshot>& outSnapshots);

    /**
     * @brief 스냅샷 기반 일괄 롤백 (실행 취소 / Undo)
     */
    bool BatchRestore(const std::vector<OfficerSnapshot>& snapshots);


    /**
     * @brief [Phase 6.6] 주인공(플레이어) 정보 조회
     */
    bool GetProtagonistInfo(ProtagonistRecord& outProtagonist);

    /**
     * @brief [Phase 6.6] 주인공 개인 소지금 수정
     */
    bool SetProtagonistGold(uint32_t gold);

    /**
     * @brief 현재 플레이어 무장 ID 조회 (미확인 시 0xFFFF)
     */
    uint16_t GetProtagonistOfficerId();

    /**
     * @brief 특정 무장이 현재 플레이어인지 여부
     */
    bool IsProtagonist(uint16_t officerId);

    /**
     * @brief 특정 무장의 메모리 주소 산출 공식
     */
    uintptr_t GetOfficerAddress(uint16_t id) const;

private:
    // [EDGE-56] VTable 값이 '모듈 베이스 + rva'와 일치하는지 검사 (ASLR/Proton 로드 베이스 변동 대응)
    bool IsExpectedVTable(uint32_t vtable, uint32_t rva) const;

    Core::IMemoryDriver* m_driver;
    uintptr_t m_rootBase;
    uintptr_t m_moduleBase;       // 게임 모듈 로드 베이스 (VTable 검증용, 실행마다 변동 가능)
    uintptr_t m_officerArrayBase;
    uintptr_t m_protagonistBase;
};

} // namespace San13::Game
