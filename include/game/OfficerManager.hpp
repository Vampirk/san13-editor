#pragma once

#include "core/IMemoryDriver.hpp"
#include "game/Officer.hpp"
#include "game/Bounds.hpp"
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
     * @brief 무장 가족 및 인연 관계 설정 (+0x40 ~ +0x4D: 부친, 모친, 배우자 1~3, 의형제, 원수)
     */
    bool SetRelations(uint16_t id, uint16_t father, uint16_t mother, uint16_t spouse1, uint16_t swornBrother,
                      uint16_t spouse2 = 0xFFFF, uint16_t spouse3 = 0xFFFF, uint16_t rival = 0xFFFF);

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
     * @brief 특정 무장의 메모리 주소 산출 공식
     */
    uintptr_t GetOfficerAddress(uint16_t id) const;

private:
    Core::IMemoryDriver* m_driver;
    uintptr_t m_rootBase;
    uintptr_t m_officerArrayBase;
};

} // namespace San13::Game
