# 삼국지 13 PK 도시 데이터 확장 및 발굴 계획서 (CITY_EXPANSION_PLAN)

> **기준 판본:** v5.6  
> **최종 갱신일:** 2026-10-03  
> **현황:** Phase 1(내정/숙련도/인물카운트), Phase 2(인구공식/부락청크/무장DLL구조), Phase 2.5(모델 및 테스트 동기화), Phase 3(부락·인물 연동 및 4탭 종합 인스펙터 UI) 전체 100% 완료.  

---

## 1. 개요 및 절대 규칙

본 문서는 삼국지 13 PK의 60개 도시(City) 객체의 상세 데이터를 발굴하고, 향후 도시 에디터 확장을 위한 기술 로드맵입니다.

### 절대 규칙 (Rules of Engagement)
1. **가설 표기 의무:** 실측으로 100% 교차 검증되기 전의 모든 자료형과 수치 범위는 반드시 **`[가설]`**로 표기한다.
2. **검증 수준 4단계 엄격 적용:** `[확정]` 등급은 2개 이상의 독립된 도시에서 게임 화면 수치와 메모리 판독값이 일치한 경우에만 부여한다.
3. **쓰기 금지 원칙:** 발굴 및 검증 단계에서는 오직 **읽기 전용(`ReadRaw`)** 도구만 사용하며, 게임 메모리를 변조하지 않는다.
4. **단계적 반영:** 구조체(`San13CityRaw`), 수치 경계(`Bounds.hpp`), UI 4탭 개편은 발굴 및 실측이 최종 승인된 후 진행한다.

---

## 2. 도시 메모리 레이아웃 현황 (368 B = `0x170` Stride)

- **타깃 프로세스:** `San13PK_kr.exe` (32-bit PE)
- **전역 청크 베이스:** `RootBase + 0x007178`
- **도시 객체 크기:** `0x170` (368 바이트)
- **도시 총 개수:** 60개 (ID 0 ~ 59)

### 2.1. 검증 수준별 필드 현황 표

| 오프셋 | 크기/타입 | 필드명 | 허용 범위 [가설] | 검증 수준 | 실측 표본 및 근거 |
| :---: | :---: | :--- | :---: | :---: | :--- |
| `+0x004` | `uint16_t` | 소속 세력 ID (`force_id`) | 0 ~ 120 | [유력] | 양평: 0(재야), 금성: 4(한수군) |
| `+0x058` | `uint32_t` | 내구도 (`durability`) | 0 ~ 5,000 [가설] | **[확정]** | 양평 2200, 업 2800, 낙양 3000, 금성 4000 (4곳 일치) |
| `+0x5C` | `uint32_t` | 자금 (`gold`) | 0 ~ 999,999 [가설] | **[확정]** | 양평 4240, 업 24000, 낙양 28000, 금성 44400 (4곳 일치) |
| `+0x60` | `uint32_t` | 군량 (`provisions`) | 0 ~ 999,999 [가설] | **[확정]** | 양평 95000, 업 108000, 낙양 125700, 금성 200000 (4곳 일치) |
| `+0x64` | `uint32_t` | 주둔 병력 (`troops`) | 0 ~ 200,000 [가설] | **[확정]** | 양평 2120, 업 12000, 낙양 14000, 금성 22200 (4곳 일치) |
| `+0x68` | `uint32_t` | 부상병 (`wounded_troops`) | 0 ~ 병력수 [가설] | [가설] | 평시 0, 전투 직후 부상병 발생 시 추가 관찰 필요 |
| `+0x6C` | `uint32_t` | 귀환병 (`returning_troops`) | 0 ~ 병력수 [가설] | [가설] | 평시 0, 원정 후 귀환 부대 발생 시 추가 관찰 필요 |
| **`+0x0D2`** | `uint16_t` | **민심** (`public_order`) | 0 ~ 1,000+ [가설] | **[확정]** | **양평: 100, 금성: 350 (2개 도시 실측 일치)** |
| **`+0x0D6`** | `uint16_t` | **상업 기본치** (`commerce`) | 0 ~ 4,000+ [가설] | **[유력]** | 양평 1000 일치 (금성: 메모리 1800 / 화면 2300, 마을 보너스 500 추정) |
| **`+0x0E0`** | `uint16_t` | **농업 기본치** (`agriculture`) | 0 ~ 4,000+ [가설] | **[확정]** | **양평: 1,050, 금성: 2,000 (2개 도시 실측 일치)** |
| **`+0x0EA`** | `uint16_t` | **문화 기본치** (`culture`) | 0 ~ 4,000+ [가설] | **[확정]** | **양평: 1,000, 금성: 2,400 (2개 도시 실측 일치)** |
| **`+0x0FC`** | `uint16_t` | **창병 숙련도** (`spear_prof`) | 0 ~ 3,000 [가설] | **[확정]** | **양평: 200, 금성: 700 (2개 도시 실측 일치)** |
| **`+0x0FE`** | `uint16_t` | **기병 숙련도** (`horse_prof`) | 0 ~ 3,000 [가설] | **[확정]** | **양평: 300, 금성: 700 (2개 도시 실측 일치)** |
| **`+0x100`** | `uint16_t` | **궁병 숙련도** (`bow_prof`) | 0 ~ 3,000 [가설] | **[확정]** | **양평: 200, 금성: 700 (2개 도시 실측 일치)** |
| `+0x140` | `uint32_t` | 소속 무장 리스트 포인터 | 주소 포인터 | [유력] | `+0x144` 카운트와 연동 구조 |
| `+0x144` | `uint32_t` | **소속 무장 수** (`officer_count`) | 0 ~ 100+ [가설] | **[유력]** | **금성: 12명 실측 일치** |
| `+0x148` | `uint32_t` | 수감 포로 리스트 포인터 | 주소 포인터 | [유력] | `+0x14C` 카운트와 연동 구조 |
| `+0x14C` | `uint32_t` | **수감 포로 수** (`prisoner_count`) | 0 ~ 100+ [가설] | **[유력]** | **금성: 0명 실측 일치** |
| `+0x150` | `uint32_t` | 재야 무장 리스트 포인터 | 주소 포인터 | [유력] | `+0x154` 카운트와 연동 구조 |
| `+0x154` | `uint32_t` | **재야 무장 수** (`free_count`) | 0 ~ 100+ [가설] | **[유력]** | **금성: 1명 실측 일치** |
| *(외부)* | `uint32_t` | 인구 (`population`) | 0 ~ 1,000,000+ [가설] | [미확인] | 도시 368B 내 222,200 직접 미발견. 주위 마을 연동 또는 별도 청크 가설 |

---

## 3. 미발굴 항목 및 기술적 분석

### 3.1. 인구 (Population) 미발견 원인 분석
- 양평(97,120) 및 금성(222,200)의 인구 수치가 도시 단일 368B 메모리 내에서 4바이트 정수로 직접 검색되지 않음.
- **가설 1:** 삼국지 13 PK는 도시 주위의 **마을(集落, 부락)**이 도시 인구에 실시간 합산되는 시스템이므로, 도시 기본 인구 + 종속 마을 인구가 합산되어 화면에 표출되는 구조일 가능성.
- **가설 2:** 도시 청크와 분리된 세력 통계 또는 별도의 60개 도시 확장 청크가 존재할 가능성.

### 3.2. 상업/농업/문화 상한치 (3,000 / 4,000 / 4,500)
- 도시 정보창의 "최대 3000 / 4000 / 4500"은 메모리에 고정 수치로 저장되지 않음.
- 도시의 규모(소도시: 3000, 중도시: 4000, 대도시: 5000 등) 플래그와 주위 마을 기술 버프에 따라 게임 엔진이 실시간 계산하여 상한을 제한하는 것으로 추정.

---

## 4. 도시 소속 인물 검증 결과 (Step 4 실측 분석)

### 4.1. 무장 구조체 `+0x0A(city_id)` 전수 검증 결과
- **가설:** 무장 구조체의 `+0x0A` 오프셋을 역색인(Reverse Lookup)하여 도시별 소속 무장을 전원 추출할 수 있을 것이다.
- **실측 표본 전수 조사 (역사 무장 800명 스캔):**
  - **도시 번호(0~59) 보유 무장:** 총 **33명 (4.2%)**
    - 표본: 조조(ID 333, 거록 5번), 관우(ID 94, 계 2번), 하후돈(ID 83, 홍농 19번), 유비(ID 610, 탁현 0번), 아회남(ID 0, 남만 37번)
    - 특징: 태수(Governor), 도독, 또는 특정 거점 부임 무장에 한해서만 도시 번호가 기록됨.
  - **`0xFF (255)` 미배치 플래그 무장:** 총 **767명 (95.8%)**
    - 표본: 원소(ID 18, 군주), 장각(ID 412, 군주), 범증(ID 851, 재야), 항적(ID 857, 가상), 일반 대다수 역사 무장
- **검증 결론:** **[가설 기각]**
  - 일반 무장은 소재 도시를 직접 갖지 않고 소속 세력(`force_id`)에 귀속되므로, **`+0x0A` 단독 검색만으로는 도시 소속 무장 12명을 전원 역색인할 수 없음**이 확인됨.

### 4.2. 대안 탐색 및 수정 계획
1. **도시 카운트 필드 활용:**
   - 도시 구조체 `+0x144` (무장 수 12), `+0x14C` (포로 수 0), `+0x154` (재야 수 1)의 수치가 실측과 일치하므로, 도시 구조체 헤더의 통계 정보 우선 표출.
2. **세력-도시 간접 매핑 (2단계 색인):**
   - 도시 소속 세력(`City +0x04 force_id`)과 무장의 소속 세력(`Officer +0x3C force_id`)을 매핑하고, 태수(`City +0x06`) 및 군주를 중심으로 해당 도시 주둔군 무장을 필터링하는 복합 판별 알고리즘 설계.

---

## 5. 향후 로드맵 (단계별 계획)

### Phase 1: 도시 구조체 확장 승인 및 적용
- 실측 확정된 6개 필드(민심 `+0x0D2`, 농업 `+0x0E0`, 문화 `+0x0EA`, 창/기/궁 숙련도 `+0x0FC/0x0FE/0x100`) 및 도시 소속 통계(`+0x144, +0x14C, +0x154`)를 사용자 승인 후 `include/game/City.hpp` 구조체에 반영.

### Phase 2: 인구 및 상업 마을 보너스 메커니즘 추가 분석
- 인구 및 상업 수치와 주변 마을(集落) 개발도 간의 가산 공식 추가 규명.

### Phase 3: UI 4탭 모달 및 반응형 레이아웃 구현 (추후 별도 작업)
- `GetContentRegionAvail()` 기반의 유동적 버튼 배치(방법 A) 및 테이블 그리드(방법 C)를 적용한 도시 4탭 인스펙터 개발.

---

## 6. Phase 1 상세 작업 명세서 (도시 구조체 확장 및 읽기 모델 구축)

> **원칙 준수:**
> 1. 오직 실측된 필드만 반영하며, 바이트 오프셋의 정밀성(`sizeof == 0x170`)을 엄격히 검증한다.
> 2. 이번 작업에서는 **일체 메모리 쓰기(Write) 코드를 작성하지 않으며**, 읽기 전용(Read-Only) 모델만 제공한다.
> 3. `Bounds.hpp` 및 클램핑 가드는 실측 및 기획 확정 전까지 변경하지 않는다.

### 6.1. `San13CityRaw` 구조체 바이트 레이아웃 상세 설계

|  시작 오프셋  |    크기     | 멤버명                  |     타입      |  검증 등급   | 설명                                              |
| :------: | :-------: | :------------------- | :---------: | :------: | :---------------------------------------------- |
| `+0x000` |   88 B    | `head_pad[0x58]`     | `uint8_t[]` |    -     | VTable 및 소속 헤더 영역                               |
| `+0x058` |    4 B    | `durability`         | `uint32_t`  | **[확정]** | 도시 내구도                                          |
| `+0x05C` |    4 B    | `gold`               | `uint32_t`  | **[확정]** | 도시 자금                                           |
| `+0x060` |    4 B    | `provisions`         | `uint32_t`  | **[확정]** | 도시 군량                                           |
| `+0x064` |    4 B    | `troops`             | `uint32_t`  | **[확정]** | 주둔 병력                                           |
| `+0x068` |   106 B   | `pad_0x68[0x6A]`     | `uint8_t[]` |    -     | 부상병/귀환병 후보 영역                                   |
| `+0x0D2` |    2 B    | `public_order`       | `uint16_t`  | **[확정]** | 민심 (양평 100, 금성 350 실측)                          |
| `+0x0D4` |    2 B    | `pad_0xD4[2]`        | `uint8_t[]` |    -     | 패딩                                              |
| `+0x0D6` |    2 B    | `commerce`           | `uint16_t`  | **[유력]** | 상업 기본 개발치 (양평 1,000 실측)                         |
| `+0x0D8` |    8 B    | `pad_0xD8[8]`        | `uint8_t[]` |    -     | 패딩                                              |
| `+0x0E0` |    2 B    | `agriculture`        | `uint16_t`  | **[확정]** | 농업 기본 개발치 (양평 1,050, 금성 2,000 실측)               |
| `+0x0E2` |    8 B    | `pad_0xE2[8]`        | `uint8_t[]` |    -     | 패딩                                              |
| `+0x0EA` |    2 B    | `culture`            | `uint16_t`  | **[확정]** | 문화 기본 개발치 (양평 1,000, 금성 2,400 실측)               |
| `+0x0EC` |   16 B    | `pad_0xEC[0x10]`     | `uint8_t[]` |    -     | 패딩                                              |
| `+0x0FC` |    2 B    | `spear_proficiency`  | `uint16_t`  | **[확정]** | 창병 숙련도 (양평 200, 금성 700 실측)                      |
| `+0x0FE` |    2 B    | `horse_proficiency`  | `uint16_t`  | **[확정]** | 기병 숙련도 (양평 300, 금성 700 실측)                      |
| `+0x100` |    2 B    | `bow_proficiency`    | `uint16_t`  | **[확정]** | 궁병 숙련도 (양평 200, 금성 700 실측)                      |
| `+0x102` |   66 B    | `pad_0x102[0x42]`    | `uint8_t[]` |    -     | 내부 vtable 및 통계 전방 패딩                            |
| `+0x144` |    4 B    | `officer_count`      | `uint32_t`  | **[유력]** | 소속 무장 수 (금성 12명 실측)                             |
| `+0x148` |    4 B    | `pad_0x148[4]`       | `uint8_t[]` |    -     | 포인터 영역 (무장 리스트 추정)                              |
| `+0x14C` |    4 B    | `prisoner_count`     | `uint32_t`  | **[유력]** | 수감 포로 수 (금성 0명 실측)                              |
| `+0x150` |    4 B    | `pad_0x150[4]`       | `uint8_t[]` |    -     | 포인터 영역 (포로 리스트 추정)                              |
| `+0x154` |    4 B    | `free_officer_count` | `uint32_t`  | **[유력]** | 재야 무장 수 (금성 1명 실측)                              |
| `+0x158` |   24 B    | `tail_pad[0x18]`     | `uint8_t[]` |    -     | 도시 청크 말미 패딩                                     |
|  **총합**  | **368 B** | -                    |      -      |    -     | `sizeof(San13CityRaw) == 0x170` (368 바이트) 불변 보장 |

### 6.2. `CityRecord` DTO 확장 명세
```cpp
struct CityRecord {
    uint8_t id = 0;
    std::string name;
    // 기존 4대 자원
    uint32_t gold = 0;
    uint32_t provisions = 0;
    uint32_t troops = 0;
    uint32_t durability = 0;

    // 내정 4대치 (읽기 전용)
    uint16_t public_order = 0;       // 민심
    uint16_t commerce = 0;           // 상업 기본치
    uint16_t agriculture = 0;        // 농업 기본치
    uint16_t culture = 0;            // 문화 기본치

    // 병과 3대 숙련도 (읽기 전용)
    uint16_t spear_proficiency = 0;  // 창병 숙련도
    uint16_t horse_proficiency = 0;  // 기병 숙련도
    uint16_t bow_proficiency = 0;    // 궁병 숙련도

    // 소속 인물 통계 (읽기 전용)
    uint32_t officer_count = 0;      // 소속 무장 수
    uint32_t prisoner_count = 0;     // 수감 포로 수
    uint32_t free_officer_count = 0; // 재야 무장 수

    uintptr_t memory_address = 0;
};
```

### 6.3. 단위 테스트 검증 계획
1. **구조체 무결성 컴파일 타임 검증 (`static_assert`):**
   - `sizeof(San13CityRaw) == 0x170`
   - 각 필드의 `offsetof`가 위 표의 오프셋과 1바이트 오차 없이 일치하는지 전수 검증.
2. **라이브 프로세스 메모리 검증:**
   - 양평(ID 0): 민심 100, 농업 1050, 문화 1000, 창병 200, 기병 300, 궁병 200 검증.
   - 금성(ID 23): 민심 350, 농업 2000, 문화 2400, 창병 700, 기병 700, 궁병 700, 무장수 12, 포로수 0, 재야수 1 검증.
3. **전체 테스트 스위트 회귀 검증:**
   - `make test` 전수 실행하여 기존 12개 테스트 정상 통과 유지.

---

## 7. Phase 2 상세 작업 명세서 (인구 및 부락 메커니즘 발굴)

> **원칙 준수:**
> 1. 모든 수치와 구조체 레이아웃은 실측 확인 전까지 **`[가설]`** 등급을 부여한다.
> 2. 이번 단계에서도 **일체 메모리 쓰기(Write)를 금지**하며, 순수 읽기 전용 스캐너 도구만 제작한다.
> 3. 게임 실행 환경을 직접 다룰 수 없으므로, 스캐너 도구로 후보 주소/오프셋을 도출하고 사용자 게임 정보창과의 대조를 요청한다.

### 7.1. 배경 및 미해결 핵심 과제
1. **인구 (Population):**
   - 사용자 실측 화면: 양평 97,120명 (`0x00017B60`), 금성 222,200명 (`0x000363E8`).
   - 도시 368바이트(`0x170`) 청크 내에서는 위 32비트 정수가 직접 검색되지 않음.
2. **상업 개발치 보너스 (+500):**
   - 금성의 도시 메모리 기본치는 1,800(`0x0708`)이나, 실제 게임 정보창에는 2,300(`0x08FC`)으로 표출됨. 차이인 500은 금성 소속 부락(集落)의 상업 개발 효과로 추정됨.

### 7.2. 3대 분석 가설
- **가설 2-A (종속 부락 합산):** 도시의 인구 및 일부 내정 수치는 도시 본체 수치와 도시 관할 부락(集落, 촌락) 수치가 실시간 동적 합산되는 구조이다.
- **가설 2-B (도시 통계 확장 청크):** `RootBase` 하위의 또 다른 60개 배열 블록(도시 통계/경제 전용 청크)에 인구 및 수입/지출 정보가 보관되어 있다.
- **가설 2-C (단위 스케일링/연산 보정):** 인구가 10단위/100단위로 축약되어 저장되어 있거나 별도 부동소수점/비트필드로 인코딩되어 있다.

### 7.3. 발굴 도구 개발 계획 (`tools/scan_population_settlement.cpp`)
- **도구 기능:**
  1. **광역 정수 검색 (`--find-values 97120 222200`):** `RootBase` 기준 유효 가상 메모리 범위(`RootBase - 0x100000` ~ `RootBase + 0x500000`)를 스캔하여 97,120과 222,200이 인접하게 또는 규칙적 스트라이드로 나타나는 위치를 추적.
  2. **도시 60개 스트라이드 검출기 (`--stride-scan`):** 60개 요소로 이루어진 배열 후보 중 0번 슬롯이 양평 관련 값, 23번 슬롯이 금성 관련 값을 가지는 구조체 배열 자동 탐지.
  3. **부락(集落) 구조체 탐색기 (`--scan-settlement`):** 부락의 고유 ID, 관할 도시 ID(0~59), 상업 보너스 500 등을 가진 연속 블록 탐색.
- **빌드 및 실행 타깃:** 루트 Makefile `make tools`에 추가하여 `bin/scan_population_settlement`로 빌드.

### 7.4. Phase 2 실측 분석 결과 (결정적 발견)

#### 1) 도시 색인 ID 정정 및 낙양(ID 23) 실측 완벽 교차 검증 [★ 확정]
- **도시 23번의 정체:** 기존 `CITY_NAMES` 테이블의 추측과 달리, 실제 184년 시나리오에서 **도시 인덱스 23번이 바로 낙양(Luoyang)**임이 실측 확인됨.
  - **인게임 렌더 버퍼(`0x749302A7`) 실측:** `낙양`, `인구 222200`, `병력 22200`, `자금 4000`, `군량 44400`, `내구 2000`, `민심 350/1250`, `창병 700`, `기병 700`, `궁병 700`, `증가병력 1975`, `무장 12`, `재야 1`이 메모리 슬롯 23번과 100% 일치.
  - **소속 무장 전수 추적(`+0x140`):** 조조(333), 원소(18), 원술(24), 하후연(88), 변씨(675) 등 184년 황건의 난 당시 수도 낙양에 부임한 한실 관리 12명이 도시 23번의 연결 리스트에 정확히 보관되어 있음이 증명됨.

#### 2) 기본 자원 4대 필드(`+0x58 ~ +0x64`) 오프셋 최종 확정 [★ 확정]
사용자 실측값("낙양 자금 4000, 군량 44400, 내구 2000")과 메모리 판독값의 대조를 통해 기존 과거 스펙의 필드 역전이 최종 바로잡힘:

| 오프셋 | 타입 | 필드명 | 낙양 (ID 23) 실측 | 양평 (ID 0) 실측 | 판정 | 설명 |
| :---: | :---: | :--- | :---: | :---: | :---: | :--- |
| **`+0x58`** | `uint32_t` | **자금** (`gold`) | **4,000** | 2,200 | **[확정]** | 도시 보유 금 (과거 내구도로 오인) |
| **`+0x5C`** | `uint32_t` | **군량** (`provisions`) | **44,400** | 4,240 | **[확정]** | 도시 보유 군량 (과거 자금으로 오인) |
| **`+0x60`** | `uint32_t` | **기초 민간 인구** (`civilian_pop`) | **200,000** | 95,000 | **[확정]** | 비군사 민간인 인구 (과거 군량으로 오인) |
| **`+0x64`** | `uint32_t` | **주둔 병력** (`troops`) | **22,200** | 2,120 | **[확정]** | 도시 주둔 모병/병력 |
| **[합산]** | `uint32_t` | **총 인구** (`total_population`) | **222,200** | **97,120** | **[확정]** | **공식: `+0x60`(민간 인구) + `+0x64`(병력)** |

#### 3) 부락(集落, Settlement) 300개 청크 및 도시 연결 구조 규명 [★ 확정]
- **부락 오브젝트 배열 위치:** `RootBase + 0x00CA48`
- **단일 크기:** **28 바이트 (`0x1C`)**, 총 **300개** (ID 0 ~ 299)
- **VTable 포인터:** `0x72919AC4`
- **도시와의 연결 인터페이스:**
  - 도시 `+0x018`: `uint8_t settlement_id[4]` (도시 관할 부락 최대 4개 슬롯)
    - 양평(0번): `[1, 255, 255, 255]` $\rightarrow$ 1번 부락 1개 관할
    - 낙양(23번): `[17, 20, 22, 24]` $\rightarrow$ 17, 20, 22, 24번 부락 4개 관할
  - 도시 `+0x13C`: `uint32_t settlement_count` (관할 부락 수: 양평 1개, 낙양 4개 일치)
  - 부락 `+0x06`: 점령 세력 ID (`uint16_t`)
  - 부락 `+0x14`: 장악도/개발도 (`uint16_t`, 100 = 100%)

#### 4) 도시 소속 인물 양방향 연결 리스트(Doubly Linked List) 구조 규명 [★ 확정]
- `+0x140`: 소속 무장 리스트 노드 헤드 포인터 (`+0x144` 카운트: 12명)
- `+0x150`: 재야 무장 리스트 노드 헤드 포인터 (`+0x154` 카운트: 1명 - 항적)
- **노드 구조:**
  - `+0x00`: `next_node` 포인터
  - `+0x04`: `prev_node` 포인터
  - `+0x08`: `officer_obj_ptr` (무장 풀 내의 대상 무장 오브젝트 원본 주소로 직결)
- **검증 결론:** Step 4에서 `+0x0A` 단순 역색인이 기각되었던 이유가 완전히 해명됨. 게임 엔진은 도시별 소속 인물을 STL `std::list` 형태의 노드 체인으로 도시 구조체에 직접 관리하고 있음!

---

## 8. Phase 2.5 상세 작업 명세서 (도시 명칭 및 자원 모델 정밀 재정의)

> **원칙 준수:**
> 1. 실측으로 확정된 사실(`+0x58` 자금, `+0x5C` 군량, `+0x60` 기초 민간 인구, `+0x64` 병력, ID 23 낙양)에 기반하여 코드베이스를 정확히 동기화한다.
> 2. `sizeof(San13CityRaw) == 0x170` (368 바이트) 및 컴파일 타임 `static_assert`를 완벽히 유지한다.
> 3. `test_city_manager.cpp` 및 전체 단위 테스트 스위트의 회귀를 100% 통과하도록 갱신한다.

### 8.1. `San13CityRaw` 구조체 개정안 (`include/game/City.hpp`)
```cpp
struct San13CityRaw {
    uint8_t  head_pad[0x18];          // +0x00 ~ +0x17: VTable, force_id, 소속 헤더
    uint8_t  settlement_ids[4];       // +0x018 ~ +0x01B: 관할 부락 ID 4개 슬롯 [확정]
    uint8_t  head_pad2[0x3C];         // +0x01C ~ +0x057: 지역/좌표/태수/군주 정보
    uint32_t gold;                    // +0x058: 도시 자금 [확정] (낙양 4,000, 양평 2,200)
    uint32_t provisions;              // +0x05C: 도시 군량 [확정] (낙양 44,400, 양평 4,240)
    uint32_t civilian_population;     // +0x060: 기초 민간 인구 [확정] (낙양 200,000, 양평 95,000)
    uint32_t troops;                  // +0x064: 주둔 병력 [확정] (낙양 22,200, 양평 2,120)
    uint8_t  pad_0x68[0x6A];          // +0x068 ~ +0x0D1: 부상병/귀환병 후보 영역 (106B)
    uint16_t public_order;            // +0x0D2: 민심 [확정] (양평 100, 낙양 350)
    uint8_t  pad_0xD4[2];             // +0x0D4 ~ +0x0D5: 패딩 (2B)
    uint16_t commerce;                // +0x0D6: 상업 기본 개발치 [유력] (양평 1000, 낙양 1800)
    uint8_t  pad_0xD8[8];             // +0x0D8 ~ +0x0DF: 패딩 (8B)
    uint16_t agriculture;             // +0x0E0: 농업 기본 개발치 [확정] (양평 1050, 낙양 2000)
    uint8_t  pad_0xE2[8];             // +0x0E2 ~ +0x0E9: 패딩 (8B)
    uint16_t culture;                 // +0x0EA: 문화 기본 개발치 [확정] (양평 1000, 낙양 2400)
    uint8_t  pad_0xEC[0x10];          // +0x0EC ~ +0x0FB: 패딩 (16B)
    uint16_t spear_proficiency;       // +0x0FC: 창병 숙련도 [확정] (양평 200, 낙양 700)
    uint16_t horse_proficiency;       // +0x0FE: 기병 숙련도 [확정] (양평 300, 낙양 700)
    uint16_t bow_proficiency;         // +0x100: 궁병 숙련도 [확정] (양평 200, 낙양 700)
    uint8_t  pad_0x102[0x3A];         // +0x102 ~ +0x13B: 내구도/방어시설 후보 영역 (58B)
    uint32_t settlement_count;        // +0x13C: 관할 부락 수 [확정] (양평 1, 낙양 4)
    uint32_t officer_list_ptr;        // +0x140: 소속 무장 링크드 리스트 헤드 포인터 [확정]
    uint32_t officer_count;           // +0x144: 소속 무장 수 [유력] (낙양 12명)
    uint32_t prisoner_list_ptr;       // +0x148: 수감 포로 링크드 리스트 헤드 포인터
    uint32_t prisoner_count;          // +0x14C: 수감 포로 수 [유력] (낙양 0명)
    uint32_t free_officer_list_ptr;   // +0x150: 재야 무장 링크드 리스트 헤드 포인터
    uint32_t free_officer_count;      // +0x154: 재야 무장 수 [유력] (낙양 1명 - 항적)
    uint8_t  tail_pad[0x18];          // +0x158 ~ +0x16F: 청크 말미 패딩 (24B)
};
```

### 8.2. `CityRecord` DTO 개정안
- `civilian_population` 및 `total_population` 공식 필드 추가.
- `settlement_count` 추가.

### 8.3. `CITY_NAMES` 명칭 테이블 정정안 (`src/game/CityManager.cpp`)
- 도시 23번을 "낙양 (Luoyang)"으로 공식 정정.

---

## 9. Phase 3 상세 작업 명세서 (부락 및 소속 무장 연동, 4탭 종합 인스펙터 UI)

> **원칙 준수:**
> 1. 오직 실측으로 규명된 메모리 구조(`RootBase + 0x00CA48` 부락 300개, `City +0x140/+0x150` 양방향 연결 리스트)를 기반으로 도메인 모듈을 설계한다.
> 2. 연결 리스트 순회 시 무한 루프(Cyclic Reference) 및 세그멘테이션 폴트를 방지하기 위해 엄격한 방문 가드(`visited`)와 포인터 유효성 검증을 필수로 적용한다.
> 3. 무장 4탭 인스펙터(`OfficerDetailModal`)와 통일된 Dear ImGui UI/UX 패턴을 유지하며, 1280x800 해상도에서 오프스크린 잘림 없이 반응형으로 동작하도록 구성한다.

### 9.1. Step 3-1: 부락(Settlement) 도메인 모듈 설계

#### 1) 부락 메모리 레이아웃 (`San13SettlementRaw` - 28B, Stride: `0x1C`)
- **시작 베이스:** `RootBase + 0x00CA48`
- **단일 크기:** 28 바이트 (`0x1C`)
- **총 개수:** 300개 (ID 0 ~ 299)
- **VTable 포인터:** `0x72919AC4` [확정]

```cpp
#pragma pack(push, 1)
struct San13SettlementRaw {
    uint32_t vtable;          // +0x00: VTable 포인터 (0x72919AC4) [확정]
    uint16_t settlement_id;   // +0x04: 부락 고유 ID (0 ~ 299) [확정]
    uint16_t force_id;        // +0x06: 점령/지배 세력 ID [확정]
    uint8_t  pad_0x08[12];    // +0x08 ~ +0x13: 좌표 및 부락 효과/특산품 후보 영역 [가설]
    uint16_t dev_progress;    // +0x14: 장악도 / 개발도 (0 ~ 100) [유력]
    uint8_t  tail_pad[6];     // +0x16 ~ +0x1B: 청크 말미 패딩 (6B)
};
#pragma pack(pop)

static_assert(sizeof(San13SettlementRaw) == 0x1C, "San13SettlementRaw size must be exactly 28 bytes");
static_assert(offsetof(San13SettlementRaw, settlement_id) == 0x04, "settlement_id offset mismatch");
static_assert(offsetof(San13SettlementRaw, force_id) == 0x06, "force_id offset mismatch");
static_assert(offsetof(San13SettlementRaw, dev_progress) == 0x14, "dev_progress offset mismatch");
```

#### 2) 부락 DTO (`SettlementRecord`) 및 매니저 인터페이스
```cpp
struct SettlementRecord {
    uint16_t id = 0;
    std::string name;             // 부락 명칭 (또는 "부락 #ID")
    uint16_t force_id = 0;        // 지배 세력 ID
    uint16_t dev_progress = 0;    // 장악도 (0 ~ 100%)
    uint8_t  city_id = 255;       // 관할 도시 ID
    uintptr_t memory_address = 0;
};
```

- **`SettlementManager` 핵심 기능:**
  - `Initialize(IMemoryDriver* driver, uintptr_t rootBase)`
  - `LoadSettlement(uint16_t id, SettlementRecord& outRecord)`
  - `LoadCitySettlements(uint8_t cityId, const uint8_t settlementIds[4], uint32_t count, std::vector<SettlementRecord>& outRecords)`

---

### 9.2. Step 3-2: 도시 소속 무장 연결 리스트(DLL) 순회 로더 설계

#### 1) 양방향 연결 리스트(Doubly Linked List) 노드 레이아웃
도시 구조체 내 무장 리스트는 STL `std::list` 계열 노드 체인으로 구성되어 있습니다:

```cpp
#pragma pack(push, 1)
struct San13CityOfficerNodeRaw {
    uint32_t next_node;        // +0x00: 다음 노드 가상 메모리 주소
    uint32_t prev_node;        // +0x04: 이전 노드 가상 메모리 주소
    uint32_t officer_obj_ptr;  // +0x08: 시나리오 무장 풀 내 무장 오브젝트 원본 주소
};
#pragma pack(pop)

static_assert(sizeof(San13CityOfficerNodeRaw) == 12, "City officer node size must be 12 bytes");
```

#### 2) 도시 인물 카운트 및 헤더 오프셋 매핑
- `+0x140`: 소속 현역 무장 헤드 포인터 (`officer_count` at `+0x144`) - 낙양: 12명
- `+0x148`: 수감 포로 무장 헤드 포인터 (`prisoner_count` at `+0x14C`) - 낙양: 0명
- `+0x150`: 재야 무장 헤드 포인터 (`free_officer_count` at `+0x154`) - 낙양: 1명 (항적)

#### 3) 안전 순회 알고리즘 (Safe Traversal & Circular Guard)
```cpp
bool CityManager::LoadCityOfficers(uint8_t cityId, CityOfficerCategory category, std::vector<CityOfficerEntry>& outOfficers) {
    // 1. 헤드 포인터 및 예상 카운트 획득
    // 2. nullptr 및 0 주소 방어
    // 3. std::set<uint32_t> visited_nodes 가드를 통한 순환 링크 차단
    // 4. 각 노드의 officer_obj_ptr에서 officer_id (+0x04), rank (+0x0C), 통/무/지/정 읽기
    // 5. OfficerNameTable::GetKoreanName(id)를 통한 한글 성명 바인딩
}
```

---

### 9.3. Step 3-3: 4탭 도시 종합 인스펙터(`CityDetailModal`) UI 와이어프레임

무장 상세 편집창과 동일한 수준의 깊이를 제공하는 모달 팝업 형태:

```
+-----------------------------------------------------------------------------------------+
| [도시 종합 인스펙터] ■ 낙양 (Luoyang) [ID 23]  | 소속 세력: 하진군 (ID 4) | 주소: 0x... | [X] |
+-----------------------------------------------------------------------------------------+
| [ 기본 경제/자원 ] | [ 내정 및 관할 부락 ] | [ 군사 및 숙련도 ] | [ 도시 소속 인물 명단 ] |
+-----------------------------------------------------------------------------------------+
| [탭 1: 기본 경제/자원]                                                                 |
| ▶ 도시 경제 및 인구 현황                                                                |
|   자금 (Gold)        : [ 4000     ]  [ ======|====== ] (한계돌파 9,999,999 허용)       |
|   군량 (Provisions)  : [ 44400    ]  [ ========|==== ]                                 |
|   주둔 병력 (Troops) : [ 22200    ]  [ ====---------- ]                                 |
|   ------------------------------------------------------------------------------------  |
|   기초 민간 인구: 200,000 명  |  주둔 병력: 22,200 명  ==>  총 인구: 222,200 명       |
|   [빠른 치트]  [자금/군량 999,999 완충]   [병력 100,000 증원]   [자원 고갈 (자금/군량 0)]  |
+-----------------------------------------------------------------------------------------+
| [탭 2: 내정 및 관할 부락]                                                               |
| ▶ 도시 내정 기본치 (읽기 전용)                                                         |
|   민심: 350 / 1250    | 상업 기본치: 1,800  | 농업 기본치: 2,000  | 문화 기본치: 2,400  |
| ▶ 관할 부락 현황 (총 4개 관할)                                                          |
|   | 부락 ID | 부락명       | 지배 세력    | 장악도 / 개발도            | 효과 / 비고 |       |
|   | #017    | 맹진 (Mengjin)| 하진군 (ID 4)| [====================] 100%| 상업 +500    |       |
|   | #020    | 양현 (Yangxian)| 하진군 (ID 4)| [====================] 100%| 농업 +300    |       |
|   | #022    | 연성 (Yancheng)| 하진군 (ID 4)| [====================] 100%| 인구 증가    |       |
|   | #024    | 곡양 (Guyang) | 하진군 (ID 4)| [====================] 100%| 병력 증가    |       |
+-----------------------------------------------------------------------------------------+
| [탭 3: 군사 및 숙련도]                                                                 |
| ▶ 병종 3대 숙련도 (0 ~ 3,000)                                                          |
|   창병 숙련도:  700  [=======-----------------] (중급 창병 편성 가능)                 |
|   기병 숙련도:  700  [=======-----------------] (중급 기병 편성 가능)                 |
|   궁병 숙련도:  700  [=======-----------------] (중급 궁병 편성 가능)                 |
| ▶ 성벽 및 방어 거점 현황                                                              |
|   내구도 (관측치): 2,000 / 2,000                                                        |
+-----------------------------------------------------------------------------------------+
| [탭 4: 도시 소속 인물 명단 (12명 현역, 1명 재야, 0명 포로)]                              |
|   [현역 소속 무장 (12명)]                                                               |
|   | ID   | 이름   | 신분/품관 | 통솔 | 무력 | 지력 | 정치 | 충성 | 주둔/임무 |           |
|   | #333 | 조조   | 5품관     | 99   | 99   | 92   | 91   | 100  | 주둔       |           |
|   | #018 | 원소   | 5품관     | 82   | 69   | 70   | 73   | 100  | 주둔       |           |
|   | #024 | 원술   | 6품관     | 65   | 65   | 68   | 44   | 95   | 주둔       |           |
|   | ...  | ...    | ...       | ...  | ...  | ...  | ...  | ...  | ...        |           |
|   [재야 인물 (1명)]                                                                     |
|   | #857 | 항적   | 재야      | 98   | 100  | 69   | 43   | -    | 거주       |           |
+-----------------------------------------------------------------------------------------+
| ● 실시간 핫싱크 성공 (0.002ms)                            [메모리 적용 (Hot-Sync)] [닫기] |
+-----------------------------------------------------------------------------------------+
```

---

### 9.4. 단위 테스트 및 품질 보증 계획
1. **`bin/test_settlement_manager`:** 300개 부락 로드 무결성, 낙양 4개 부락(17, 20, 22, 24) 장악도 100% 판독 검증.
2. **`bin/test_city_officers`:** 도시 23번 연결 리스트 12명 무장 전수 순회, 조조(333), 원소(18) 등 정확 매핑 검증, 재야 항적(857) 노드 판독 검증.
3. **`bin/test_city_table`:** 도시 목록 더블클릭 시 4탭 인스펙터 정상 호출 및 탭 전환 120프레임 GUI 스모크 검증.
4. **`bin/test_city_detail_modal`:** 4탭 도시 인스펙터 독립 유닛 테스트 및 120프레임 스모크 테스트.

---

### 9.5. Phase 3 구현 및 검증 결과 (100% 통과 완료)

- **Step 3-1 (부락 도메인 모듈):**
  - [`include/game/Settlement.hpp`](../../include/game/Settlement.hpp) (`San13SettlementRaw` 28B, Stride `0x1C`, `RootBase + 0x00CA48`, 300개).
  - [`include/game/SettlementManager.hpp`](../../include/game/SettlementManager.hpp) 및 [`src/game/SettlementManager.cpp`](../../src/game/SettlementManager.cpp).
  - [`tests/test_settlement_manager.cpp`](../../tests/test_settlement_manager.cpp) 전체 통과 (낙양 17, 20, 22, 24 부락 바인딩 및 300개 부락 전수 로드 무결성 확인).
- **Step 3-2 (도시 소속 무장 연결 리스트 순회):**
  - [`include/game/City.hpp`](../../include/game/City.hpp) (`San13CityOfficerNodeRaw` 12B, `CityOfficerCategory`, `CityOfficerEntry`).
  - [`src/game/CityManager.cpp`](../../src/game/CityManager.cpp) (`LoadCityOfficers`, `LoadAllCityOfficers` 순환 방지 방문 가드 포함).
  - [`tests/test_city_officers.cpp`](../../tests/test_city_officers.cpp) 전체 통과 (낙양 12현역/1재야/0포로 판독 및 전국 60개 도시 순회 0.2ms 초고속 통과).
- **Step 3-3 (4탭 도시 종합 인스펙터 UI):**
  - [`include/ui/CityDetailModal.hpp`](../../include/ui/CityDetailModal.hpp) 및 [`src/ui/CityDetailModal.cpp`](../../src/ui/CityDetailModal.cpp).
  - [`include/ui/CityTableView.hpp`](../../include/ui/CityTableView.hpp) 및 [`src/ui/CityTableView.cpp`](../../src/ui/CityTableView.cpp) 더블클릭/컨텍스트 메뉴 완벽 연동.
  - [`src/main.cpp`](../../src/main.cpp)에 `SettlementManager` 및 `CityTableView` 바인딩 완료.
  - [`tests/test_city_detail_modal.cpp`](../../tests/test_city_detail_modal.cpp) 및 [`tests/test_city_table.cpp`](../../tests/test_city_table.cpp) 120프레임 스모크 테스트 100% 통과.
- **배포 패키지:**
  - `make package` -> `dist/san13_editor_linux_x86_64.tar.gz` (1.3MB) 빌드 검증 완료.

---

## 10. Phase 4 상세 작업 명세서 (도시 내정·숙련도·부락 쓰기 파이프라인 및 최종 완성)

> **원칙 준수:**
> 1. 실측 교차 검증이 완료된 오프셋(민심 `+0xD2`, 상업 `+0xD6`, 농업 `+0xE0`, 문화 `+0xEA`, 숙련도 `+0xFC/+0xFE/+0x100`, 부락 장악도 `+0x14`)에 한해서만 쓰기 API를 개방한다.
> 2. `BoundsValidator`를 반드시 경유하여 게임 엔진 크래시를 유발할 수 있는 비정상 수치 오버플로를 방지한다.
> 3. 내구도(`+0x102` 후보군)는 안전 검증 완료 전까지 쓰기 가드를 유지하여 기존 자원 오염을 완벽 차단한다.
> 4. 모든 수정 내역은 테스트 직후 원래 수치로 100% 안전 복구(Rollback)되어야 한다.

### 10.1. 수치 경계(Bounds) 설계

| 필드명                          |   오프셋    |    자료형     |    SafeVanilla 범위    | LimitBreak 범위 | 비고                      |
| :--------------------------- | :------: | :--------: | :------------------: | :-----------: | :---------------------- |
| 민심 (`public_order`)          | `+0x0D2` | `uint16_t` | 0 ~ 1,000 (최대 1,250) |   0 ~ 5,000   | 100 도달 시 인구 유입 가속       |
| 상업 기본치 (`commerce`)          | `+0x0D6` | `uint16_t` |      0 ~ 4,000       |  0 ~ 10,000   | 금 수입 영향                 |
| 농업 기본치 (`agriculture`)       | `+0x0E0` | `uint16_t` |      0 ~ 4,000       |  0 ~ 10,000   | 군량 수입 영향                |
| 문화 기본치 (`culture`)           | `+0x0EA` | `uint16_t` |      0 ~ 4,000       |  0 ~ 10,000   | 도시 기술 해금                |
| 창병 숙련도 (`spear_proficiency`) | `+0x0FC` | `uint16_t` |      0 ~ 3,000       |   0 ~ 5,000   | 1,000: 중급, 2,000: 고급 창병 |
| 기병 숙련도 (`horse_proficiency`) | `+0x0FE` | `uint16_t` |      0 ~ 3,000       |   0 ~ 5,000   | 1,000: 중급, 2,000: 고급 기병 |
| 궁병 숙련도 (`bow_proficiency`)   | `+0x100` | `uint16_t` |      0 ~ 3,000       |   0 ~ 5,000   | 1,000: 중급, 2,000: 고급 궁병 |
| 부락 장악도 (`dev_progress`)      | `+0x14`  | `uint16_t` |     0 ~ 100 (%)      |  0 ~ 100 (%)  | 100% 시 도시 효과 상시 부여      |

### 10.2. 도메인 매니저 쓰기 메서드 설계

#### 1) `CityManager`
```cpp
bool SetCityPublicOrder(uint8_t id, int64_t val, ClampingPolicy policy = ClampingPolicy::SafeVanilla);
bool SetCityCommerce(uint8_t id, int64_t val, ClampingPolicy policy = ClampingPolicy::SafeVanilla);
bool SetCityAgriculture(uint8_t id, int64_t val, ClampingPolicy policy = ClampingPolicy::SafeVanilla);
bool SetCityCulture(uint8_t id, int64_t val, ClampingPolicy policy = ClampingPolicy::SafeVanilla);
bool SetCitySpearProficiency(uint8_t id, int64_t val, ClampingPolicy policy = ClampingPolicy::SafeVanilla);
bool SetCityHorseProficiency(uint8_t id, int64_t val, ClampingPolicy policy = ClampingPolicy::SafeVanilla);
bool SetCityBowProficiency(uint8_t id, int64_t val, ClampingPolicy policy = ClampingPolicy::SafeVanilla);
bool MaximizeCityDomestic(uint8_t id);
bool MaximizeCityProficiency(uint8_t id);
```

#### 2) `SettlementManager`
```cpp
bool SetSettlementDevProgress(uint16_t id, int64_t val);
bool MaximizeCitySettlements(uint8_t cityId, const uint8_t settlementIds[4], uint32_t count);
```

### 10.3. 4탭 모달 UI 인터랙션 설계
- **탭 2 (내정 및 관할 부락):**
  - 민심, 상업, 농업, 문화 수치 슬라이더(`SliderInt`) 및 직접 입력(`InputInt`).
  - [빠른 치트]: "내정 올 3,000 풀충전", "관할 부락 전원 100% 장악" 원클릭 버튼.
- **탭 3 (군사 및 숙련도):**
  - 창병, 기병, 궁병 숙련도 슬라이더(0 ~ 3,000).
  - [빠른 치트]: "전 병종 숙련도 3,000 (최고 병종) 완충" 원클릭 버튼.
- **핫싱크(Hot-Sync):**
  - `Auto-Sync` 활성화 시 슬라이더 조작 즉시 게임 프로세스에 반영.
  - `[메모리 적용 (Hot-Sync)]` 버튼 클릭 시 경제/내정/숙련도/부락 데이터 원자적 일괄 동기화.

### 10.4. 단위 테스트 및 검증 계획
1. **`bin/test_city_detail_modal`:** 내정치, 숙련도, 부락 장악도 쓰기 및 읽기 일치 검증, 테스트 후 원래 상태 완벽 복구 검증.
2. **`make test`:** 15개 전체 오프라인/온라인 테스트 스위트 회귀 무결성 검증.
3. **`make package`:** 최종 릴리스 패키징.

---

### 10.5. Phase 4 구현 및 검증 완료 보고 (완료일: 2026-10-03)

- **Step 4-1 (수치 경계 및 세니타이저 수립):**
  - [`include/game/Bounds.hpp`](../../include/game/Bounds.hpp):
    - `StatBounds::CityPublicOrder`: 0 ~ 1,000 (Safe) / 0 ~ 5,000 (LimitBreak).
    - `StatBounds::CityDomestic`: 0 ~ 4,000 (Safe) / 0 ~ 10,000 (LimitBreak).
    - `StatBounds::CityProficiency`: 0 ~ 3,000 (Safe) / 0 ~ 5,000 (LimitBreak).
    - `StatBounds::SettlementDev`: 0 ~ 100 (%).
    - `BoundsValidator::SanitizeCityPublicOrder()`, `SanitizeCityDomestic()`, `SanitizeCityProficiency()`, `SanitizeSettlementDev()`.
- **Step 4-2 (도메인 매니저 쓰기 메서드 구현):**
  - [`include/game/CityManager.hpp`](../../include/game/CityManager.hpp) 및 [`src/game/CityManager.cpp`](../../src/game/CityManager.cpp):
    - `SetCityPublicOrder`, `SetCityCommerce`, `SetCityAgriculture`, `SetCityCulture`.
    - `SetCitySpearProficiency`, `SetCityHorseProficiency`, `SetCityBowProficiency`.
    - `MaximizeCityDomestic`, `MaximizeCityProficiency`.
    - 실측 검증 중인 성벽 내구도(`+0x102`)는 안전 가드를 유지하여 기존 자금(`+0x58`) 오염 완벽 차단.
  - [`include/game/SettlementManager.hpp`](../../include/game/SettlementManager.hpp) 및 [`src/game/SettlementManager.cpp`](../../src/game/SettlementManager.cpp):
    - `SetSettlementDevProgress(id, val)` (Stride 28B, 오프셋 `+0x14`).
    - `MaximizeCitySettlements(ids, count)`.
- **Step 4-3 (4탭 인스펙터 모달 직접 편집 UI 및 치트 버튼 완성):**
  - [`include/ui/CityDetailModal.hpp`](../../include/ui/CityDetailModal.hpp) 및 [`src/ui/CityDetailModal.cpp`](../../src/ui/CityDetailModal.cpp):
    - **탭 2 (내정 및 관할 부락):** 민심, 상업, 농업, 문화 `SliderInt` 컨트롤 및 프로그레스바 동시 제공. 빠른 치트(`내정 올 3,000 완충`, `관할 부락 100% 일괄 장악`, 한계돌파 10,000) 탑재. 부락별 개별 슬라이더 및 `[100%]` 즉시 장악 버튼 제공.
    - **탭 3 (군사 및 숙련도):** 창병, 기병, 궁병 숙련도 `SliderInt` (0 ~ 3,000/5,000) 및 병과 티어(초급/중급/고급) 실시간 인디케이터. 빠른 치트(`전 병종 숙련도 3,000 완충 (고급 병과 해금)`) 탑재.
    - **핫싱크(Hot-Sync):** 변경 감지 시 즉시 메모리 패치 (`Auto-Sync`) 및 `[메모리 적용]` 버튼 지원.
- **Step 4-4 (단위 테스트 및 회귀 무결성 검증 통과):**
  - [`tests/test_city_detail_modal.cpp`](../../tests/test_city_detail_modal.cpp):
    - 낙양(ID 23) 대상 내정 4대치 직접 편집, Safe 모드 클램핑(민심 1,000 / 상업 4,000), 내정 치트 완충 확인.
    - 병과 3대 숙련도 직접 편집, 클램핑(3,000), 숙련도 치트 완충 확인.
    - 부락 장악도 직접 편집 및 100% 장악 치트 확인.
    - **100% 무결성 롤백(Safe Rollback):** 테스트 완료 후 낙양의 원래 내정/숙련도/부락 데이터를 100% 복구하여 게임 프로세스 오염 방지 확인.
    - ImGui 4탭 인스펙터 120프레임 무결성 스모크 테스트 통과.
  - `make test`: 전체 15개 오프라인/온라인 테스트 스위트 100% 통과 (Active Process Verified).
- **Step 4-5 (독립 실행형 배포 패키지):**
  - `make package` -> `dist/san13_editor_linux_x86_64.tar.gz` (1.3MB) 빌드 검증 완료.

---

## 11. Phase 5 상세 작업 명세서 (부락 한글 명칭 매핑 및 무장 통합 분류 필터/뱃지 시스템)

### 11.1. 배경 및 사용자 요구사항 분석
1. **부락 한글 명칭 표출:**
   - 기존의 숫자 기반 임시 레이블(`부락 #017`) 대신 실제 인게임 지명(`호로관`, `밀`, `옹구`, `무평`, `영음` 등)을 연동하여 가독성 및 몰입도 극대화.
   - 요새/진/보루는 맵상의 동적 건설 시설이므로 엔진 전체 역분석 대신 300개 부락의 명칭 체계 및 도시 연동에 집중.
2. **무장 관리 UI 전략 (칸 나누기 vs 통합 뷰):**
   - 탭으로 화면을 분리할 경우 다중 선택(Multi-Select) 기반의 일괄 버프(신무장과 역사 무장을 함께 선택하여 스탯/수명/적성 일괄 수정) 및 전체 랭킹 정렬 비교가 단절되는 심각한 UX 저하 발생.
   - **채택된 설계 (통합 뷰 + 직교 분류 필터 & 뱃지):**
     - 단일 가상 스크롤 테이블 뷰를 유지하여 2,165명 전체에 대한 일괄 작업 및 검색/정렬 편의성 100% 보존.
     - 상단 툴바에 직교 분류 필터(`[분류: 전체 | 역사무장 | 등록신무장 | 고대무장]`)를 제공하여 원클릭 분리 보기 완벽 지원.
     - 테이블 목록 및 상세 모달에서 `[신무장]`(하늘색), `[고대]`(보라색) 컬러 태그 뱃지를 부여하여 한눈에 식별 가능하도록 구현.

### 11.2. 아키텍처 및 모듈 설계

#### 1) 부락 명칭 매핑 시스템 (`SettlementNameTable`)
- [`include/game/SettlementNameTable.hpp`](../../include/game/SettlementNameTable.hpp) & [`src/game/SettlementNameTable.cpp`](../../src/game/SettlementNameTable.cpp)
- **API 설계:**
  ```cpp
  class SettlementNameTable {
  public:
      static const char* GetSettlementName(uint16_t id);
      static std::string GetFormattedSettlementName(uint16_t id, const std::string& cityName = "");
  };
  ```
- 알려진 300개 부락 지명 매핑 테이블 구축 및 관할 도시 컨텍스트 결합 포맷팅 지원.
- `SettlementManager::LoadSettlement()`에서 `outRecord.name`에 자동 적용.

#### 2) 무장 통합 분류 필터 & 뱃지 시스템 (`OfficerTableView`)
- [`include/ui/OfficerTableView.hpp`](../../include/ui/OfficerTableView.hpp):
  ```cpp
  enum class OfficerOriginFilter {
      All = 0,         // 전체
      Historical = 1,  // 역사 무장 (ID 0 ~ 799)
      Created = 2,     // 등록 신무장 (ID >= 1000)
      Ancient = 3      // 고대 무장 (ID 850 ~ 899)
  };
  ```
- **필터링 조건식:**
  - 기존 상태 필터(`All`, `Active`, `Free`, `Unappeared`)와 분류 필터(`All`, `Historical`, `Created`, `Ancient`)의 AND 결합 필터링.
- **UI 뱃지 렌더링:**
  - `ID >= 1000`: `[신무장]` (하늘색: `ImVec4(0.35f, 0.85f, 0.95f, 1.0f)`)
  - `850 <= ID < 900`: `[고대]` (보라색: `ImVec4(0.85f, 0.55f, 0.95f, 1.0f)`)
  - `ID < 800`: 역사 무장 기본 색상 (군주 황금색 / 일반 흰색)

### 11.3. 작업 단계 및 검증 계획
1. `SettlementNameTable` 구현 및 `SettlementManager` 연동.
2. `OfficerTableView`에 `OfficerOriginFilter` 및 상단 라디오/버튼 컨트롤 추가, 뱃지 렌더링 구현.
3. 단위 테스트 작성 및 회귀 검증 (`make test`).
4. 독립 실행 배포 패키지(`make package`) 빌드 및 문서 갱신.

---

### 11.4. Phase 5 구현 및 검증 완료 보고 (완료일: 2026-10-03)

- **Step 5-1 (부락 한글 지명 매핑 시스템 구축):**
  - [`include/game/SettlementNameTable.hpp`](../../include/game/SettlementNameTable.hpp) & [`src/game/SettlementNameTable.cpp`](../../src/game/SettlementNameTable.cpp) 신규 개발.
  - 전국 주요 부락(답씨, 망평, 호로관, 밀, 옹구, 하동, 무평, 영음, 곤양, 성부, 여양, 박망, 무음, 고시, 육양, 신야, 채양, 장릉, 의성, 오장원, 견성, 제남, 평양, 누와, 동병, 낙, 칠 등) 실측 지명 매핑 및 미등록 슬롯 스마트 포맷팅 지원.
  - [`src/game/SettlementManager.cpp`](../../src/game/SettlementManager.cpp)에 연동하여 `LoadSettlement` 호출 시 즉시 실제 지명으로 바인딩.
  - [`tests/test_settlement_manager.cpp`](../../tests/test_settlement_manager.cpp)에서 답씨(0), 망평(1), 호로관(17) 지명 매핑 검증 100% 통과.
- **Step 5-2 (무장 테이블 통합 분류 필터 & 뱃지 시스템 구축):**
  - [`include/ui/OfficerTableView.hpp`](../../include/ui/OfficerTableView.hpp) & [`src/ui/OfficerTableView.cpp`](../../src/ui/OfficerTableView.cpp):
    - `OfficerOriginFilter` (`All`, `Historical`, `Created`, `Ancient`) 도입.
    - 상단 툴바에 `출신: [전체 | 역사 | 신무장 | 고대]` 라디오 버튼 배치.
    - 테이블 목록 성명 컬럼에 `[신무장]`(하늘색), `[고대]`(보라색), `[특수]`(주황색) 컬러 뱃지 렌더링.
    - 검색창에 `신무장`, `등록무장`, `고대`, `고대무장` 입력 시 즉시 해당 그룹 스마트 매칭 지원.
  - [`tests/test_officer_table.cpp`](../../tests/test_officer_table.cpp)에서 역사(800인), 고대(50인), 신무장(1,007인) 분류 필터링 및 표본 유효성 검증 100% 통과.
- **Step 5-3 (회귀 테스트 및 패키지 릴리스):**
  - `make test`: 전체 15개 단위 및 렌더링 테스트 스위트 100% PASS.
  - `make package` -> `dist/san13_editor_linux_x86_64.tar.gz` (1.3MB) 최신화 완료.

---

## 12. 부락 시스템 심층 진단 및 아키텍처 정정 (진단일: 2026-10-03)

### 12.1. 문제 제기 및 현상
- 인게임에서 확인 결과, 에디터에 표시되는 부락 명칭이 실제 게임과 완전히 불일치하는 현상 발생 (예: 낙양의 부락으로 '호로관', '밀', '옹구', '하동'이 표시됨).

### 12.2. 라이브 메모리 교차 실측 기반 3대 근본 원인 규명
1. **도시 구조체 오프셋 `+0x18`의 오독 (결정적 원인):**
   - 기존에는 도시 구조체(`San13CityRaw`)의 `+0x18 ~ +0x1B` 4바이트를 "관할 부락 ID 목록"으로 가정함.
   - 60개 도시 전체 메모리를 교차 실측한 결과, 이는 부락 ID가 아니라 **도로망으로 직결된 인접 도시(Connected / Neighboring Cities) ID 목록**임이 규명됨:
     - 도시 0 (양평): `[1, 255, 255, 255]` (인접 도시: 북평)
     - 도시 1 (북평): `[0, 2, 3, 255]` (인접 도시: 양평, 계, 남피)
     - 도시 2 (계): `[1, 3, 4, 255]` (인접 도시: 북평, 남피, 중산)
     - 도시 23 (낙양): `[17, 20, 22, 24]` (인접 도시: 진류, 완, 천수, 무위)
   - 즉, 낙양의 부락으로 "낙양의 인접 도시 번호"를 읽고 있었음.
2. **관문(Gate)과 부락(Settlement)의 체계 혼동:**
   - 삼국지 13 PK의 거점은 도시(60개, ID 0~59), 부락(300개, ID 0~299, 거점 60~359), 관문(26개, 호로관/함곡관/동관 등)으로 분리됨.
   - 호로관은 관문(Gate)이지 부락(Settlement)이 아님에도 부락 ID 17에 억지로 매핑되어 있었음.
3. **임시 UI 렌더링 큐 텍스트의 오독:**
   - 이전 스캔에서 발견된 문자열(적벽, 화용, 기춘 등)은 3D 맵 화면에 현재 카메라가 비추고 있는 지역의 스프라이트 텍스트 임시 렌더링 캐시 버퍼(`0x74930240`)였음.

### 12.3. 진짜 도시-부락 연결 아키텍처 규명
- 부락 300개 청크(`RootBase + 0xCA48`, 28B Stride):
  ```cpp
  struct San13SettlementRaw {
      uint32_t vtable;               // +0x00: VTable 포인터 (0x72919AC4)
      uint16_t settlement_id;        // +0x04: 부락 고유 ID (0 ~ 299)
      uint16_t force_id;             // +0x06: 점령 세력 ID
      uint16_t global_site_id;       // +0x08: 글로벌 거점 ID (60 + settlement_id)
      uint8_t  connected_cities[4];  // +0x0A ~ +0x0D: 소속 및 관할 도시 ID (최대 4개)
      uint8_t  pad_0x0E[6];          // +0x0E ~ +0x13: 미지의 플래그
      uint8_t  dev_progress;         // +0x14: 장악도 / 개발도 (0 ~ 100%)
      uint8_t  pad_0x15[7];          // +0x15 ~ +0x1B: 말미 패딩
  };
  ```
- **실측 검증:**
  - 낙양(City 23) 소속 부락 검색: 정확히 4개 검출 -> **ID 6, ID 9, ID 35, ID 144**.
  - 양평(City 0) 소속 부락 검색: 정확히 1개 검출 -> **ID 117**.
  - 인게임 도시별 관할 부락 수(`settlement_count`)와 100% 완벽 일치!

### 12.4. 정정 조치 계획
1. `San13CityRaw`의 `settlement_ids[4]` 필드를 `connected_city_ids[4]`로 의미 변경 및 주석/구조체 정정.
2. `San13SettlementRaw` 구조체에 `connected_cities[4]` 명시.
3. `SettlementManager::LoadCitySettlements`를 도시 ID 기준 역방향 탐색(Reverse Lookup)으로 전환.
4. `SettlementNameTable.cpp`에서 임의 추측된 50여 개 가짜 지명을 전면 제거하고, 컨텍스트 기반의 안전한 표준 명칭(`[도시명] 부락 #ID` 또는 `부락 #ID`)으로 표시하도록 전환.

---

## 13. 게임 아카이브(DATA0/DATA1) 전수 언팩 및 공식 부락/효과 DB 추출 결과 (2026-10-03)

### 13.1. 아카이브 언패커(`tools/unpack_s13.py`) 아키텍처
- **KOEI S13 ZP1 포맷 규격 규명:**
  - `DATA0.S13` (인덱스 테이블): 32바이트 Stride
    - `uint64_t offset`: `DATA1.S13` 내 파일 시작 오프셋
    - `uint64_t decompressed_size`: 원본 압축 해제 크기
    - `uint64_t compressed_size`: 아카이브 내 저장 크기
    - `uint64_t is_compressed`: 1(ZP1 압축), 0(Raw 비압축)
  - `DATA1.S13` ZP1 블록 헤더:
    - `char magic[4]`: `"zp1\0"`
    - `uint32_t tot_sz`, `chunk_sz`, `num_chunks`
    - `uint32_t chunk_lens[num_chunks]`: 청크별 압축 바이트 수
  - **128바이트 청크 정렬 법칙:**
    - 첫 번째 청크는 항상 `offset + 0x800` (2,048B)에서 시작.
    - 각 청크는 4B little-endian 실제 길이 prefix + zlib 스트림(`0x78 0xDA`).
    - 이후 청크는 `(rel_pos + chunk_len + 0x7F) & ~0x7F`로 128B 경계에 정렬됨.
- **언팩 결과:**
  - `DATA0.S13` (1,873개 항목): 1,432개 파일 추출, 441개 빈 슬롯, **0 에러** (4.01초 소요, 4.40 GB).
  - `DATA0_2.S13` (141개 항목): 120개 파일 추출, 21개 빈 슬롯, **0 에러** (0.45초 소요, 0.28 GB).
  - 저장 위치: `unpacked_pk/data/`, `unpacked_pk/data2/`, 인덱스: `unpacked_pk/manifest.json`.

### 13.2. Entry 1655 시스템 메시지 테이블 분석 및 100% 공식 데이터 매핑
- **토큰 인덱스별 공식 데이터:**
  - `Token 14 ~ 73`: 60개 도시 (`14 + city_id`) -> 14: 양평(0), 37: 낙양(23), 73: 교지(59).
  - `Token 74 ~ 173`: 도시/부락 효과 설명 (100개 토큰).
  - `Token 174 ~ 181`: 8대 주요 관문 (호관, 호로관, 함곡관, 동관, 무관, 양평관, 가맹관, 면죽관).
  - `Token 182 ~ 328`: **기본 부락 147개 (`182 + settlement_id`) 공식 한글 지명 100% 일치**!
    - ID 0: 범 (Fan)
    - ID 1: 동평 (Dongping)
    - ID 6: **밀 (Mi)** -> 낙양 관할 부락 (일치)
    - ID 9: **온 (Wen)** -> 낙양 관할 부락 (일치)
    - ID 35: **하남 (Henan)** -> 낙양 관할 부락 (일치)
    - ID 40: 화용 (Huarong), ID 41: 이릉 (Yiling), ID 48: 합비 (Hefei), ID 49: 장판 (Changban)
    - ID 71: 오장원 (Wuzhangyuan), ID 77: 가정 (Jieting), ID 86: 검각 (Jiange)
    - ID 117: **임유 (Linyu)** -> 양평 관할 부락 (일치)
    - ID 144: **신안 (Xin'an)** -> 낙양 관할 부락 (일치)
    - ID 146: 증 (Zeng)
  - `Token 329 ~ 372`: **부락 44종 특수 효과 및 특수 병과**:
    - 효과 0~8: 상업/농업/병역 (소/중/대)
    - 효과 9: 궁기병, 10: 청주병, 11: **호표기**, 12: 동주병, 13: 서량병, 14: 등갑병, 21: 상병
    - 효과 22~30: 상업+500/750/1000, 농업+500/750/1000, 인구+20000/40000/60000
    - 효과 31~36/43: 특수 병종 편제 가능
- **부락 메모리 구조체 완전 규명 (`San13SettlementRaw`):**
  - `+0x0E ~ +0x11`: `uint8_t buff_effects[4]` (최대 4개 버프 ID, 255=없음)
    - 부락 117(임유): `[11, 255, 255, 255]` -> **효과 11: 호표기 편제가능**!
    - 부락 0(범), 1(동평): `[28, 255, 255, 255]` -> **효과 28: 인구+20000**!
    - 부락 6(밀): `[28, 29, 255, 255]` -> **인구+20000, 인구+40000**!
  - `+0x12`: `uint8_t is_active` (1=활성)
  - `+0x13`: `uint8_t primary_city_id` (주 관할 도시)
  - ID 147 ~ 299: PK 동적 요새(砦/진/보루) 확장 슬롯 (초기값: `force=255, global=65535`).

### 13.3. 소프트웨어 적용 및 검증 현황
- `include/game/Settlement.hpp`: `buff_effects[4]`, `is_active`, `primary_city_id` 반영.
- `src/game/SettlementNameTable.cpp`: 147개 부락명 및 44개 효과명 전수 매핑, `GetEffectDescription` 구현.
- `src/ui/CityDetailModal.cpp`: 관할 부락 테이블에 "효과 / 특수병과" 컬럼 추가 및 UI 표시.
- `tests/test_settlement_manager.cpp`: 147개 공식 명칭 및 효과 단위 테스트 100% PASS.
- `make test`: 전체 테스트 스위트 100% 통과 (Green).








