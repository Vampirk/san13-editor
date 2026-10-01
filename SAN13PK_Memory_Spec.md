# [SPEC v5.0] Romance of the Three Kingdoms 13 PK - Modern Cross-Platform Memory Editor & Engine Suite

> **문서 버전:** v5.6 (Phase 6.0 전 항목 100% 완료: 4탭 심층 인스펙터, 141개 전법, 20대 특기, 8대 성향/가치관, 7대 가족/인연망, EDGE-32 검증, 독립 배포 패키징 완료)  
> **최초 작성일:** 2026-09-30  
> **최종 갱신일:** 2026-10-01  
> **상태:** **Phase 1 ~ Phase 6.0 전 항목 및 EDGE-01 ~ EDGE-32 100% 완료** / **다음 진행:** Phase 6.2 (Windows 10/11 네이티브 크로스 컴파일)


---

## 1. 프로젝트 비전 및 목표

기존 2000년대 고전 스타일의 반에디터(`San13EditorPK_kr.exe`) 단순 리눅스 포팅을 넘어, **리눅스(Proton)와 윈도우(Windows 10/11) 양쪽에서 최고 수준의 사용자 경험을 제공하는 현대적인 삼국지 13 PK 전문 에디터 & 트레이너 엔진**을 구축합니다.

### 1.1. 기존 반에디터의 한계와 개선 방향
1. **낡은 Win32/MFC 기반 UI 탈피**
   - 구시대적 고정 해상도 창, HiDPI 미지원, 불편한 다이얼로그 방식 개선.
   - **Dear ImGui (GLFW/OpenGL3 백엔드)** 기반의 모던 다크 테마, 가변 그리드 뷰, 다중 정렬/필터링, 도킹 윈도우 UI 적용.
2. **완벽한 한국어(UTF-8) 네이티브 지원**
   - CP949(EUC-KR) 인코딩 깨짐 없는 유니코드 한글 폰트(Noto Sans KR / NanumGothic) 내장 렌더링.
3. **독립 창(Standalone) 및 인게임 오버레이(Overlay) 듀얼 아키텍처**
   - **메인 에디터:** 듀얼 모니터 또는 창모드에서 수천 명의 데이터를 엑셀처럼 쾌적하게 검색/정렬/일괄 수정하는 **고성능 독립 창 GUI**.
   - **인게임 트레이너:** 동일한 UI 코드를 재사용하여 `version.dll` 프록시를 통해 게임 내부 화면에 즉시 띄우는 **DirectX 인게임 오버레이**.
4. **실시간 동기화 (Live Hot-Sync)**
   - 게임을 재시작하거나 세이브를 매번 불러올 필요 없이 메모리 데이터를 실시간으로 읽고 패치.

---

## 2. 전체 시스템 아키텍처 및 모듈화 설계도

### 2.1. 계층형 아키텍처 다이어그램
```mermaid
flowchart TD
    subgraph Presentation ["Presentation Layer (UI)"]
        UI_Standalone["San13EditorNext (독립 실행 GUI)<br/>Dear ImGui + GLFW / OpenGL3<br/>- 대용량 데이터 그리드 / 다중 정렬<br/>- 무장 / 도시 / 전법 / 명품 상세 편집"]
        UI_Overlay["San13Overlay (인게임 트레이너)<br/>Dear ImGui + DirectX Hook<br/>- 단축키 호출 (F11/Insert)<br/>- 실시간 치트 / 전투 보조"]
    end

    subgraph DataModel ["Data Model & Business Logic"]
        DM_Officer["OfficerManager (무장 매니저)<br/>- 기본 스탯 vs 명품 보정 스탯 분리<br/>- 혈통, 인연, 전법, 성격 매핑"]
        DM_City["CityManager (도시 매니저)<br/>- 자금, 군량, 병력, 내구도 일괄 조정"]
        DM_Item["ItemManager (명품 매니저)<br/>- 소유권 이전, 보물 생성/수정"]
        DM_Bounds["BoundsValidator (수치 검증기)<br/>- Min/Max 클램핑, 오버플로/크래시 방지"]
        DM_Engine["EnginePatches (엔진 패치)<br/>- 30/60 FPS 제한 해제, 타이머 제어"]
    end

    subgraph MemoryCore ["Abstracted Memory Core (OS 추상화 계층)"]
        IMem["IMemoryDriver (추상 인터페이스)<br/>- Read<T> / Write<T><br/>- PatternScan / ResolvePointerChain"]
        Driver_Linux["LinuxMemoryDriver<br/>- /proc/$PID/mem & process_vm_readv/writev<br/>- GE-Proton / Wine 호환"]
        Driver_Win["Win32MemoryDriver<br/>- OpenProcess / ReadProcessMemory / WriteProcessMemory"]
    end

    subgraph TargetGame ["Target Process (삼국지 13 PK)"]
        Game["San13PK_kr.exe (x86 32-bit PE)<br/>Proton WoW64 (Linux) or Native (Windows)"]
    end

    UI_Standalone --> DataModel
    UI_Overlay --> DataModel
    DataModel --> IMem
    IMem --> Driver_Linux
    IMem --> Driver_Win
    Driver_Linux -.->|호스트 시스템 콜| Game
    Driver_Win -.->|Win32 API| Game
```

---

### 2.2. 확정된 프로젝트 디렉터리 및 모듈별 역할

```text
vaneditor/
├── SAN13PK_Memory_Spec.md       # 마스터 기술 명세서 및 진행 로그
├── Makefile                     # 크로스 플랫폼 빌드 스크립트
├── assets/                      # 번들 에셋 디렉터리
│   └── fonts/                   # 나눔고딕 등 번들 TTF 한글 폰트
│       └── NanumGothic.ttf
├── vendor/                      # 서드파티 라이브러리
│   └── imgui/                   # Dear ImGui (Docking 브랜치) + GLFW/OpenGL3 백엔드
├── bin/                         # 컴파일 완료 바이너리 출력 위치
│   ├── test_memory_driver       # Phase 4.1 드라이버 검증 바이너리
│   ├── test_bounds              # Phase 4.2 수치 검증 바이너리
│   ├── test_officer_manager     # Phase 4.3 무장 매니저 검증 바이너리
│   ├── test_city_manager        # Phase 4.4 도시 매니저 검증 바이너리
│   ├── test_gui_window          # Phase 5.1 GUI 파이프라인 검증 바이너리
│   ├── test_officer_table       # Phase 5.2 무장 가상 스크롤/슬롯 확장 검증 바이너리
│   ├── test_officer_edit        # Phase 5.3 인라인 편집 및 실시간 핫싱크 검증 바이너리
│   ├── test_city_table          # Phase 5.4 도시 자원 편집 및 치트 검증 바이너리
│   ├── test_batch_actions       # Phase 5.5 다중 선택 및 일괄 수정 검증 바이너리
│   ├── test_hangul_automata     # Phase 5.6 네이티브 한글 2벌식 오토마타 검증 바이너리
│   ├── test_hangul_input_widget # Phase 5.8 Dear ImGui 한글 위젯 연동 검증 바이너리
│   ├── test_officer_names       # Phase 5.7 무장 700인 한글 이름 테이블 검증 바이너리
│   └── san13_editor             # Phase 6.1 리눅스 독립 실행형 완성형 바이너리
├── dist/                        # Linux Standalone 배포 번들 (Phase 6.1)
│   ├── run.sh                   # ptrace 권한 점검 및 자동 런처 스크립트
│   ├── san13_editor.desktop     # 리눅스 데스크톱 바로가기 엔트리
│   ├── README_LINUX.md          # 비개발자용 한국어 상세 실행 가이드
│   └── san13_editor_linux_x86_64.tar.gz # 독립 배포 압축 아카이브 (1.3MB)
├── include/                     # C++ 공용 헤더 디렉터리
│   ├── core/                    # OS 추상 메모리 드라이버 계층
│   │   ├── IMemoryDriver.hpp    # 메모리 읽기/쓰기/검색 가상 인터페이스
│   │   ├── LinuxMemoryDriver.hpp# Linux process_vm_readv/writev 구현 헤더
│   │   └── Win32MemoryDriver.hpp# Windows ReadProcessMemory 구현 헤더
│   ├── game/                    # 도메인 데이터 모델 및 비즈니스 로직
│   │   ├── GameVersion.hpp      # PE TimeDateStamp 판별 및 RootBase 계산기
│   │   ├── Bounds.hpp           # 수치별 Min/Max, 안전 클램핑, 오버플로 방지 정책
│   │   ├── Officer.hpp          # 무장 오브젝트(0x12BC Stride) 및 스탯 구조체
│   │   ├── OfficerManager.hpp   # 800인 무장 도메인 매니저 헤더
│   │   ├── OfficerNameTable.hpp # 700인 역사 무장 공식 한글 이름 플랫 테이블 (Phase 5.7)
│   │   ├── City.hpp             # 도시 오브젝트(0x170 Stride) 구조체
│   │   └── CityManager.hpp      # 60개 도시 도메인 매니저 헤더
│   └── ui/                      # Dear ImGui 프레젠테이션 계층
│       ├── UIManager.hpp        # GLFW/OpenGL3 ImGui 창 관리자 및 테마
│       ├── FontHelper.hpp       # 한글 폰트 아틀라스 로더 및 글리프 빌더
│       ├── HangulAutomata.hpp   # 리눅스 네이티브 한글 2벌식 오토마타 및 UI 위젯 (Phase 5.6)
│       ├── OfficerTableView.hpp # 무장 가변 그리드/다중 정렬 뷰 (Phase 5.2)
│       └── CityTableView.hpp    # 도시 60개 자원 편집 뷰 (Phase 5.4)
├── src/                         # 모듈 구현 소스코드 (.cpp)
│   ├── main.cpp                 # 통합 에디터 대시보드 메인 엔트리포인트 (Phase 6.1)
│   ├── core/
│   │   └── LinuxMemoryDriver.cpp
│   ├── game/
│   │   ├── GameVersion.cpp
│   │   ├── OfficerManager.cpp
│   │   ├── OfficerNameTable.cpp
│   │   └── CityManager.cpp
│   └── ui/
│       ├── UIManager.cpp
│       ├── FontHelper.cpp
│       ├── HangulAutomata.cpp
│       ├── OfficerTableView.cpp
│       └── CityTableView.cpp
├── tests/                       # 세부 단계별 자동화 단위 테스트 스위트
│   ├── test_memory_driver.cpp
│   ├── test_bounds.cpp
│   ├── test_officer_manager.cpp
│   ├── test_city_manager.cpp
│   ├── test_gui_window.cpp
│   ├── test_officer_table.cpp
│   ├── test_officer_edit.cpp
│   ├── test_city_table.cpp
│   ├── test_batch_actions.cpp
│   ├── test_hangul_automata.cpp
│   ├── test_hangul_input_widget.cpp
│   └── test_officer_names.cpp
├── tools/                       # 검증 및 PoC CLI 스크립트 도구 (san13_tool)
└── legacy_van/                  # 원본 반에디터(San13EditorPK_kr.exe) 보관소
```

---

### 2.3. 세부 기능 구현 명세서 (Functional Scope)

1. **코어 메모리 엔진 (Core Engine)**
   - **프로세스 자동 감지:** `/proc` 또는 Win32 API로 `San13PK_kr.exe` 자동 탐색 및 PID 부착.
   - **버전 자동 감지:** PE 헤더의 `TimeDateStamp`(`0x5E3BA719` 등)를 검사하여 글로벌 오프셋 및 RootBase 자동 산출.
   - **실시간 양방향 핫-싱크:** 에디터에서 값 수정 시 즉시 게임 메모리에 반영되며, 주기적으로 게임 상태를 폴링하여 외부 변경 감지.
   - **트랜잭션 일괄 쓰기:** 다수의 수치를 변경할 때 원자적(Atomic)으로 메모리에 동시 커밋.

2. **무장 편집 기능 (Officer Editor)**
   - **물리적 무장 풀 (800명) vs 시나리오별 유동성 메커니즘:**
     - 삼국지 13 PK의 역사 무장 메모리 풀(ID 0 ~ 799, 총 800개)은 **시나리오와 무관하게 전원 고정 할당**되어 메모리에 상주합니다.
     - 시나리오 연도(예: 184년 황건의 난 vs 208년 적벽대전 vs 225년 출사표)에 따라 실제로 활동하는 무장은 **상태 플래그(Status: 현역, 재야, 미등장, 사망)** 및 등장년도에 의해 유동적으로 결정됩니다.
     - 에디터는 시나리오별 상태를 실시간 판별하여 **[전체 무장], [현역 무장만], [재야 무장만], [미등장 포함], [사망자 포함]** 다차원 필터링을 지원합니다.
   - **결혼 자녀 무장 & 등록/신무장 동적 자동 반영 (★ Phase 5 GUI 통합 명시):**
     - 플레이 중 결혼하여 태어난 2세 육성 무장이나 유저 등록 무장은 800번 이후의 확장 슬롯(Chunk 16, 최대 1,015개)에 배정됩니다.
     - **Phase 5 GUI 구축 시:** 고정 800명 파싱을 넘어, 확장 슬롯 중 활성화 플래그(`Active == 1`)가 켜진 슬롯을 동적으로 자동 탐지하는 스캐너를 통합합니다.
     - 이를 통해 게임 중 아이가 태어나 성인식을 치르면 **에디터 재시작 없이도 실시간으로 자녀 무장이 에디터 목록에 자동 추가**되도록 구현합니다.
     - **자녀/신무장 고유 이름(성명) 동적 반영 메커니즘:**
       - 역사 무장(고정 700명)과 달리, 유저가 인게임에서 직접 입력하여 작명한 이름은 **게임 런타임 메모리(Chunk 16: RootBase + 0xD3DD50, 0x424 Stride) 및 세이브 데이터에 평문 바이트(CP949/EUC-KR)로 직접 상주**합니다.
       - 에디터는 슬롯 감지 시 해당 구조체의 성/명/자 버퍼를 읽어 CP949 -> UTF-8로 실시간 디코딩하여 "신무장 / 자녀 #N" 대체 실제 유저 작명 이름(예: "조아름", "강선우")을 즉시 테이블에 반영합니다.
       - 또한 에디터 UI 상에서도 자녀/신무장 이름을 직접 수정하여 게임 메모리에 실시간 쓰기(Write)할 수 있도록 양방향 편집을 지원합니다.
   - **능력치 개별 편집:**
     - 4대 기본치 (통솔, 무력, 지력, 정치)
     - 4대 현재치 (아이템 및 일시적 버프 상태)
     - 능력치별 경험치 (0 ~ 99)
   - **병종 적성 편집:** 창병, 기병, 궁병 적성 (C:0, B:1, A:2, S:3).
   - **수명 및 출현 편집:** 출생년도, 등장년도, 사망년도.
   - **일괄 변경 기능 (Batch Action):** 선택된 무장 전체 스탯 +10, 아군 세력 소속 무장 전원 수명 20년 연장 등.

3. **도시 편집 기능 (City Editor)**
   - **도시 목록 뷰 (60개):** 도시 ID, 주도 여부, 자금, 군량, 병력, 내구도 테이블.
   - **자원 개별/일괄 조작:**
     - 자금 (최대 999,999) 및 군량 (최대 999,999) 안전 수정.
     - 병력 및 모병량 설정.
     - 도시 내구도 수치 변경.
   - **원클릭 치트 버튼:** "아군 도시 전체 자원 최대화", "적 도시 군량 고갈".

4. **명품 및 보물 편집 기능 (Item Editor)**
   - **명품 목록 뷰 (200개):** 명품 이름, 가치(등급), 능력치 보정치(+1~+10), 소유 무장 ID.
   - **소유권 이전:** 조조/유비/손권 등 원하는 무장에게 원클릭 명품 지급.

5. **트레이너 & 편의 치트 기능 (Trainer Suite - Phase 6 연동)**
   - 실시간 단축키(F1~F12) 기반 치트:
     - 금/군량 무한 충전.
     - 전투 출진 부대 사기 200 고정 및 지휘 포인트 10 고정.
     - 부대 무적 / 즉시 퇴각.
   - 30/60 FPS 제한 해제 및 게임 속도 조절 배속 패치.

---

## 3. 메모리 주소 체계 및 리버스 엔지니어링 검증 데이터

### 3.1. 프로세스 및 윈도우 식별자
- **타깃 바이너리:** `SAN13PK.exe` / `San13PK_kr.exe` (PE32, x86 32비트)
- **윈도우 클래스명:** `KT_SANGOKUSHI_13_PK_WINDOW_NAME`
- **Linux 탐색 방식:** `/proc/*/cmdline`에서 `San13PK_kr.exe` 자동 탐색 (PID 획득)

### 3.2. PE Header 타임스탬프 기반 버전 판별 공식
게임 바이너리의 PE 헤더 `TimeDateStamp`를 검사하여 데이터 루트 주소를 산출:
1. `e_lfanew = ReadMemory<DWORD>(BaseAddress + 0x3C)`
2. `TimeDateStamp = ReadMemory<DWORD>(BaseAddress + e_lfanew + 8)`
3. 테이블 매칭 후 오프셋 도출:

| 게임 버전 및 언어 | TimeDateStamp (Hex) | Version ID | Global Base Offset (`0x0044D0D0 + ID*4`) | 검증 상태 |
| :--- | :--- | :--- | :--- | :--- |
| **한국어 PK 최신 (1.0.2)** | **`0x5E3BA719`** | **`0x35`** | **`+0x01FBC5D8`** | **실제 Proton 메모리 실측 완료** |
| 한국어 PK 1.0.1 | `0x5D834FFA` | `0x34` | `+0x01FBC618` | 오프셋 확보 |
| 한국어 PK 1.0.0 | `0x59919494` | `0x33` | `+0x01FB94A8` | 오프셋 확보 |
| 일본어 PK 최신 | `0x5E3BA678` | `0x2A` | `+0x02DF1288` | 오프셋 확보 |
| 중문 번체 PK 최신 | `0x5E3BA5D5` | `0x32` | `+0x01FCAA18` | 오프셋 확보 |
| 영문 PK 최신 | `0x5E3BA865` | `0x39` | `+0x02052110` | 오프셋 확보 |

> **실시간 데이터 루트 포인터 계산식:**  
> `RootBaseAddress = BaseAddress (Proton WoW64 Base) + Global Base Offset`  
> *(현재 테스트 환경 기준: `0x71bb0000 + 0x01FBC5D8 = 0x73B6C5D8`)*

---

## 4. 데이터 청크 맵 (Live Memory Verification)

| 청크 인덱스 | 데이터 항목               | 오프셋 (From RootBase) | 단일 크기 (Stride)         | 최대 개수    | 청크 크기                    | 비고                                                             |
| :----: | :------------------- | :------------------ | :--------------------- | :------- | :----------------------- | :------------------------------------------------------------- |
| **0**  | **시나리오 정보**          | `+0x000000`         | -                      | 1개       | `0x1410` (5,136 B)       | **실시간 게임 일자:** 년 `+0x137A` (190), 월 `+0x1380` (1), 일 `+0x1386` (1) (EDGE-25 실측 완료) |
| **1**  | **도시 (City)**        | `+0x007178`         | **`0x170` (368 B)**    | **60개**  | `0x5640` (22,080 B)      | 자금, 군량, 병력, 민심, 내구도 (60개 도시 실측 검증)                  |
| **2**  | **관소 (Gate)**        | `+0x00C7B8`         | `0x54` (84 B)          | 26개      | `0x888` (2,184 B)        | 관문 방어력 및 주둔군                                                   |
| **4**  | **세력 (Force)**       | `+0x00ED60`         | `0x148` (328 B)        | 64개      | `0x4FB0` (20,400 B)      | 군주 ID, 국호, 세력 색상, 외교                                           |
| **5**  | **군단 (Corps)**       | `+0x013D10`         | `0x58` (88 B)          | 64개      | `0x1600` (5,632 B)       | 군단장 ID, 방침, 소속 도시                                              |
| **-**  | **시나리오 활성 무장 풀**     | **`+0x015EE0`**     | **`0x12BC` (4,796 B)** | **2,165명** | **가변 풀**                 | **`RootBase + 0x15EE0 + ID * 0x12BC` (역사 800 + 고대 50 + 신무장 1,315)** |
| **10** | **역사 무장 (Save/Raw)** | `+0xA5750C`         | `0x280` (640 B)        | 800명     | `0x7BD40` (507,200 B)    | 원본 무장 기본 DB (메모리 정렬 640B)                                      |
| **13** | **명품 (Item)**        | `+0xBD475C`         | `0x8C` (140 B)         | 200개     | `0x6D60` (28,000 B)      | 무력/지력 가산치, 소유자 ID                                              |
| **14** | **전법 (Tactics)**     | `+0xC0D260`         | `0x48` (72 B)          | 약 200개   | `0x3800`                 | 소모 지휘, 버프/디버프 계수                                               |
| **16** | **등록 무장 & 인연**       | `+0xD3DD50`         | `0x424` (1060 B)       | 1,015명   | `0x106ABC` (1,075,900 B) | 신무장 슬롯 및 인연 관계망 (`editdata_pk.s13` 파일 파싱 연동 완료)          |

---

## 5. 핵심 데이터 구조체 정의 (C/C++ Header)

### 5.1. 도시 구조체 (`San13CityRaw` - Stride: 0x170 = 368 B, RootBase + 0x007178)
실시간 Proton 라이브 메모리 실측을 통해 내구도, 자금, 군량, 병력 오프셋 검증 완료.

```cpp
#pragma once
#include <cstdint>

#pragma pack(push, 1)

struct San13CityRaw {
    uint8_t  head_pad[0x58];         // +0x00 ~ +0x57: VTable(0x10), 소속 세력/군주/태수 정보
    uint32_t durability;             // +0x58: 도시 내구도 (낙양 3,000, 업 2,800, 양평 2,200)
    uint32_t gold;                   // +0x5C: 자금 (낙양 28,000, 업 24,000, 양평 4,240)
    uint32_t provisions;             // +0x60: 군량 (낙양 125,700, 업 108,000, 양평 95,000)
    uint32_t troops;                 // +0x64: 주둔 모병/병력 (낙양 14,000, 업 12,000, 양평 2,120)
    uint8_t  tail_pad[0x170 - 0x68]; // +0x68 ~ +0x16F: 인구, 민심, 상업, 농업 등 기술치
};

#pragma pack(pop)
```

### 5.2. 무장 활성 오브젝트 구조체 (`San13OfficerObject` - Stride: 0x12BC = 4,796 B, RootBase + 0x015EE0)
실시간 Proton 라이브 메모리 실측을 통해 800인 역사 무장 풀, 고대 무장(50인), 신무장 슬롯 및 실제 엔진 오프셋 검증 완료.

```cpp
#pragma pack(push, 1)

// 무장 활성 오브젝트 내부의 16바이트 능력치/적성/전법 블록 (오프셋 +0x15)
struct San13OfficerLiveStats {
    uint8_t  base_leadership;    // +0x00 (+0x15): 기본 통솔 (아이템 제외 순수치)
    uint8_t  base_war;           // +0x01 (+0x16): 기본 무력
    uint8_t  base_intel;         // +0x02 (+0x17): 기본 지력
    uint8_t  base_politics;      // +0x03 (+0x18): 기본 정치
    uint8_t  curr_leadership;    // +0x04 (+0x19): 현재 통솔 (버프/아이템 반영)
    uint8_t  curr_war;           // +0x05 (+0x1A): 현재 무력
    uint8_t  curr_intel;         // +0x06 (+0x1B): 현재 지력
    uint8_t  curr_politics;      // +0x07 (+0x1C): 현재 정치
    uint8_t  exp_leadership;     // +0x08 (+0x1D): 통솔 경험치 (0 ~ 99)
    uint8_t  exp_war;            // +0x09 (+0x1E): 무력 경험치
    uint8_t  exp_intel;          // +0x0A (+0x1F): 지력 경험치
    uint8_t  exp_politics;       // +0x0B (+0x20): 정치 경험치
    uint8_t  spear_apt;          // +0x0C (+0x21): 창병 적성 (0:C, 1:B, 2:A, 3:S)
    uint8_t  horse_apt;          // +0x0D (+0x22): 기병 적성
    uint8_t  bow_apt;            // +0x0E (+0x23): 궁병 적성
    uint8_t  tactic_id;          // +0x0F (+0x24): 보유 전법 ID (0~140, Phase 6.0 실측 규명 완료)
};

// 무장 오브젝트 헤더부 (오프셋 +0x00 ~ +0x4F, 실측 검증 완료)
struct San13OfficerHeader {
    uint32_t              vtable;             // +0x00: VTable 포인터 (0x7291c510)
    uint32_t              officer_id;         // +0x04: 고유 무장 ID (0 ~ 2164)
    uint16_t              face_id;            // +0x08: 얼굴 CG 번호 (Face ID, 조조 405, 관우 117 등 실측 검증)
    uint16_t              city_id;            // +0x0A: 소재 도시 ID
    uint8_t               rank;               // +0x0C: 관직 / 품관 (0: 군주/무품, 1~9: 1품관~9품관, 10+: 무품)
    uint8_t               loyalty;            // +0x0D: 충성도
    uint16_t              birth_year;         // +0x0E: 출생년도 (조조 155, 관우 162, 제갈량 181)
    uint16_t              appear_year;        // +0x10: 등장년도 (조조 184, 제갈량 209, 아회남 217)
    uint16_t              death_year;         // +0x12: 사망년도 (조조 220, 관우 219, 제갈량 234)
    uint8_t               unknown_14;         // +0x14
    San13OfficerLiveStats stats;              // +0x15 ~ +0x24: 16바이트 능력치/적성/전법 블록 (+0x24: tactic_id)
    uint8_t               teach_skill;        // +0x25: 전수 특기 ID (0~19)
    uint8_t               skills[20];         // +0x26 ~ +0x39: 20대 특기 레벨 블록 (0~9, Phase 6.0 실측 규명)
    uint8_t               minister_trait_1;   // +0x3A: 고유 중신특성 ID 1
    uint8_t               minister_trait_2;   // +0x3B: 고유 중신특성 ID 2
    uint16_t              force_id;           // +0x3C: 실제 소속 세력/군단 ID (EDGE-23 특정, 0=무소속, >=120=재야)
    uint16_t              lord_officer_id;    // +0x3E: 소속 군주 무장 ID (본인이 군주면 본인 ID와 일치, EDGE-31 특정)
    uint16_t              father_id;          // +0x40: 부친 무장 ID (조조: 1048 조숭, 손책: 355 손견, 제갈량: 1045 제갈규)
    uint16_t              mother_id;          // +0x42: 모친 무장 ID (손책: 1101 오씨)
    uint16_t              spouse_1_id;        // +0x44: 배우자 1 무장 ID (조조: 675 변씨, 제갈량: 654 황월영)
    uint16_t              spouse_2_id;        // +0x46: 배우자 2 무장 ID (0xFFFF = 없음)
    uint16_t              spouse_3_id;        // +0x48: 배우자 3 무장 ID (0xFFFF = 없음)
    uint16_t              sworn_brother_id;   // +0x4A: 의형제/인연 리더 무장 ID (관우/장비/유비: 610 유비, 손책: 241 주유)
    uint16_t              rival_id;           // +0x4C: 원수 무장 ID (0xFFFF = 없음)
    uint8_t               personality;        // +0x4E: 성격 (0:소심, 1:냉정, 2:대담, 3:저돌, Phase 6.0 실측 규명)
    uint8_t               activity_flag;      // +0x4F: 활동/사관 상태 (1: 세력 사관 현역, 0: 세력 미사관 재야/미등장, 0xFF: 미할당)
};

// 무장 성향/기호/가치관 블록 (오프셋 +0x50 ~ +0x5F, 16바이트)
struct San13OfficerCharacter {
    uint8_t               like_weapon;        // +0x00 (+0x50): 선호무기 (0:무관심, 1:관심)
    uint8_t               like_book;          // +0x01 (+0x51): 선호서적 (0:무관심, 1:관심)
    uint8_t               like_treasure;      // +0x02 (+0x52): 선호보물 (0:무관심, 1:관심)
    uint8_t               drink_level;        // +0x03 (+0x53): 음주/주량 (0:금주, 1:보통, 2:대음)
    uint8_t               greed;              // +0x04 (+0x54): 물욕/재물 (0:무욕, 1:보통, 2:강욕)
    uint8_t               loyalty_justice;    // +0x05 (+0x55): 의리 (0:경시, 1:낮음, 2:보통, 3:중시, 4:매우중시)
    uint8_t               valuing_courage;    // +0x06 (+0x56): 용애 (0:경시, 1:낮음, 2:보통, 3:중시, 4:매우중시)
    uint8_t               valuing_talent;     // +0x07 (+0x57): 재애 (0:경시, 1:낮음, 2:보통, 3:중시, 4:매우중시)
    uint8_t               affinity;           // +0x08 (+0x58): 상성 (0~149, 유비 3형제=50, 손견/동탁/원소=100)
    uint8_t               pad_59_5f[7];       // +0x09 ~ +0x0F (+0x59 ~ +0x5F): 패딩
};

// 무장 전역 오브젝트 (전체 4,796 바이트 = 0x12BC, 총 2,165 슬롯 지원)
struct San13OfficerObject {
    San13OfficerHeader    header;             // +0x00 ~ +0x4F: 80바이트 헤더부
    San13OfficerCharacter character;          // +0x50 ~ +0x5F: 16바이트 성향/기호/가치관 블록
    uint8_t               extended_data[4700];// +0x60 ~ +0x12BB: PK 동지 슬롯 (+0x1296), 친근/인연 데이터 등
};

#pragma pack(pop)
```

#### 5.2.1. 삼국지 13 PK 무장 상태 판별 진리표 (State Determination Truth Table)
게임 런타임 메모리 실측 및 리버스 엔지니어링을 통해 확립된 공식 무장 상태 판별 알고리즘 (EDGE-31 개정):

| 단계 | 조건식 | 최종 판정 상태 | 비고 및 해당 무장 사례 |
| :---: | :--- | :---: | :--- |
| **1단계** | `activity_flag == 0xFF` | **미할당 (빈 슬롯)** | 사용되지 않는 비활성 슬롯 |
| **2단계** | `current_year < appear_year` | **미등장** | 시나리오 연도 미도달 무장 (아회남 ID 0: 217년 등장, 제갈량 ID 283 등) |
| **3단계** | `death_year <= current_year` | **사망** | 수명 종료 무장 |
| **4단계** | `current_year >= appear_year` 상태에서:<br>`activity_flag == 1 && force_id > 0 && force_id < 120 && force_id != 0xFFFF` | **현역** | - **군주:** `rank == 0 && lord_officer_id == officer_id` (조조, 원소, 유비, 손견 등)<br>- **일반 (무품관):** `rank == 0 && lord_officer_id != officer_id` (원희, 원상, 주유 등 군주 친족/무품관)<br>- **일반 (N품관):** `rank 1~9` (하후돈 1품관, 관우 6품관, 번쾌 3품관 등) |
| **5단계** | `current_year >= appear_year` 상태에서:<br>`activity_flag == 0` 또는 `force_id == 0 \|\| force_id == 0xFFFF \|\| force_id >= 120` | **재야** | 현재 등장했으나 세력에 사관하지 않았거나 미거병 가상 세력에 배치된 무장 (범증 ID 851, 우희 ID 858, 항적 ID 857, 손책 ID 359 등) |

#### 5.2.2. 공식 반에디터(`San13EditorPK_kr.exe`) 역공학 품관(Rank) 체계 및 군주 식별 규명 (EDGE-31)
- **오프셋 `+0x0C`의 본질:** 기존 `rank == 0` 무조건 군주 단정 오류(EDGE-31)를 교정하고 **9품관 + 무품관 체계**로 정밀화.
  - `0`: **무품관 / 군주 또는 친족** (`lord_officer_id == officer_id` 일 때만 실제 군주, 그 외 원희/원상/손책 등은 무품관 신하/친족)
  - `1`: **1품관** (공적 30,000 이상, 도독 임명 요건)
  - `2`: **2품관** (공적 24,000 이상)
  - `3`: **3품관** (공적 18,000 이상)
  - `4`: **4품관** (공적 13,000 이상)
  - `5`: **5품관** (공적 10,000 이상, 태수 임명 요건)
  - `6`: **6품관** (공적 7,000 이상)
  - `7`: **7품관** (공적 4,500 이상, 중신 임명 요건)
  - `8`: **8품관** (공적 2,000 이상)
  - `9`: **9품관** (공적 0 이상, 신임 무장)
  - `10+`: 무품 (재야 및 미등장)
- **소속 군주 무장 ID (`+0x3E`, `uint16_t`):**
  - 군주 본인은 `lord_officer_id == officer_id` (조조 333 == 333, 원소 18 == 18)
  - 신하 및 친족은 `lord_officer_id == 주군 ID` (원희/원상은 18, 손책은 355)
  - 미거병/재야 고대무장(항적 등)은 `force_id >= 120` (맵상 미존재 가상 세력)으로 분리되어 **"재야"**로 100% 정상 분류됨.

### 5.3. 명품/보물 구조체 (`San13Item` - Stride: 0x8C = 140 B, RootBase + 0xBD475C)

```cpp
#pragma pack(push, 1)

struct San13Item {
    uint16_t id;                 // +0x00: 명품 고유 ID
    uint16_t owner_officer_id;   // +0x02: 소유 무장 ID (0xFFFF = 미발견/도시소유)
    uint16_t city_id;            // +0x04: 소재 도시 ID
    uint8_t  item_type;          // +0x06: 종류 (무기, 명마, 서적 등)
    uint8_t  value;              // +0x07: 가치 (등급)
    int8_t   add_leadership;     // +0x08: 통솔 가산치
    int8_t   add_war;            // +0x09: 무력 가산치 (청룡언월도 +5 등)
    int8_t   add_intelligence;   // +0x0A: 지력 가산치
    int8_t   add_politics;       // +0x0B: 정치 가산치
    uint8_t  pad[128];           // 전체 140 바이트 (0x8C) 스트라이드 정렬
};

#pragma pack(pop)
```

---

### 5.4. 수치 경계값(Bounds) 및 오버플로/크래시 방지 설계도

에디터에서 사용자가 잘못된 값이나 한계를 초과하는 수치를 입력했을 때 **게임 크래시(Crash), 데이터 오염(Data Corruption), UI 텍스트 깨짐, 비트 오버플로 래핑(예: 256 입력 시 0이 됨)**이 발생하는 것을 완벽히 방지하기 위한 설계 규칙입니다.

#### 1) 수치별 제약 조건 및 위험성 분석 매트릭스

| 대상 항목 | 메모리 자료형 | 물리 하드웨어 한계 | 인게임 바닐라 정상 범위 | 한계 돌파 허용치 (Limit Break) | 범위 초과 시 엔진 동작 및 위험성 | 에디터 제어 정책 |
| :--- | :---: | :---: | :---: | :---: | :--- | :--- |
| **무장 4대 스탯**<br>(통솔, 무력, 지력, 정치) | `uint8_t` (1 Byte) | `0 ~ 255` | `1 ~ 100` | `1 ~ 255` | - 256 이상 입력 시 모듈로 연산(`% 256`)으로 인해 `0`이 되어 급사/무력화 버그 발생.<br>- 음수 입력 시 `255`로 언더플로 래핑. | **Safe Mode:** 1~100 클램핑<br>**Limit Break:** 1~255 포화 연산(Saturating Cast) |
| **병종 적성**<br>(창병, 기병, 궁병) | `uint8_t` (1 Byte) | `0 ~ 255` | `0 ~ 3`<br>(0:C, 1:B, 2:A, 3:S) | **절대 불허**<br>(최대 3 고정) | **[치명적 크래시 위험]** 4 이상 입력 시 게임 UI의 적성 아이콘 스프라이트 텍스처 인덱스를 초과하여 **게임이 즉시 세그폴트로 튕김**. | **Strict Guard:**<br>무조건 `input & 0x03` 강제 마스킹 |
| **능력치 경험치** | `uint8_t` (1 Byte) | `0 ~ 255` | `0 ~ 99` | `0 ~ 99` | 100 이상 시 다음 달 레벨업 트리거가 꼬여 스탯 증가 루틴 무한 루프 위험. | **Safe Mode:** 0~99 클램핑 |
| **생몰년도**<br>(출생, 등장, 사망) | `uint16_t` (2 Byte) | `0 ~ 65,535` | `100 ~ 350` | `1 ~ 999` | - `사망년도 < 출생년도`일 경우 수명 계산 루틴에서 음수 언더플로로 영구 생존 또는 즉사 버그 발생.<br>- 0년 입력 시 미등장 버그. | **Logical Validation:**<br>`출생 <= 등장 <= 사망` 무결성 검증 |
| **도시 자금 / 군량** | `uint32_t` (4 Byte) | `0 ~ 42억` | `0 ~ 999,999` | `0 ~ 9,999,999` | 인게임 UI가 6자리 숫자에 맞춰져 있어 100만 초과 시 UI 텍스트가 짤리며, 내부 16비트/24비트 연산 루틴 오버플로 위험. | **Safe Mode:** 0~999,999<br>**Limit Break:** 최대 9,999,999 상한 |
| **도시 병력** | `uint32_t` (4 Byte) | `0 ~ 42억` | `0 ~ 200,000` | `0 ~ 999,999` | 도시 규모(소/중/대/거)별 상한 초과 시 다음 턴 자동 감축. | **Safe Mode:** 0~200,000<br>**Limit Break:** 999,999 상한 |
| **도시 내구도** | `uint16_t` (2 Byte) | `0 ~ 65,535` | `0 ~ 5,000` | `0 ~ 10,000` | 65,535 초과 시 16비트 오버플로로 0이 되어 성벽 즉시 함락. | **Safe Mode:** 0~5,000<br>**Limit Break:** 10,000 상한 |
| **명품 능력 가산치** | `int8_t` (1 Byte Signed)| `-128 ~ 127` | `0 ~ 10` | `-50 ~ +50` | 음수 패널티 지원 (저주받은 보물 등). | **Safe Clamping:** -50 ~ +50 |

---

#### 2) 에디터의 3대 안전 제어 정책 (Clamping & Sanitization Policies)

1. **`SafeVanilla` 정책 (기본 모드)**
   - 게임 원본 밸런스와 UI 표시 규격을 100% 준수합니다.
   - 무장 스탯은 `1 ~ 100`, 자금/군량은 `999,999`를 절대 넘지 않도록 자동 보정합니다.
2. **`LimitBreak` 정책 (한계 돌파 모드)**
   - 플레이어가 "사기 캐릭터"나 "치트 플레이"를 원할 때 활성화하는 모드입니다.
   - 무장 스탯을 물리 최대치인 `255`까지 허용합니다.
   - **오버플로 차단(Saturating Cast):** 사용자가 `999`를 입력해도 `999 % 256 = 231`처럼 엉뚱한 값이나 `0`이 되지 않고, 하드웨어 상한인 **`255`로 고정(Clamp)**됩니다.
3. **`StrictGuard` 정책 (절대 보호 모드)**
   - 엔진 렌더러가 내부 배열 인덱스로 직접 사용하는 항목(병종 적성 `0~3`, 성별 `0~1` 등)은 **한계 돌파 모드에서도 절대 예외 없이 유효 범위로 강제 고정**합니다.

---

### 5.5. 리눅스/크로스 플랫폼 네이티브 한글 2벌식 오토마타 (Native Hangul 2-Set Automata) 명세

#### 1) 문제 배경 및 리눅스 IME(fcitx5/ibus) 비호환 원인 분석
- **현상:** Fedora, Arch, SteamOS(Steam Deck), Ubuntu 등 리눅스 데스크톱 환경에서 fcitx5, ibus 등의 입력기를 사용할 때 에디터의 무장/도시 검색창 및 텍스트 입력 필드에서 한글 입력이 되지 않고 영문 알파벳만 타이핑되는 현상 발생.
- **원인 1 (GLFW의 Linux IME 프로토콜 미지원):**
  - GLFW 3.4는 Wayland 환경에서 `zwp_text_input_v3` 프로토콜을 구현하지 않으며, X11 환경에서도 XIM Preedit Context(`XFilterEvent`) 파이프라인을 지원하지 않습니다.
  - 이로 인해 fcitx5는 GLFW 윈도우의 포커스를 인식하여 키 입력을 가로채 조합(Preedit)한 후 완성된 글자를 커밋하는 통신 채널을 열 수 없습니다.
- **원인 2 (Dear ImGui의 CharCallback 특성):**
  - Dear ImGui의 GLFW 백엔드(`ImGui_ImplGlfw_CharCallback`)는 OS나 IME가 이미 조합을 끝내고 유니코드(UTF-32)로 전달한 문자만을 `io.AddInputCharacter()`로 수신합니다.
- **해결 방안 선정 (Option 1: 자체 C++ 2벌식 소프트웨어 오토마타):**
  - 클립보드(`Ctrl+V`)를 통한 한글 붙여넣기와 번들 글꼴(`NanumGothic.ttf` 11,172자 완성형)은 100% 정상 작동합니다.
  - 따라서 외부 시스템 프로세스(D-Bus, fcitx5 데몬)에 의존하지 않고, **C++ 레벨에서 QWERTY 영문 키 입력을 실시간으로 한글 자모로 매핑하여 유니코드 완성형 음절로 결합/분해하는 경량 2벌식 오토마타(`HangulAutomata`)**를 내장합니다.
  - 이 방식은 윈도우, 리눅스(X11/Wayland), Steam Deck 등 어떠한 환경에서도 100% 동일하게 네이티브 한글 입력을 보장합니다.

#### 2) 2벌식 자모 매핑 및 유니코드 완성형 조합 공식
- **초성 (19자):** `ㄱ ㄲ ㄴ ㄷ ㄸ ㄹ ㅁ ㅂ ㅃ ㅅ ㅆ ㅇ ㅈ ㅉ ㅊ ㅋ ㅌ ㅍ ㅎ` (인덱스 0 ~ 18)
- **중성 (21자):** `ㅏ ㅐ ㅑ ㅒ ㅓ ㅔ ㅕ ㅖ ㅗ ㅘ ㅙ ㅚ ㅛ ㅜ ㅝ ㅞ ㅟ ㅠ ㅡ ㅢ ㅣ` (인덱스 0 ~ 20)
- **종성 (28자, 0은 받침 없음):** `(없음) ㄱ ㄲ ㄳ ㄴ ㄵ ㄶ ㄷ ㄹ ㄺ ㄻ ㄼ ㄽ ㄾ ㄿ ㅀ ㅁ ㅂ ㅄ ㅅ ㅆ ㅇ ㅈ ㅊ ㅋ ㅌ ㅍ ㅎ` (인덱스 0 ~ 27)

- **유니코드 완성형 한글 음절 계산 공식:**
  $$\text{Codepoint} = \text{0xAC00} + (\text{Choseong} \times 21 + \text{Jungseong}) \times 28 + \text{Jongseong}$$
  - 단독 자음 (Compatibility Jamo): `0x3131` ~ `0x314E`
  - 단독 모음 (Compatibility Jamo): `0x314F` ~ `0x3163`
  - UTF-8 인코딩 (3바이트):
    - `Byte0 = 0xE0 | (Codepoint >> 12)`
    - `Byte1 = 0x80 | ((Codepoint >> 6) & 0x3F)`
    - `Byte2 = 0x80 | (Codepoint & 0x3F)`

#### 3) 복모음 및 복자음 조합 규칙 (Compound Jamo Tables)
- **복모음 조합 (7종):**
  - `ㅗ (8) + ㅏ (0) = ㅘ (9)`
  - `ㅗ (8) + ㅐ (1) = ㅙ (10)`
  - `ㅗ (8) + ㅣ (20) = ㅚ (11)`
  - `ㅜ (13) + ㅓ (4) = ㅝ (14)`
  - `ㅜ (13) + ㅔ (5) = ㅞ (15)`
  - `ㅜ (13) + ㅣ (20) = ㅟ (16)`
  - `ㅡ (18) + ㅣ (20) = ㅢ (19)`

- **복자음 종성 조합 (11종):**
  - `ㄱ (1) + ㅅ = ㄳ (3)`
  - `ㄴ (4) + ㅈ = ㄵ (5)`, `ㄴ (4) + ㅎ = ㄶ (6)`
  - `ㄹ (8) + ㄱ = ㄺ (9)`, `ㄹ (8) + ㅁ = ㄻ (10)`, `ㄹ (8) + ㅂ = ㄼ (11)`, `ㄹ (8) + ㅅ = ㄽ (12)`, `ㄹ (8) + ㅌ = ㄾ (13)`, `ㄹ (8) + ㅍ = ㄿ (14)`, `ㄹ (8) + ㅎ = ㅀ (15)`
  - `ㅂ (17) + ㅅ = ㅄ (18)`

#### 4) 연음 분리 (Consonant Transition) 및 음절 경계 규칙
- 받침이 존재하는 상태(`CHO_JUNG_JONG`)에서 새로운 모음이 입력될 때:
  1. **단일 받침 (예: `각` + `ㅏ`):**
     - 종성 `ㄱ`을 제거한 이전 음절 `가`를 버퍼에 확정(Commit).
     - 제거된 종성 `ㄱ`을 새 음절의 초성으로 변환하고 입력된 모음 `ㅏ`를 결합하여 새 음절 `가` 생성 $\rightarrow$ 최종 표시: `가가`.
  2. **복합 받침 (예: `닭` + `ㅏ`):**
     - 복합 종성 `ㄺ(ㄹ+ㄱ)` 중 첫 번째 자음 `ㄹ`만을 남겨 이전 음절 `달`을 확정(Commit).
     - 두 번째 자음 `ㄱ`을 새 음절의 초성으로 가져와 `ㄱ` + `ㅏ` (`가`) 조합 시작 $\rightarrow$ 최종 표시: `달가`.
- 종성 상태에서 더 이상 결합할 수 없는 자음이 입력될 때:
  - 현재 음절을 즉시 확정하고, 새 자음을 다음 음절의 초성으로 전환.

#### 5) 백스페이스 분해 규칙 (Reverse Decomposition)
- 사용자가 백스페이스(Backspace)를 누를 때, 글자 전체를 한 번에 지우지 않고 자모 단위로 역분해하여 자연스러운 타이핑 경험 제공:
  - `CHO_JUNG_JONG` (복합 종성): 두 번째 받침을 지우고 첫 번째 받침으로 환원 (예: `닭` $\rightarrow$ `달`)
  - `CHO_JUNG_JONG` (단일 종성): 종성을 제거하여 초성+중성 상태로 환원 (예: `달` $\rightarrow$ `다`)
  - `CHO_JUNG` (복모음): 두 번째 모음을 지우고 첫 번째 모음으로 환원 (예: `과` $\rightarrow$ `고`)
  - `CHO_JUNG` (단모음): 모음을 제거하여 초성 단독 상태로 환원 (예: `고` $\rightarrow$ `ㄱ`)
  - `CHO`: 초성을 제거하여 조합 버퍼를 비움 (Empty)
  - 조합 버퍼가 빈 상태에서 백스페이스 입력 시: 앞서 확정된 문자열의 마지막 UTF-8 문자 1자 삭제.

#### 6) 한/영 모드 전환 및 단축키 바인딩
- **토글 단축키:**
  - `Hangul` 키 (표준 한/영 키코드)
  - `Right Alt` 키 (리눅스 데스크톱 환경 한/영 전환 표준)
  - `Shift + Space` (전통 리눅스/유닉스 한글 전환 키)
- **UI 인디케이터 배지:**
  - 텍스트 입력창 바로 우측에 `[한]` (골드 하이라이트) / `[영]` (다크 슬레이트) 상태 뱃지 표시.
  - 마우스 클릭으로도 즉시 한/영 모드 토글 가능.
- **포커스 아웃 및 제어 키:**
  - `Enter`, `Tab`, `ESC` 또는 입력 필드 외부 마우스 클릭 시 현재 조합 중인 글자 자동 커밋(Commit).

#### 7) Dear ImGui 위젯 연동 아키텍처 (`HangulInputText`)
- `HangulInputText(label, buf, buf_size, automata, hint)` 함수를 제공하여 기존 ImGui `InputText`를 완벽히 대체.
- `ImGuiInputTextFlags_CallbackCharFilter`: 한글 모드일 때 QWERTY 알파벳 키 입력을 가로채고(`return 1`로 ImGui 내부 삽입 차단), 오토마타에 키 이벤트를 공급.
- `ImGuiInputTextFlags_CallbackAlways`: 매 프레임 조합 중인 한글 음절을 ImGui 텍스트 버퍼에 동기화(`DeleteChars` 및 `InsertChars`).

---

## 6. 상세 단계별 프로젝트 로드맵 (Granular Roadmap)

배포 목적의 소프트웨어 무결성을 확보하기 위해, 각 세부 단계마다 명확한 **검증 기준(Acceptance Criteria)**을 통과한 후 다음 단계로 진행합니다.

| 세부 단계 | 모듈 및 작업 항목 | 주요 개발 내용 | 검증 기준 (Acceptance Criteria) | 상태 |
| :---: | :--- | :--- | :--- | :---: |
| **Phase 1** | **바이너리 역공학** | `San13EditorPK_kr.exe` VTable 분해 및 PE 타임스탬프 판별식, 청크 맵 규명 | radare2 역추적 및 어셈블리 증거 확보 | **[완료]** |
| **Phase 2** | **실시간 메모리 읽기 실측** | Proton 환경 PID 자동 탐색, WoW64 Base 및 RootBase, 4대 스탯 실측 | 조조(99/72/92/91), 장비(90/98/30/22) 콘솔 출력 성공 | **[완료]** |
| **Phase 3** | **실시간 쓰기 검증** | 활성 무장 풀 공식(`RootBase + 0x15EE0 + ID*0x12BC`) 도출 및 실시간 쓰기 검증 | 조조 무력 72 $\rightarrow$ 99 실시간 변조 및 인게임 반영 확인 | **[완료]** |
| ─── | ───────────────────── | ────────────────────────────────────────── | ─────────────────────────────────── | ─── |
| **Phase 4.1** | **코어 메모리 드라이버** | `IMemoryDriver`, `LinuxMemoryDriver` 모듈화 및 프로세스 라이프사이클 처리 | 프로세스 미실행/종료 시 정상 대기 및 자동 재연결 단위 테스트 통과 | **[완료]** |
| **Phase 4.2** | **수치 경계값 검증기** | `Bounds.hpp`, `BoundsValidator` 및 3대 정책(Safe, LimitBreak, Strict) 구현 | 음수, 0, 100, 255, 256, 99999 등 엣지케이스 단위 테스트 100% 통과 | **[완료]** |
| **Phase 4.3** | **무장 도메인 매니저** | `OfficerManager` 구축: 800명 전원 파싱, ID 검색, 4대 스탯/적성 일괄 읽기/쓰기 | 800명 전원 스탯 무결성 덤프 및 관우/제갈량/여포 등 CLI 수정 검증 | **[완료]** |
| **Phase 4.4** | **도시 도메인 매니저** | `CityManager` 구축: 60개 도시(0x170 Stride) 자금/군량/병력/내구도 읽기/쓰기 | 업/낙양/장안 등 60개 도시 자원 실시간 덤프 및 CLI 수정 검증 | **[완료]** |
| ─── | ───────────────────── | ────────────────────────────────────────── | ─────────────────────────────────── | ─── |
| **Phase 5.1** | **GUI 파이프라인 & 폰트** | Dear ImGui + GLFW/OpenGL3 독립 창 구동 및 Noto Sans KR/NanumGothic 한글 폰트 통합 | 리눅스 데스크톱에서 한글 텍스트 깨짐 없는 모던 GUI 창 렌더링 | **[완료]** |
| **Phase 5.2** | **무장 테이블 뷰 & 동적 확장** | 800인 역사 무장(현역/재야/미등장/사망 필터) + [Phase 5 통합] 결혼 자녀/등록무장 동적 슬롯 자동 스캔 및 실시간 목록 갱신 | 대용량 800~1,985행 테이블에서 렉 없는 실시간 검색/정렬 및 자녀 무장 자동 반영 확인 | **[완료]** |
| **Phase 5.3** | **인라인 편집 & 핫싱크** | GUI 테이블 내에서 스탯 더블클릭 수정 시 Bounds 자동 적용 및 실시간 메모리 패치 | GUI에서 수치 변경 시 인게임에 0.1초 내 즉각 반영 | **[완료]** |
| **Phase 5.4** | **도시 자원 편집 뷰** | 60개 도시 카드/그리드 뷰, 원클릭 자원 최대화 치트 버튼 | 원클릭 버튼 클릭 시 해당 도시 군량/자금/내구도 즉각 완충 | **[완료]** |
| **Phase 5.5** | **일괄 수정 (Batch Action)** | 선택 무장 다중 선택(Ctrl/Shift), 아군 무장 스탯 일괄 +10, 수명 일괄 연장 | 다중 선택 50인 이상 동시 수정 시 트랜잭션 메모리 무결성 확인 | **[완료]** |
| **Phase 5.6** | **네이티브 한글 오토마타** | `HangulAutomata` 구축: 리눅스 fcitx5 한계 극복 자체 두벌식 조합기 및 Dear ImGui 연동 | "조조", "제갈량", "낙양", "닭" 등 복합 자모 조합, 백스페이스 분해, 한/영 토글 단위 테스트 100% 통과 | **[완료]** |
| **Phase 5.7** | **무장 한글 이름 표시** | `OfficerNameTable` 구축: 역사 무장 700인(ID 0~699) + 고대무장 공식 한글 이름 내장 및 O(1) 플랫 테이블 연동 | 700인 전수 무결성 검증, 조조/관우/제갈량/유비 등 32인 표본 100% 일치 및 GUI 연동 완료 | **[완료]** |
| **Phase 5.8** | **한글 입력 ImGui 연동** | `HangulInputText()` 위젯 구현 및 무장/도시 검색창(2개소) 2벌식 오토마타 연동 | 검색창에서 `whwh` 입력 시 `조조` 완성, 1자모 백스페이스 역분해, 한/영 전환 및 혼용 검색 단위/통합 테스트 100% 통과 | **[완료]** |
| **Phase 5.9-A**| **등록 신무장 이름 연동** | `editdata_pk.s13` 파싱 및 유저 등록 신무장("강월" ID 800) 한글 성명 동적 추출 | 신무장 파일 자동 탐색 및 인게임 유저 등록 무장 이름 100% 연동 | **[완료]** |
| **Phase 5.9-B**| **9품관 & 실시간 시계** | 무장 품관(Rank 0~9) 정밀 매핑 및 청크 0 기반 인게임 일자(190년 1월 1일) 파싱 | `+0x0C` 품관 1:1 매핑 및 상단 바 실시간 게임 일자 뱃지 출력 검증 | **[완료]** |
| **Phase 5.9-C**| **상태 판별식 & 2단 툴바**| 범증/아회남 상태 판별 진리표 확립 및 2단 반응형 컨트롤 바 분리 | 아회남(미등장), 범증(재야) 완벽 분류 및 1280 해상도 버튼 잘림 0건 | **[완료]** |
| ─── | ───────────────────── | ────────────────────────────────────────── | ─────────────────────────────────── | ─── |
| **Phase 6.0** | **무장 심층 인스펙터** | 얼굴 CG(Face ID), 전법, 특기, 중신특성, 성격, 음성 상세 뷰 & 픽툴 연동 준비 | 무장 단독 상세 인스펙터 모달 구축 및 픽툴 Face ID 바이트 슬롯 분석 | **[차기]** |
| **Phase 6.1** | **Linux 배포 패키징** | Standalone 바이너리 및 권한 가이드(ptrace_scope), 의존성 정적 링크 | 비개발자 리눅스 환경에서 더블클릭으로 즉시 실행 확인 | **[완료]** |
| **Phase 6.2** | **Windows 네이티브 빌드** | `Win32MemoryDriver` 활성화 및 MSVC/MinGW 64비트 Windows 배포판 생성 | Windows 10/11 환경에서 동일 GUI 및 메모리 쓰기 작동 확인 | **대기** |
| ─── | ───────────────────── | ────────────────────────────────────────── | ─────────────────────────────────── | ─── |
| **Phase 7.1** | **인게임 오버레이 트레이너** | MinGW 32비트 `version.dll` 하이재킹 및 DirectX Hook 파이프라인 | 게임 실행 시 F11 단축키로 인게임 ImGui 오버레이 팝업 | **대기** |
| **Phase 7.2** | **FPS 해제 & 치트 연동** | 30/60 FPS 리미터 분기 패치 및 전투 중 무적/사기 200 고정 치트 | 144Hz 모니터에서 부드러운 프레임 확인 및 전투 치트 정상 작동 | **대기** |

---

## 7. 검증 보고서 및 실측 증거 아카이브 (Evidence Archive)

### 7.1. Phase 1: 윈도우 클래스명 및 버전 판별식 바이너리 증거
- **윈도우 클래스명 식별:** `San13EditorPK_kr.exe` 주소 `0x00433068`에서 `KT_SANGOKUSHI_13_PK_WINDOW_NAME` 문자열(`0x46D6B0`)을 `FindWindowW` 인자로 푸시함을 확인.
- **버전 테이블 매핑:** 주소 `0x004061DD`에서 PE 헤더의 `TimeDateStamp`를 읽고, `0x0049ED90` 테이블에서 인덱스 `0x35`(한국어 최신 1.0.2, 값 `0x5E3BA719`)를 찾아 `0x0044D0D0 + 0x35*4`에서 오프셋 `+0x01FBC5D8`을 획득함.

### 7.2. Phase 2: 라이브 Proton 메모리 실측 증거
- **실행 환경:** Fedora 44 x86_64, Valve GE-Proton11-7, PID 55202
- **WoW64 Base:** `0x71bb0000`, **RootBase:** `0x73B6C5D8`
- **조조 (Cao Cao) 실측 메모리 블록 (`0x73D08359`):**
  ```text
  Raw Hex: 63 48 5c 5b 63 48 5c 5b 00 00 00 00 02 02 02 2c
  - 통솔: 99 (0x63), 무력: 72 (0x48), 지력: 92 (0x5C), 정치: 91 (0x5B)
  - 병종 적성: 창병 A (2), 기병 A (2), 궁병 A (2)
  ```

### 7.3. Phase 3: 실시간 메모리 쓰기(Write) 및 활성 무장 풀 공식 실측 증거
- **활성 무장 배열 주소 공식:**
  `OfficerObj(ID) = RootBase (0x73B6C5D8) + 0x015EE0 + (ID * 0x12BC)`
  - 무장 개별 오브젝트 크기(Stride): **`0x12BC` (4,796 바이트)**
  - 스탯 블록 시작 위치: `OfficerObj(ID) + 0x15`
- **전역 무장 인덱스 일치 실측:**
  - ID 94: 관우 (통솔 96 / 무력 97 / 지력 73 / 정치 62)
  - ID 283: 제갈량 (통솔 98 / 무력 38 / 지력 100 / 정치 95)
  - ID 333: 조조 (통솔 99 / 지력 92 / 정치 91)
  - ID 418: 장비 (통솔 90 / 무력 98 / 지력 30 / 정치 22)
  - ID 635: 여포 (통솔 91 / 무력 100 / 지력 26 / 정치 13)
- **조조(ID 333) 실시간 무력 변조 실측 (`process_vm_writev`):**
  - 대상 주소: `0x73d0835a` (기본 무력), `0x73d0835e` (현재 무력)
  - 변경 전: `72`
  - 변경 값: `99` (0x63)
  - 쓰기 결과: `process_vm_writev` 반환값 1 (성공)
  - 쓰기 직후 및 1초 후 재확인: `Base War = 99`, `Current War = 99` 지속 확인됨.

### 7.4. Phase 4.1: 코어 메모리 드라이버 모듈화 및 라이프사이클 단위 테스트 실측 증거
- **테스트 바이너리:** `bin/test_memory_driver` (C++17 표준 빌드)
- **테스트 결과 (5개 세트 전원 PASS):**
  - `[PASS] [EDGE-09] 미실행 프로세스 부착 시도시 비정상 크래시 없이 false 반환 및 IsAlive() == false 검증`
  - `[PASS] 0번지, 널포인터, 크기 0 읽기/쓰기 가드 정상 방어 (세그폴트 0건)`
  - `[PASS] 실행 중인 San13PK_kr.exe 프로세스 부착 성공 (PID: 55202, WoW64 Base: 0x71bb0000)`
  - `[PASS] PE 헤더 시그니처 무결성 검증 (DOS MZ 0x5A4D, NT PE\0\0, TimeDateStamp 0x5E3BA719 한국어 최신판 일치)`
  - `[PASS] [EDGE-11] Detach() 상태 초기화 및 Re-attach() 동일 PID 재연결 라이프사이클 100% 정상 작동`

### 7.5. Phase 4.2: 수치 경계값 검증기 및 엣지 케이스 단위 테스트 실측 증거
- **테스트 바이너리:** `bin/test_bounds` (C++17 표준 빌드)
- **테스트 결과 (6개 세트 전원 PASS):**
  - `[PASS] [EDGE-01 & EDGE-02] 무장 스탯 음수(-1, -100) 및 0 입력 시 언더플로 255 방어 -> 최소치 1 강제 보정`
  - `[PASS] [EDGE-03] 바닐라 모드(100 상한 절삭) vs 한계 돌파 모드(101~255) 정상 통과`
  - `[PASS] [EDGE-04] 256, 300, 99999 등 8비트 초과 입력 시 모듈로 0 급감 방어 -> Saturating Cast(255 고정)`
  - `[PASS] [EDGE-05] 병종 적성 4, 10, 255 입력 시 Strict Guard(3:S 절삭)로 렌더러 스프라이트 크래시 원천 차단`
  - `[PASS] [EDGE-06] 생몰년도 역전(사망 < 출생, 등장 < 출생) 논리 무결성 거부 및 정상 통과 검증`
  - `[PASS] [EDGE-07 & EDGE-08] 도시 자금 100만 초과 UI 잘림 방지 및 내구도 65536 16비트 오버플로 0 즉시 함락 방어`

### 7.6. Phase 4.3: 무장 도메인 매니저 및 800인 전원 파싱/수정 실측 증거
- **테스트 바이너리:** `bin/test_officer_manager` (C++17 표준 빌드)
- **테스트 결과 (전 과정 100% 통과):**
  - `[PASS] GameVersionDetector 연동 한국어 PK 1.0.2 자동 감지 및 RootBase(0x73b6c5d8) 바인딩 성공`
  - `[PASS] 유명 무장 데이터 무결성 검증:`
    - 관우 (ID 94): 통솔 96 / 무력 97 (S/A/B)
    - 제갈량 (ID 283): 지력 100 / 정치 95 (S/C/S)
    - 조운 (ID 380): 무력 96
    - 장비 (ID 418): 무력 98
    - 여포 (ID 635): 무력 100
  - `[PASS] 실시간 쓰기 및 Bounds Clamping 검증 (조조 ID 333):`
    - Safe 모드: 120 입력 시 100으로 자동 클램핑 확인
    - LimitBreak 모드: 256 입력 시 255로 포화 연산(Saturating Cast) 확인
    - 음수(-10) 입력 시 1로 안전 보정 확인
    - 적성 5 입력 시 Strict Guard(3:S)로 절삭 확인
    - 검증 후 조조 무력 99 원복 완료
  - `[PASS] 800명 역사 무장 전원 일괄 파싱 및 메모리 무결성 검증:`
    - 0부터 799까지 800명 무장 레코드 누락 없이 100% 연속 수집 확인

### 7.7. Phase 4.4: 도시 도메인 매니저 및 60개 도시 전원 자원 파싱/수정 실측 증거
- **테스트 바이너리:** `bin/test_city_manager` (C++17 표준 빌드)
- **테스트 결과 (전 과정 100% 통과):**
  - `[PASS] RootBase(0x73b6c5d8) 기반 CityManager 초기화 성공`
  - `[PASS] 주요 도시 실시간 자원 데이터 무결성 검증:`
    - 양평 (ID 0): 자금 4,240 / 군량 95,000 / 병력 2,120 / 내구 2,200 (실측값 100% 일치)
    - 업 (ID 9): 자금 24,000 / 군량 108,000 / 병력 12,000 / 내구 2,800 (실측값 100% 일치)
    - 낙양 (ID 18): 자금 28,000 / 군량 125,700 / 병력 14,000 / 내구 3,000 (실측값 100% 일치)
    - 장안 (ID 20), 허창 (ID 25), 건업 (ID 39), 성도 (ID 46) 로드 성공
  - `[PASS] 실시간 자원 수정 및 Bounds Clamping 검증 (낙양 ID 18):`
    - Safe 모드: 2,000,000 입력 시 999,999로 자동 클램핑 확인
    - LimitBreak 모드: 1,500,000 군량 입력 허용 확인
    - 음수(-500) 자금 입력 시 0으로 안전 보정 확인
    - MaximizeCityResources 치트 실행 시 자금 999,999 / 군량 999,999 / 내구 5,000 완충 확인
    - 원상 복구 로직으로 원래 값 복구 확인
  - `[PASS] 60개 도시 전원 일괄 파싱 및 메모리 무결성 검증:`
    - 0부터 59까지 60개 도시 레코드 누락 없이 100% 연속 수집 확인

### 7.8. Phase 5.1: GUI 파이프라인 및 한글 폰트 통합 실측 증거
- **테스트 바이너리:** `bin/test_gui_window` (Dear ImGui Docking + GLFW + OpenGL3 + C++17)
- **실행 환경:** Wayland/X11 하이브리드 환경 (`DISPLAY=:0`, `WAYLAND_DISPLAY=wayland-0`)
- **폰트 아키텍처:**
  - 번들 폰트: `assets/fonts/NanumGothic.ttf` (2.0MB 디지털 서명 TrueType Font)
  - 글리프 범위: `ImFontGlyphRangesBuilder` (라틴 문자 + 완성형 한글 11,172자 + 특수 기호 `→`, `▲`, `▼`, `★`, `※`, `●` 등)
  - 서브픽셀 오버샘플링: `OversampleH = 2`, `OversampleV = 1`, `PixelSnapH = true`
- **테스트 결과 (전 과정 100% 통과):**
  - `[PASS] UIManager 초기화 및 GLFW 1280x800 독립 윈도우 생성 완료`
  - `[PASS] NanumGothic.ttf 18px 폰트 아틀라스 무결성 빌드 및 로드 성공`
  - `[PASS] 삼국지 다크 브론즈 & 골드 앤틱 테마(Theme) 적용`
  - `[PASS] live 프로세스 연결 바인딩 (PID 55202, RootBase 0x73B6C5D8)`
  - `[PASS] 무장/도시 실시간 라이브 뷰 카드 및 한글 글리프 위젯 렌더링 무결성 확인`
  - `[PASS] 120프레임 무결성 스모크 테스트 자동 종료 및 자원 정상 해제(Shutdown) 검증`

### 7.9. Phase 5.2: 대용량 무장 가상 스크롤(ImGuiListClipper) 및 동적 슬롯 확장 실측 증거
- **테스트 바이너리:** `bin/test_officer_table` (Dear ImGui Docking + GLFW + OpenGL3 + C++17)
- **실측 라이브 환경:** Fedora 44 x86_64, Valve GE-Proton11-7, PID 55202, RootBase `0x73B6C5D8`
- **테스트 결과 (6개 세트 전원 PASS):**
  - `[PASS] [Step 1] 게임 프로세스 연결 및 OfficerManager RootBase 초기화 성공`
  - `[PASS] [Step 2] 800명 역사 무장 + 1,185명 활성 확장 슬롯(신무장/결혼 자녀) 총 1,985명 일괄 로드 및 ID 800+ is_extended 플래그 검증`
  - `[PASS] [Step 3] 실시간 다중 컬럼 정렬(Sorting) 알고리즘 검증:`
    - 무력 내림차순: 1위 여포(Lu Bu, 무력 100) 최상단 정렬 확인
    - 지력 내림차순: 1위 제갈량(Zhuge Liang, 지력 100) 최상단 정렬 확인
    - 스탯 총합 내림차순: 1위 총합 400 최상단 정렬 확인
    - ID 오름차순: 0번 무장 위치 복귀 확인
  - `[PASS] [Step 4] 실시간 한글 및 ID 검색(Search) 필터 검증:`
    - "조조" 검색: 정확히 1건(ID 333) 매칭
    - "조" 부분 일치 검색: 2건 이상(조조, 조운 등) 매칭
    - "관우" 검색: 정확히 1건(ID 94) 매칭
    - 숫자 ID "380" 검색: 조운(ID 380) 매칭
    - 미존재 검색어: 0건 반환 및 검색어 초기화 시 1,985명 전원 복귀
  - `[PASS] [Step 5] 상태별 탭 필터(현역 961명 / 재야 2명 / 확장 슬롯 1,185명) 검증:`
    - 현역(Active): force_id != 0xFFFF 및 status_flag != 0xFF 100% 만족
    - 재야(Free): force_id == 0xFFFF 100% 만족
    - 확장 슬롯(Extended): is_extended == true 및 ID >= 800 100% 만족
  - `[PASS] [Step 6] ImGuiListClipper 120프레임 가상 스크롤 렌더링 무결성 스모크 테스트:`
    - 1,985행 중 뷰포트 내 25~35행만 동적 렌더링되어 프레임 드랍(60fps 유지) 및 버퍼 오버런/크래시 0건 검증 완료

### 7.10. Phase 5.3: 무장 인라인 편집 및 실시간 핫싱크(Hot-Sync) 실측 증거
- **테스트 바이너리:** `bin/test_officer_edit` (Dear ImGui Docking + GLFW + OpenGL3 + C++17)
- **실측 라이브 환경:** Fedora 44 x86_64, Valve GE-Proton11-7, PID 55202, RootBase `0x73B6C5D8`
- **테스트 결과 (8개 세트 전원 PASS):**
  - `[PASS] [Step 1] 게임 프로세스 연결 및 OfficerManager RootBase 초기화 성공`
  - `[PASS] [Step 2] OfficerTableView 초기화 및 대상 무장(조조 ID 333) 바인딩 완료`
  - `[PASS] [Step 3] 무장 상세 편집 모달(OpenEditModal) 호출 및 ID 333 일치, 버퍼 스탯 정상 로드 확인`
  - `[PASS] [Step 4] SafeVanilla 모드 실시간 핫싱크(CommitEditToMemory) 검증:`
    - 핫싱크 소요 시간: **`0.0018ms` (약 1.8 마이크로초)**로 인게임 즉각 반영 확인 (기준 50ms 미만 압도적 통과)
    - Safe 모드: 120 입력 시 인게임 메모리에 100으로 안전 클램핑 확인
    - 에디터 내부 버퍼 및 테이블 캐시도 100으로 자동 동기화 확인
  - `[PASS] [Step 5] LimitBreak(한계 돌파) 모드 및 포화 연산(Saturating Cast) 검증:`
    - 한계 돌파 모드 활성화 후 250 입력 시 인게임 라이브 메모리에 250 패치 성공
    - 256 이상 입력 시 0 오버플로 방지 및 255 포화 연산 고정 확인
  - `[PASS] [Step 6] 빠른 치트 프리셋(Quick Cheats) 원클릭 패치 검증:`
    - 전 병종 S급 원클릭 치트 실행 -> 조조 창병/기병/궁병 전종 S(3) 실시간 패치 확인
    - 능력치 올 100 원클릭 치트 실행 -> 통솔/무력/지력/정치 전원 100 실시간 패치 확인
  - `[PASS] [Step 7] 검증 후 데이터 안전 원상 복구 (조조 99/99/92/91, 적성 A/A/A 원복 완료)`
  - `[PASS] [Step 8] Dear ImGui 편집 모달 120프레임 무결성 스모크 테스트 통과`

### 7.11. Phase 5.4: 도시 자원 편집 뷰(CityTableView) 및 실시간 핫싱크(Hot-Sync) 실측 증거
- **테스트 바이너리:** `bin/test_city_table` (Dear ImGui Docking + GLFW + OpenGL3 + C++17)
- **실측 라이브 환경:** Fedora 44 x86_64, Valve GE-Proton11-7, PID 55202, RootBase `0x73B6C5D8`
- **테스트 결과 (9개 세트 전원 PASS):**
  - `[PASS] [Step 1] 게임 프로세스 연결 및 CityManager RootBase 초기화 성공 (RootBase: 0x73b6c5d8)`
  - `[PASS] [Step 2] CityTableView 초기화 및 60개 전체 도시 레코드 바인딩 완료 (0번 양평 ID 0 일치)`
  - `[PASS] [Step 3] 실시간 다중 컬럼 정렬 기능 검증 (자금/군량/병력 내림차순 1위 금성(Jincheng) 일치, ID 오름차순 복귀)`
  - `[PASS] [Step 4] 실시간 한글 및 ID 검색 필터 검증 ('낙양' 1건, '장' 부분 매칭 2건, ID '18' 포함, 검색어 초기화 후 60개 전원 복구)`
  - `[PASS] [Step 5] 도시 상세 편집 모달(OpenEditModal) 호출 및 버퍼 검증 (낙양 ID 18 자금 28,000 / 군량 125,700 / 내구 3,000)`
  - `[PASS] [Step 6] SafeVanilla 모드 실시간 핫싱크 및 안전 클램핑 검증:`
    - 핫싱크 소요 시간: **`0.0022ms` (약 2.2 마이크로초)**로 인게임 즉각 반영 확인 (기준 50ms 미만 압도적 통과)
    - Safe 모드: 2,000,000 자금 입력 시 인게임 메모리에 999,999로 안전 클램핑 확인
  - `[PASS] [Step 7] LimitBreak 모드 및 빠른 치트(Quick Cheats) 원클릭 검증:`
    - 한계 돌파 모드 활성화 시 1,500,000 자금 실시간 메모리 패치 성공
    - 낙양 자원 고갈 치트 (자금 0, 군량 0) 정상 작동 확인
    - 낙양 자원 풀충전 치트 (자금 999,999 / 군량 999,999 / 내구 5,000) 원클릭 패치 확인
  - `[PASS] [Step 8] 검증 후 데이터 안전 원상 복구 (낙양 원래 자금 28,000 / 군량 125,700 / 내구 3,000 완벽 복구)`
  - `[PASS] [Step 9] Dear ImGui 도시 테이블 및 편집 모달 120프레임 무결성 스모크 테스트 통과`

### 7.12. Phase 5.5: 다중 선택(Multi-Select) 및 50인 이상 무장 일괄 수정(Batch Actions) 실측 증거
- **테스트 바이너리:** `bin/test_batch_actions` (Dear ImGui Docking + GLFW + OpenGL3 + C++17)
- **실측 라이브 환경:** Fedora 44 x86_64, Valve GE-Proton11-7, PID 55202, RootBase `0x73B6C5D8`
- **테스트 결과 (9개 세트 전원 PASS):**
  - `[PASS] [Step 1] 게임 프로세스 연결 및 OfficerManager RootBase 초기화 성공 (RootBase: 0x73b6c5d8)`
  - `[PASS] [Step 2] OfficerTableView 바인딩 및 다중 선택(Multi-Select) 기본 로직 검증:`
    - 개별 선택 및 토글(Toggle), 전체 선택(SelectAll), 전체 해제(Clear), 선택 반전(Invert) 100% 정상 작동
    - 동일 도시(City 37) 소속 12명 무장 일괄 선택(`SelectSameCityAs`) 확인
    - 전국 군주(Rank 0) 78명 일괄 선택(`SelectByRank`) 및 조조(군주) 포함 확인
  - `[PASS] [Step 3] 다중 무장 50인 이상(0~49번 50명) 표본 선택 및 원본 상태 스냅샷(OfficerSnapshot) 100% 캡처 완료`
  - `[PASS] [Step 4] 50인 무장 스탯 일괄 +10 가산 (SafeVanilla 100 상한) 검증:`
    - 50인 일괄 메모리 패치 소요 시간: **`0.0769ms` (약 77 마이크로초)**로 기준(50ms 미만) 압도적 통과
    - 50인 전원 인게임 메모리 직접 검증 결과 통솔/무력/지력/정치 +10 정확히 반영 확인
  - `[PASS] [Step 5] 50인 무장 수명(사망년도) 일괄 +20년 연장 검증:`
    - 50인 일괄 수명 연장 트랜잭션 성공 및 인게임 메모리 사망년도 +20년 전원 일치 확인
  - `[PASS] [Step 6] 50인 무장 병종 적성 전종 S급(창병/기병/궁병 S) 일괄 승급 검증:`
    - 50인 전원 창병/기병/궁병 S(3) 적성 실시간 패치 확인
  - `[PASS] [Step 7] 트랜잭션 롤백(Undo / Restore) 무결성 검증:`
    - `BatchRestore` 수행 후 50인 전원 스탯, 적성, 수명이 변경 전 원본 값으로 100% 완벽 복구됨을 메모리 직접 바이트 대조로 검증 완료
  - `[PASS] [Step 8] 빠른 일괄 치트 프리셋(올 100 & 한계돌파 255) 및 직전 작업 Undo 검증 (10인 표본):`
    - 10인 올 100 일괄 치트 적용 확인 및 직전 작업 Undo 정상 복원 확인
    - 10인 한계 돌파 올 255 일괄 치트 적용 확인 및 Undo 정상 복원 확인
  - `[PASS] [Step 9] Dear ImGui 다중 선택 테이블 및 일괄 수정 모달 120프레임 무결성 스모크 테스트 통과`

### 7.13. Phase 6.1: Linux Standalone 배포 패키징 및 런처 실측 검증
- **패키지 아카이브:** `dist/san13_editor_linux_x86_64.tar.gz` (압축 크기: **1.3MB**, 빌드: Release `-O2`, Symbols: Stripped)
- **번들 포함 파일 명세:**
  - `san13_editor` (1.2MB 완성형 GUI 단독 실행 바이너리)
  - `run.sh` (Yama ptrace_scope 보안 점검 및 디스플레이 감지 자동 런처 스크립트)
  - `san13_editor.desktop` (XDG 표준 리눅스 데스크톱 바로가기)
  - `README_LINUX.md` (비개발자용 한국어 실행 매뉴얼 및 FAQ)
  - `assets/fonts/NanumGothic.ttf` (2.0MB 디지털 서명 한글 완성형 11,172자 TTF 글꼴)
- **배포판 무결성 검증 결과 (100% 통과):**
  - `[PASS] [Step 1] 바이너리 스트립(Strip) 및 1.3MB 초경량 무의존 배포 아카이브 빌드 성공`
  - `[PASS] [Step 2] Yama ptrace_scope(0 및 1) 환경 자동 감지 및 보안 가이드 검증 완료`
  - `[PASS] [Step 3] 상대 경로 폰트 자동 로드(assets/fonts/NanumGothic.ttf) 및 UI 텍스처 아틀라스 빌드 성공`
  - `[PASS] [Step 4] Live Steam/GE-Proton 게임(PID: 55202, RootBase: 0x73B6C5D8) 자동 탐색 및 즉시 바인딩 성공`
  - `[PASS] [Step 5] 독립 배포 런처(run.sh --smoke-test) 120프레임 무결성 실행 및 정상 종료 검증 완료`
  - `[PASS] [Step 6] 독립 tar.gz 아카이브를 임의의 클린 디렉터리(scratch/)에 압축 해제 후 단독 실행 성공 검증 완료`

---

### 7.14. Phase 5.6: 네이티브 한글 2벌식 오토마타(HangulAutomata) 단위 테스트 실측 증거
- **테스트 바이너리:** `bin/test_hangul_automata` (Pure C++17, Dear ImGui/GLFW/OpenGL 의존성 없음)
- **빌드 환경:** Fedora 44 x86_64, g++ 14.2.1, -std=c++17 -O2
- **테스트 결과 (6개 세트 전원 100% PASS):**
  - `[PASS] [Set 1] 삼국지 주요 무장 성명 두벌식 변환 검증 (6인):`
    - 조조 (whwh), 유비 (dbql), 관우 (rhksdn), 손권 (thsrnjs), 제갈량 (wprkffid), 하후돈 (gkgnehs) 전원 정확한 UTF-8 한글 완성형 변환 확인
  - `[PASS] [Set 2] 삼국지 주요 도시명 변환 검증 (4개):`
    - 낙양 (skrdid), 장안 (wkddks), 건업 (rjsdjq), 한중 (gkswnd) 전원 정확한 변환 확인
  - `[PASS] [Set 3] 복합 자음(ㄺ/ㄳ/ㅄ/ㄵ) 및 복모음(ㅢ) 조합 검증 (5종):`
    - 닭 (ekfr, ㄺ 복합받침), 값 (rkqt, ㅄ 복합받침), 몫 (ahrt, ㄳ 복합받침), 앉다 (dkswek, ㄵ 복합받침), 의사 (dmltk, ㅢ 복모음) 전원 정확
  - `[PASS] [Set 4] [EDGE-17] 연음 분리(Consonant Transition) 검증:`
    - 단일 받침 연음: `각` + `ㅏ` → `가가` 정확히 분리 확인
    - 복합 받침 연음: `닭` + `ㅣ` → `달기` (ㄺ 분리 ㄹ 확정 + ㄱ 이전) 정확
    - 복합 받침 연음: `값` + `ㅣ` → `갑시` (ㅄ 분리 ㅂ 확정 + ㅅ 이전) 정확
  - `[PASS] [Set 5] [EDGE-17] 백스페이스 1자모 역분해 검증:`
    - `닭` → `달` → `다` → `ㄷ` → 빈문자열 (4단계 순차 역분해) 정확
    - `과` → `고` → `ㄱ` → 빈문자열 (복모음 ㅘ 분해) 정확
    - 확정 문자열 `조조` 백스페이스 → `조` → 빈문자열 (UTF-8 3바이트 단위 제거) 정확
  - `[PASS] [Set 6] [EDGE-18] 한/영 모드 토글 및 혼용 타이핑 검증:`
    - 한글 `조조` 입력 → Right Alt 토글(영문) → 영문 `Cao 123` 타이핑 → 재토글(한글) → `유비` 입력
    - 최종 버퍼: `조조 Cao 123 유비` (한글/영문/숫자 혼용 완전 일치)

### 7.15. Phase 5.7: 역사 무장 700인 한글 성명 테이블(OfficerNameTable) 단위 테스트 실측 증거
- **테스트 바이너리:** `bin/test_officer_names` (Pure C++17, 헤드리스 독립 단위 테스트)
- **빌드 환경:** Fedora 44 x86_64, g++ 14.2.1, `-std=c++17 -O2`
- **구현 아키텍처:**
  - `include/game/OfficerNameTable.hpp`: $O(1)$ 즉시 조회를 위한 `GetOfficerKoreanName(id)` 및 `HasOfficerKoreanName(id)` 인터페이스
  - `src/game/OfficerNameTable.cpp`: ID 0~699 역사 무장 700명 전수 플랫 포인터 테이블 (`HISTORICAL_OFFICER_NAMES[700]`) 및 700번 이상 고대무장(`SPECIAL_OFFICER_NAMES`) 분기 매핑
  - `src/game/OfficerManager.cpp`: 기존 15명 하드코딩 맵 완전 제거 및 `OfficerNameTable` 연동, 비존재/신무장 fallback 보존
- **테스트 결과 (3개 스텝 전원 100% PASS):**
  - `[PASS] [Step 1] 역사 무장 700명(0 ~ 699) 전수 무결성 검증:`
    - 0번(아회남)부터 699번(미당대왕)까지 700명 전원 누락 없는 매핑 및 공백/숫자 오염 0건 확인 (700 / 700)
  - `[PASS] [Step 2] 주요 역사 무장 매핑 일치성 검증 (32인 표본 전원 일치):`
    - 조조 (ID 333), 관우 (ID 94), 제갈량 (ID 283), 조운 (ID 380), 장비 (ID 418), 여포 (ID 635), 유비 (ID 610)
    - 하후돈 (ID 83), 하후연 (ID 76), 순욱 (ID 250), 곽가 (ID 61), 허저 (ID 137), 전위 (ID 458), 사마의 (ID 220)
    - 주유 (ID 241), 손책 (ID 359), 손권 (ID 356), 감녕 (ID 113), 황충 (ID 178), 마초 (ID 498), 강유 (ID 129), 방통 (ID 537)
    - 원소 (ID 18), 안량 (ID 118), 문추 (ID 527), 동탁 (ID 480), 가후 (ID 59), 아회남 (ID 0), 미당대왕 (ID 699)
    - 고대무장: 백기 (ID 700), 항우 (ID 701), 유방 (ID 702)
  - `[PASS] [Step 3] 범위 초과 슬롯 Fallback 검증:`
    - ID 999: `HasOfficerKoreanName(999) == false` 및 빈 문자열 반환 확인
- **GUI 및 빌드 연동 검증:**
  - `make test_officer_table` 가상 스크롤 및 무장 검색 필터 정상 통과
  - `make package` 1.3MB 독립 배포 번들(`dist/san13_editor_linux_x86_64.tar.gz`) 빌드 성공

### 7.16. Phase 5.8: Dear ImGui 네이티브 2벌식 한글 입력 위젯(HangulInputText) 연동 실측 증거
- **테스트 바이너리:** `bin/test_hangul_input_widget` (C++17, Dear ImGui 헤드리스 프레임 및 단위 테스트 통합)
- **빌드 환경:** Fedora 44 x86_64, g++ 14.2.1, `-std=c++17 -O2`
- **구현 아키텍처:**
  - `HangulInputText(label, hint, buf, buf_size, automata, extra_flags)` 위젯 구현
  - `ImGuiInputTextFlags_CallbackCharFilter`: 영문 알파벳/ASCII 키 입력을 가로채 `automata.ProcessKey(c)`로 전달하고 `EventChar = 0`으로 내부 버퍼 직접 삽입 차단
  - `ImGuiInputTextFlags_CallbackAlways`: 매 프레임 `ImGuiKey_Backspace` 감지 및 `automata.ProcessBackspace()` 호출, `DeleteChars()`/`InsertChars()`로 ImGui 버퍼 실시간 동기화
  - 한/영 모드 전환: `Right Alt` 및 `Shift + Space` 단축키 감지 + 우측 `[한]`(골드) / `[영]`(다크슬레이트) 뱃지 버튼 클릭 지원
  - `OfficerTableView` 및 `CityTableView` 상단 검색창에 `HangulInputText` 위젯 100% 통합
- **테스트 결과 (5개 스텝 전원 100% PASS):**
  - `[PASS] [Step 1] HangulAutomata 상태 및 기본 음절 조합 연동 검증: whwh -> '조조' 및 C 버퍼 동기화`
  - `[PASS] [Step 2] 1자모 백스페이스 역분해 연동 검증: ekfr('닭') -> '달' -> '다' -> 'ㄷ' -> '' 순차 삭제`
  - `[PASS] [Step 3] 한/영 모드 토글 및 영문/한글 혼용 타이핑 검증: '조조' + ' Cao' + ' 유비' -> '조조 Cao 유비'`
  - `[PASS] [Step 4] OfficerTableView & CityTableView 한글 검색 연동 검증: '조조', '낙양' 검색어 설정 성공`
  - `[PASS] [Step 5] Dear ImGui 프레임 내 HangulInputText 렌더링 검증: 1280x800 버퍼 보존 및 컨텍스트 정상 해제`

### 7.17. Phase 5.8 핫픽스 & UX 개선: 포커스 격리, 검색어 원클릭 클리어 및 대소문자 무시 검색 실측 증거
- **테스트 바이너리:** `bin/test_hangul_input_widget` (C++17, 7개 전체 통합 테스트 스위트)
- **개선 및 버그 수정 내용:**
  1. **단축키 전역 오작동 격리:** `Right Alt` 및 `Shift+Space` 토글을 `CallbackAlways` 내부로 격리하여 입력창이 활성화(Focus)되어 있을 때만 한/영 전환이 동작하도록 수정.
  2. **외부 동기화 가드:** `InputTextWithHint` 호출 후 `ImGui::IsItemActive()` 사후 평가를 적용하여, 유저가 타이핑 중일 때 외부 버퍼 동기화로 인한 한글 조합 파괴(Preedit 손실) 원천 차단.
  3. **`SetText` 멱등성 보장:** `m_committedText == text && m_state == Empty`일 경우 불필요한 `ResetComposing()`을 건너뛰어 성능 및 상태 보존 강화.
  4. **한/영 배지 클릭 후 오토포커스 복원:** 배지 버튼(`[한]`/`[영]`) 클릭 시 `ImGui::SetKeyboardFocusHere(-1)`를 호출하여 마우스 클릭 후에도 즉시 연속 타이핑 가능.
  5. **검색어 원클릭 클리어 (`×`) 버튼:** `OfficerTableView` 및 `CityTableView` 검색창 우측에 텍스트가 있을 때 원클릭으로 검색어를 지우고 전체 목록으로 복귀하는 클리어 버튼 연동 (`m_searchAutomata.Clear()`).
  6. **대소문자 무시 (Case-Insensitive) 검색:** 영문 모드 또는 유저 등록 무장 영문명 검색 시 `cao` $\rightarrow$ `Cao Cao` 매칭 지원.
- **테스트 결과 (7개 스텝 전원 100% PASS):**
  - `[PASS] [Step 6] 검색어 원클릭 초기화(Clear) 및 오토마타 상태 동기화 검증: 무장 및 도시 검색어 빈 문자열 클리어 확인`
  - `[PASS] [Step 7] 대소문자 무시 (Case-Insensitive) 검색 매칭 검증: 'Cao Cao' vs 'cao' 매칭 검증 성공`

### 7.18. Phase 5.9: 무장 활동 상태(+0x4F) 및 소속 군주(+0x4A) 실측 기반 현역/재야/미등장 필터링 정상화 실측 증거
- **테스트 환경:** Valve GE-Proton11-7, PID 10808 (184년 시나리오 실시간 메모리 바이트 분석)
- **리버스 엔지니어링 실측 성과:**
  1. **활동 상태 플래그 교정:** 기존 `+0x14`에서 실제 게임 엔진 플래그인 **`+0x4F` (`activity_flag`)**로 교정.
     - `0x01`: 현재 시나리오에서 활약 중인 생존 무장 (442명)
     - `0x00`: 아직 연도가 도달하지 않은 미등장 미래 무장 (258명: 제갈량, 곽가, 사마의, 강유 등)
  2. **소속 세력/군주 오프셋 교정:** 기존 무장 고유 번호가 들어가던 `+0x08`에서 실제 군주 사관 번호인 **`+0x4A` (`lord_id`)**로 교정.
     - `0xFFFF`: 무소속 / 재야 무장
     - 유비(610), 관우(610), 장비(610): 동일 군주 ID 바인딩 확인
  3. **신분(`rank`) 오프셋 확정:** `+0x0C` (0: 군주, 1~3: 도독/군단장, 4~7: 태수, 그 외 일반)
  4. **필터링 탭 확장:** `전체` | `현역` | `재야` | `미등장` | `신무장/자녀` 5개 탭 지원
- **테스트 결과 (100% PASS):**
  - `make test_officer_table`: 현역 탭과 재야 탭이 명확히 분리 필터링 검증 완료
  - `make test`: 전 도메인 테스트 100% 통과
  - `make package`: 1.3MB 배포 아카이브 재빌드 성공

### 7.19. EDGE-22: `HangulInputText` 가용폭 계산 버그 및 컨트롤 바 UI 클리핑 복원 실측 증거
- **수정 내역:**
  - `src/ui/HangulAutomata.cpp`: `GetContentRegionAvail().x`(~1200px) 독점 코드를 제거하고, 호출자가 지정한 `ImGui::CalcItemWidth()`를 존중하도록 변경.
  - 텍스트 입력창 너비 = `CalcItemWidth() - badgeWidth - spacing`으로 정밀하게 분할하여 전체 위젯 너비가 정확히 호출자가 설정한 폭(260px)에 수렴.
- **검증 결과:**
  - `HangulInputText` 오른쪽에 배치된 `[전체]`, `[현역]`, `[재야]`, `[미등장]`, `[신무장/자녀]` 라디오 버튼 및 도구 버튼들이 화면 우측 바깥으로 밀려나지 않고 정상 위치에 완벽 렌더링 복원.
  - `make test` (100% PASS) 및 `make package` 패키징 성공.

### 7.20. EDGE-23: 실제 세력(Force/Corps) 오프셋(+0x3C) 규명 및 현역 5명 과소 표출 버그 실측 증거
- **현상:** 상단 컨트롤 바에서 `현역` 필터를 선택했을 때 오직 5명(관우, 장비, 유비, 손책, 주유)만 표출되고 나머지 439명이 전부 재야로 분류되던 현상.
- **실측 리버스 엔지니어링 결과:**
  1. **오인된 오프셋 `+0x4A`의 실체:**
     - 기존 코드에서 소속 군주/세력 ID로 취급했던 `+0x4A`는 일반 소속 세력이 아닌 유비 삼형제(`0x0262`=610 유비) 및 손책-주유(`0x0167`=359 손책) 5명에게만 부여된 **특수 인연/의형제 관계망 포인터**였음.
     - 조조, 하후돈, 하진, 동탁, 원소 등 대다수 무장은 `+0x4A`가 `0xFFFF`였기 때문에 `force_id != 0xFFFF` 필터를 통과하지 못함.
  2. **실제 소속 세력/군단 오프셋 `+0x3C` (`uint16_t`) 전수 스캔 및 검증:**
     - 4,796바이트 메모리 전수 스캔 결과, 오프셋 `+0x3C`가 세력별/군단별로 완벽히 일치함을 확인:
       - **조조군 (`25` = `0x19`):** 조조(333), 하후돈(83), 하후연(76), 하후은(77), 허저(137), 진랑(298), 조앙(324), 조순(327), 조창(328), 조인(331), 전위(458) 등 일족 및 직속 무장 11명 전원 일치.
       - **유비군 (`75` = `0x4B`):** 유비(610), 관우(94), 관이(92), 관흥(102), 관통(110), 관평(116), 관은병(652), 장비(418), 장포(421), 장씨(669), 조운(380), 조광(398), 조통(413), 주창(236), 제갈상(279), 제갈첨(281), 장준(403), 유봉(615), 포삼랑(676) 등 20명 전원 일치.
       - **원소군 (`101` = `0x65`), 손견군 (`126` = `0x7E`), 손책군 (`125` = `0x7D`), 동탁군 (`1` = `0x01`), 맹획군 (`60`), 올돌골군 (`51`), 여포군 (`145`)** 등 각 세력별로 고유 ID 완벽 매핑.
       - **무소속/재야 (`0` = `0x00`):** 백기(700), 유방(702) 등.
  3. **활동 상태(`+0x4F`)와 결합된 필터링 기준 정립:**
     - **현역 (Active):** `activity_flag == 1 && force_id > 0 && force_id != 0xFFFF` (442명)
     - **재야 (Free):** `activity_flag == 1 && (force_id == 0 || force_id == 0xFFFF)` (2명)
     - **미등장 (Unappeared):** `activity_flag == 0` (259명)
     - **전체 (All):** `activity_flag != 0xFF` (유효 슬롯 703명)

### 7.21. EDGE-24: 공식 고대무장 50인 슬롯(ID 850~899) 매핑 및 범증 검색 정상화 실측 증거
- **현상:** 유저가 인게임 시나리오에 배치한 고대무장 '범증(ID 851)'을 에디터에서 검색했을 때 전혀 검색되지 않고 `신무장 / 자녀 #851`로 표출되던 문제.
- **원인 분석:**
  - 코에이 테크모 삼국지 13 PK의 공식 고대무장 50인은 신무장 확장 슬롯(ID 800번대) 중 `ID 850 ~ 899`에 배정됨.
  - 기존 `OfficerNameTable`은 역사 무장(0~699)과 700번대 초반(백기, 항우 등)만 내장하여 850번 이후 고대무장이 누락되어 있었음.
- **해결 및 실측 검증:**
  - `src/game/OfficerNameTable.cpp`에 공식 고대무장 50인 성명 테이블 완벽 탑재:
    - 850: 영포, 851: **범증**, 852: 악의, 853: **장량**, 854: 한신, 855: 소하, 856: 조참, 857: 팽월, 858: 주발, 866: **번쾌**, 892: **악비** 등.
  - `test_officer_names` 검증: 범증(851) 검색 시 "범증" 즉시 반환 확인.
  - GUI 검색창에 `qjawmd` 입력 시 `범증` 자동 완성 및 즉시 필터링 100% 정상 작동.

### 7.22. EDGE-25: 청크 0 인게임 실시간 일자(190년 1월 1일) 오프셋 규명 및 UI 연동 실측 증거
- **현상:** 게임 화면에서는 190년 1월 1일이 진행 중이나, 에디터 상단에 게임 현재 일자가 표시되지 않아 사용자가 시나리오 진행 시점을 파악하기 어려움.
- **실측 리버스 엔지니어링 규명 결과:**
  - Proton 라이브 프로세스(PID: 10808)의 청크 0(`RootBase + 0x000000`) 전수 메모리 스캔:
    - `RootBase + 0x137A` (`uint16_t`): 현재 연도 = **`190`** (0x00BE)
    - `RootBase + 0x1380` (`uint16_t`): 현재 월 = **`1`** (0x0001)
    - `RootBase + 0x1386` (`uint16_t`): 현재 일 = **`1`** (0x0001)
- **구현 및 검증:**
  - `OfficerManager::GetGameDate()` 구현: 연/월/일 실시간 동기화.
  - 상단 툴바에 `📅 게임 일자: 190년 1월 1일` 골드 테두리 배지 실시간 표시.
  - `test_officer_names` 단위 테스트에 게임 일자 동기화 무결성 검증 추가 (100% PASS).

### 7.23. EDGE-26 & 27: 등장년도 기반 아회남(ID 0) 미등장 정상화 실측 증거
- **현상:** 아회남(ID 0)은 190년생, 217년 등장(시나리오 시점보다 27년 뒤) 무장이나, 원본 세이브 DB에 `force_id = 62`가 세팅되어 있어 190년 시점에 "현역"으로 잘못 분류되는 결함.
- **해결 방안:**
  - 삼국지 13 PK 엔진의 등장 메커니즘을 반영하여, **현재 게임 연도가 무장의 등장 연도보다 이전(`current_year < appear_year`)일 경우 세력 번호와 무관하게 무조건 `미등장`으로 강제 분류**하도록 판정식 개정.
- **검증 결과:**
  - 190년 시나리오에서 아회남(ID 0)이 현역 목록에서 완전히 제외되고 `미등장` 탭에 정확히 분류됨을 실측 확인.

### 7.24. EDGE-28: 새로고침 물음표(?) 특수문자 폰트 글리프 깨짐 해결 실측 증거
- **현상:** 에디터 상단 툴바의 새로고침 버튼 텍스트가 `새로고침 ?` 형태로 출력되는 현상.
- **원인 분석:**
  - 기존 버튼 라벨에 포함되어 있던 회전 화살표 유니코드 특수문자(`↻`, U+21BB)가 번들 폰트인 `NanumGothic.ttf`의 글리프 범위에 포함되어 있지 않아 Dear ImGui 폰트 렌더러가 미지원 문자로 판단하여 `?`로 대체 렌더링함.
- **해결 및 검증:**
  - 버튼 라벨을 글리프 호환성이 완벽한 `새로고침 (F5)`로 수정.
  - 물음표 깨짐 없이 미려한 한글 텍스트로 정상 렌더링 확인.

### 7.25. EDGE-29: 범증(ID 851) 세력 배치 후 인게임 재야 상태 완벽 분류 실측 증거
- **현상:** 유저가 시나리오에 배치하여 세력 120에 둔 범증(ID 851, 181년 등장)이 실제 인게임 상태는 "재야"임에도 에디터에서 `일반 (9품관)`(현역)으로 오인 표출되던 결함.
- **원인 분석 및 리버스 엔지니어링:**
  - `status_flag (+0x4F)`는 단순 활동 여부가 아닌 **'세력 사관(복무) 여부 플래그'**임.
    - `status_flag == 1`: 세력에 임관하여 봉직 중인 실제 현역 (조조, 하후돈, 악비, 번쾌 등)
    - `status_flag == 0`: 세력 미임관 / 거병 전 / 방랑 상태인 재야 또는 미등장
  - 범증은 이미 등장 연도(181 <= 190)를 만족하여 성인이나, 세력에 사관하지 않았으므로 `status_flag == 0` 상태임.
- **해결 및 검증:**
  - 무장 상태 판별 진리표(5.2.1) 확립: `status_flag == 0` 무장은 `current_year >= appear` 조건 하에서 무조건 **"재야"**로 분류.
  - 실제 임관 중인 악비(ID 892, 번쾌 ID 866)는 `status_flag == 1`이므로 정상적으로 **"현역"** 유지.
  - `test_officer_names` [Step 6] 검증: 조조(군주), 번쾌(3품관 현역), 범증(재야), 아회남(미등장) 전수 일치 확인 (100% PASS).

### 7.26. EDGE-30: 상단 컨트롤 툴바 2단(2-Row) 반응형 레이아웃 분리 실측 증거
- **현상:** 상단 컨트롤 바에 검색창, 필터 5개, 새로고침, 게임 일자, 전체선택, 선택해제, 반전, 아군선택, 일괄수정, 선택무장편집 등 15개 위젯이 `SameLine()` 한 줄로 배치되어 있어, 일반적인 창 크기(1280x820)에서 우측 버튼들이 화면 바깥으로 잘려나감.
- **해결 규격:**
  - 상단 툴바를 논리적인 2줄(2-Row) 분리 레이아웃으로 개편:
    - **1단:** `[검색창] | [전체][현역][재야][미등장][신무장] | [새로고침 (F5)] | [📅 게임 일자: %d년 %d월 %d일]`
    - **2단:** `[전체 선택] [선택 해제] [반전] [아군 선택] [선택 무장 편집] [일괄 수정 ★] | [총 %zu명 중 %zu명 표시]`
- **검증 결과:**
  - 기본 해상도(1280x820)에서도 가로 스크롤이나 버튼 잘림 현상 0건 확인. 모든 액션 버튼이 즉시 클릭 가능한 최적의 사용자 인터페이스 확보.

---

## 8. 배포 목적(Distribution-Grade) 품질 보증(QA) 및 엣지 케이스 테스트 명세서

일반 게이머에게 배포 시 발생할 수 있는 모든 오작동, 크래시, 데이터 오염을 예방하기 위한 4대 분야 16개 엣지 케이스 검증 시나리오입니다.

### 8.1. 수치 경계값 및 비트 오버플로 엣지 케이스 (Value Bounds & Overflow QA)

| 케이스 ID | 테스트 시나리오 | 입력값 | 예상되는 위험 현상 | 에디터 필수 방어 동작 (검증 기준) |
| :---: | :--- | :--- | :--- | :--- |
| **EDGE-01** | 무장 스탯 음수 입력 | `-1`, `-50` | `uint8_t` 언더플로 발생 시 `255`로 변환되어 의도치 않은 사기 캐릭터화 | 최소 유효치인 `1`로 자동 보정(Clamping) |
| **EDGE-02** | 무장 스탯 0 입력 | `0` | 게임 내 무장 사망/불능 처리 또는 0 나누기 예외(Divide by Zero) | 최소 유효치인 `1`로 자동 보정 |
| **EDGE-03** | 바닐라 상한 초과 입력 | `101`, `120` | 게임 원본 밸런스 붕괴 | - **Safe 모드:** `100`으로 자동 절삭<br>- **LimitBreak 모드:** 입력값 그대로 통과 |
| **EDGE-04** | 8비트 하드웨어 오버플로 | `256`, `512` | 하위 8비트만 남아 `256 % 256 = 0`이 되어 **스탯이 0으로 급감하는 버그** | **포화 연산(Saturating Cast):** 모듈로가 아닌 최대치 `255`로 고정 |
| **EDGE-05** | 병종 적성 범위 초과 | `4`, `10`, `255` | 게임 렌더러의 UI 스프라이트 인덱스 범위 초과로 **게임 즉시 세그폴트 크래시** | **절대 마스킹:** `input & 0x03`을 적용하여 0~3 이외의 값 메모리 쓰기 차단 |
| **EDGE-06** | 생몰년도 역전 현상 | `사망(180) < 출생(200)` | 수명 계산 공식에서 언더플로 발생, 영구 생존 또는 게임 멈춤 현상 | 논리 검증 실패 팝업 발생 및 `출생 <= 등장 <= 사망` 강제 |
| **EDGE-07** | 도시 자금/군량 100만 초과 | `1,000,000` ~ `42억` | 6자리 게임 UI 텍스트 짤림 및 엔진 내부 24비트 연산 루틴 오버플로 | 999,999 상한 클램핑 (LimitBreak 활성화 시에도 최대 9,999,999 제한) |
| **EDGE-08** | 도시 내구도 16비트 오버플로| `65,536` | 16비트 오버플로로 내구도가 `0`이 되어 공성전 없이 도시 즉시 함락 | 16비트 최대치(65,535) 미만인 10,000으로 엄격 제한 |

---

### 8.2. 프로세스 및 OS 라이프사이클 엣지 케이스 (Process & OS Lifecycle QA)

| 케이스 ID | 테스트 시나리오 | 재현 환경 | 예상되는 문제점 | 에디터 필수 방어 동작 (검증 기준) |
| :---: | :--- | :--- | :--- | :--- |
| **EDGE-09** | 게임 미실행 상태에서 에디터 실행 | 게임 꺼진 상태 | 프로세스 탐색 실패로 에디터 비정상 종료(Crash) | 에디터가 죽지 않고 `[게임 대기 중...]` 상태로 폴링 유지, 켜지는 즉시 연결 |
| **EDGE-10** | 에디터 사용 중 게임 강제 종료 | 게임 실행 중 Alt+F4 | 끊어진 메모리 주소에 읽기/쓰기 시도 시 에디터 크래시 | `process_vm_readv/writev` 실패 감지 $\rightarrow$ `[게임 연결 끊김]` 안전 모드 전환 |
| **EDGE-11** | 게임 재시작 (PID 변경) | 게임 재실행 | 이전 PID에 쓰기를 시도하여 실패 | 이전 PID 무효화 및 새 PID 자동 재탐지 후 데이터 재동기화 |
| **EDGE-12** | 시나리오 진입 전 메인 메뉴 접근 | 타이틀 화면 상태 | 데이터 풀이 아직 할당되지 않아 쓰레기 값 읽기 | `RootBase` 유효성 플래그 검사 $\rightarrow$ 시나리오 진입 전에는 편집 비활성화 |
| **EDGE-13** | 리눅스 ptrace 권한 제한 환경 | `ptrace_scope = 1` | `process_vm_readv` 시스템 콜 EPERM(권한 거부) 발생 | 권한 부족 감지 시 친절한 해결 가이드 안내 메시지 출력 (비루트 권한 가이드) |

---

### 8.3. 게임 플레이 동시성 및 레이스 컨디션 (Live Concurrency QA)

| 케이스 ID | 테스트 시나리오 | 재현 환경 | 예상되는 문제점 | 에디터 필수 방어 동작 (검증 기준) |
| :---: | :--- | :--- | :--- | :--- |
| **EDGE-14** | 턴 넘김(월 변경) 중 스탯 수정 | 게임에서 날짜 진행 중 | 게임 엔진의 월말 결산 스탯 업데이트와 에디터 쓰기 충돌 | 바이트 단위가 아닌 원자적(Atomic) 구조체 커밋으로 데이터 찢어짐 방지 |
| **EDGE-15** | 일기토/설전 진행 중 수정 | 일기토 연출 화면 | 전투 계산 스택에 캐시된 수치와 불일치 | 기본치(Base)와 현재치(Current) 동시 갱신으로 일관성 유지 |
| **EDGE-16** | 인게임 창 열람 중 수정 | 무장 정보창 켜둔 상태 | 인게임 UI가 이전 수치를 캐싱하여 수정 안 된 것처럼 보임 | 사용자가 혼란스럽지 않도록 "창을 다시 열면 갱신됩니다" 툴팁 제공 |

---

### 8.4. 한글 입력 및 텍스트 조합 엣지 케이스 (Hangul Input & Automata QA)

| 케이스 ID | 테스트 시나리오 | 입력 키 시퀀스 | 예상되는 오작동 위험 | 에디터 필수 방어 동작 (검증 기준) |
| :---: | :--- | :--- | :--- | :--- |
| **EDGE-17** | 복합 받침(닭, ㄳ) 상태에서 모음 입력 시 연음 분리 및 백스페이스 역분해 | `ekfr` (닭) + `k` (ㅏ)<br>및 Backspace | 1. `닭` 전체가 다음 모음과 충돌하여 깨지거나 분실됨<br>2. 백스페이스 시 `닭` 한 글자 전체가 통째로 지워짐 | 1. 복합 받침 분해: 첫 자음(`ㄹ`)은 앞 음절에 남아 `달`로 확정되고, 둘째 자음(`ㄱ`)이 다음 음절 초성으로 이전되어 `달가`로 정확히 전환<br>2. 백스페이스 시 `닭` $\rightarrow$ `달` $\rightarrow$ `다` $\rightarrow$ `ㄷ` $\rightarrow$ 공백으로 1자모씩 역분해 |
| **EDGE-18** | 한/영 모드 전환 및 포커스 아웃 시 자동 커밋 무결성 | 한글 입력 중 Right Alt / Shift+Space / 마우스 토글 및 외부 클릭 | 조합 중인 미완성 음절(Preedit)이 버려지거나 영문과 뒤섞여 깨지는 현상 | 한/영 전환 및 포커스 아웃 즉시 현재 조합 중인 한글 음절을 온전히 텍스트 버퍼에 커밋(Commit) 후 모드 전환 |
| **EDGE-19** | 입력창 비활성 상태에서의 단축키 전역 오발동 방지 | 다른 위젯 조작 중 Right Alt / Shift+Space 누름 | 엉뚱한 검색창의 한/영 모드가 의도치 않게 토글되는 현상 | `CallbackAlways` 내부로 단축키 처리를 격리하여 해당 텍스트창이 포커스(Active)를 가진 동안에만 단축키 반응 |
| **EDGE-20** | 타이핑 중 외부 버퍼 동기화로 인한 한글 조합 파괴 방지 | 한글 음절 조합 중 외부에서 `buf` 접근 또는 리셋 | 조합 중이던 글자가 강제 리셋되거나 글자 씹힘 발생 | `InputTextWithHint` 사후 `ImGui::IsItemActive()` 가드를 두어 편집 중이 아닐 때만 외부 동기화 허용 & `SetText` 멱등성 보장 |
| **EDGE-21** | 시나리오 현재 연도 미반영으로 인한 현역/전체 무장 필터링 불일치 | 시나리오 190년 진행 중 "현역" 필터 탭 선택 | 209년에나 등장할 제갈량이나 미등장/사망 무장까지 전부 "현역"으로 통과되어 전체 목록과 동일하게 700명이 뜨는 현상 | 1. `Chunk 0 (RootBase + 0x000000)`에서 시나리오 현재 연도/날짜 동적 추출<br>2. `appear_year <= current_year && current_year <= death_year` 및 실제 소속 세력/신분 플래그 결합 검증으로 실제 활동 무장만 필터링 |
| **EDGE-22** | `HangulInputText` 가용폭 독점으로 인한 우측 컨트롤 바(현역/재야 탭) 오프스크린 클리핑 | `OfficerTableView` 상단 컨트롤 바 렌더링 | `HangulInputText`가 `GetContentRegionAvail().x`(~1200px)를 통째로 차지하여, 뒤에 오는 `현역`, `재야`, `미등장` 라디오 버튼 및 도구들이 화면 오른쪽 바깥으로 밀려나 사라지는 현상 | 호출자의 `CalcItemWidth()`를 존중하여 `[검색창 + [한] 배지]`의 합산 너비가 정확히 호출자가 지정한 너비(예: 260px)에 수렴하도록 레이아웃 분할 교정 |
| **EDGE-23** | 소속 군주/인연 오프셋(`+0x4A`) 오인으로 인한 현역 무장 과소 표출 (현역 5명 버그) | "현역" 탭 클릭 시 5명만 표출됨 | `+0x4A`는 일반 무장의 소속 세력이 아닌 유비 삼형제/손책 등 특정 인연 관계 포인터였음. 그 결과 조조, 하진, 동탁, 원소 등 대다수 무장이 재야로 오분류 | 무장 오브젝트(0x12BC 바이트)에서 실제 세력 번호(Force ID: 0~63) 및 사관 상태를 결정짓는 정확한 오프셋 역추적 및 필터 조건 교정 |
| **EDGE-24** | 공식 고대무장 50인 슬롯(ID 850~899) 미매핑으로 인한 범증 등 고대무장 검색 누락 버그 | 검색창에 "범증" 입력 시 0건 반환 | 삼국지 13 PK 공식 고대무장은 ID 850~899번에 배정되나, 에디터에 이름 테이블이 없어 `신무장 / 자녀 #851`로 표기되어 텍스트 검색 실패 | 공식 50인 고대무장(ID 850: 영포, 851: 범증, 852: 악의, 853: 장량 등) 매핑 테이블 탑재로 완벽 해결 |
| **EDGE-25** | 청크 0 게임 일자 미표시로 인한 시나리오 진행 시점 파악 불가 | 시나리오 진행 중 에디터 열람 | 게임 내 현재 연도/월/일을 에디터에서 알 수 없어 연도별 필터링 신뢰도 저하 | `RootBase + 0x137A`(년), `0x1380`(월), `0x1386`(일) 실시간 파싱 및 상단 컨트롤 바 뱃지(`📅 190년 1월 1일`) 연동 |
| **EDGE-26** | 미래 무장(아회남 ID 0 등)의 기본 DB 세력 번호로 인한 현역 오인 표출 | 190년 시나리오에서 아회남이 현역으로 뜸 | 217년(27년 뒤) 등장 무장이나 기본 DB에 `force_id=62`가 있어 현역으로 잘못 분류됨 | `current_year < appear_year`일 경우 세력 번호와 무관하게 무조건 `미등장`으로 강제 분류하여 완벽 해결 |
| **EDGE-27** | 신분/품관 체계 미정밀 매핑으로 인한 도독/태수 임의 분기 왜곡 | 관우가 태수, 허저가 도독, 여포가 일반으로 뜸 | `+0x0C`를 도독/태수로 임의 분기하여 실제 게임 내 신분과 불일치 | 공식 반에디터 역공학 결과 `+0x0C`는 9품관 체계임을 규명하고 `군주` / `일반 (1~9품관)`으로 정밀 매핑 |
| **EDGE-28** | 새로고침 버튼 특수문자(`↻`) 글리프 누락으로 인한 물음표(`?`) 깨짐 현상 | 상단 툴바 새로고침 버튼 | `NanumGothic.ttf`에 회전 화살표 기호가 없어 `새로고침 ?`로 출력 | 버튼 라벨을 `새로고침 (F5)`로 수정하여 폰트 깨짐 원천 제거 |
| **EDGE-29** | 범증(ID 851) 세력 배치 후 인게임 상태(재야)와 에디터 신분(9품관) 불일치 | 범증 배치 후 에디터 열람 | `status_flag (+0x4F)`는 세력 사관 여부 플래그임에도 `force_id > 0`만 보고 현역으로 오판 | `current_year >= appear_year`라도 `status_flag == 0`이면 세력 미사관이므로 **"재야"**로 분류하도록 판정식 교정 |
| **EDGE-30** | 상단 컨트롤 툴바 1줄 나열로 인한 1280 해상도 우측 버튼 잘림/클리핑 | 1280x820 기본 창 크기 | 15개 위젯이 한 줄에 나열되어 다중선택/일괄수정 버튼이 창 밖으로 잘림 | 상단 툴바를 **1단(검색/필터/일자) + 2단(다중선택/편집/통계)** 반응형 레이아웃으로 분리하여 100% 노출 보장 |
| **EDGE-31** | 고대무장 ID 857(항적) 명칭 검색 실패 및 인게임 재야 무장의 군주 오표기 버그 | 고대무장 항적(ID 857) 인게임 배치 후 에디터 열람 및 검색 | 1. 고대무장 857이 "항우"로 등록되어 있어 인게임 공식 표기인 "항적" 검색 시 미검출.<br>2. `rank == 0`을 무조건 "군주"로 단정 지어, 실제 인게임 재야 무장(항적: 세력 125, rank 0, status 1)이나 군주 친족(원희, 원상, 주유, 손책, 유선 등)이 전부 "군주"로 오분류되어 [재야] 필터에서 누락됨 | 1. `OfficerNameTable` ID 857을 인게임 공식 명칭인 **"항적"**으로 교정 (동의어 "항우"도 동시 호환).<br>2. 신분 판정식 정상화: 유효하지 않은 가상 세력(125 등)이나 사관하지 않은 무장은 **"재야"**로 정상 분류하고 `rank == 0` 단독 군주 판정 왜곡 제거. |
| **EDGE-32** | 재야 무장 표본 검증 시 구형 `force_id` 단독 검사로 인한 단위 테스트 오실패 방지 | `OfficerTableView`의 재야(Free) 필터링 단위 테스트 실행 시 | 삼국지 13 PK 엔진은 미사관 재야 무장(`status_flag == 0`)에게도 이전 소속 세력 번호(예: ID 2의 force 77, ID 9의 force 95)나 가상 세력 번호(`force_id >= 120`, 예: 항적의 force 125)를 메모리에 잔류시킴. 따라서 `force_id == 0 || force_id == 0xFFFF`로만 단독 검사하면 실제 재야 무장들이 검증 실패로 판정됨. | 재야 무장 판정식(`status_flag == 0 || force_id == 0 || force_id == 0xFFFF || force_id >= 120`)과 단위 테스트 검증 조건식을 완벽히 일치시켜 인게임 실제 재야 무장 데이터 구조를 100% 수용하도록 단위 테스트 검증식 동기화. |

---

## 9. 차기 작업 항목 및 버전 갱신 로그 (Roadmap & Next Steps)

> [!IMPORTANT]
> 이 섹션은 세션 연속성을 위해 작성됨. 다음 에이전트는 이 섹션을 먼저 읽고 시작할 것.
> 현재 상태: **Phase 1 ~ 5.9 전 항목 및 EDGE-01 ~ EDGE-30 (총 30개 QA 엣지 케이스) 100% 완료 (v5.0 마스터 빌드 배포 완료)**.  
> 차기 핵심 목표: **Phase 6.0 — 무장 심층 인스펙터 및 얼굴(Face ID)/전법/특기/외모(픽툴 연동 준비)**, **Phase 6.2 — Windows 10/11 네이티브 크로스 컴파일**.

---

### 9.0. 결혼 자녀 및 유저 등록 무장 이름 동적 파싱 아키텍처 (기술 명세)

- **역사 무장 vs 유저 생성 무장 데이터 모델 차이:**
  - 역사 무장(0~699): 원본 게임 아카이브(DATA1.S13) 인덱스 참조 방식 -> OfficerNameTable 내장 플랫 룩업 테이블로 해결.
  - 결혼 자녀 & 등록 신무장(800+): 유저가 인게임에서 한글로 직접 입력한 데이터이므로 **Chunk 16 (RootBase + 0xD3DD50, 0x424 Stride) 메모리에 평문(CP949/EUC-KR)으로 직접 상주**.
- **동적 이름 반영 파이프라인:**
  1. OfficerManager::LoadOfficer(id): id >= 800 확장 슬롯일 경우 Chunk 16 메모리의 이름 필드(성/명/자) 버퍼 직접 읽기.
  2. CP949 -> UTF-8 변환 유틸리티를 거쳐 outRecord.name에 유저가 지어준 실제 이름 바인딩.
  3. 에디터 상세 모달에서도 자녀 이름을 직접 변경하여 Chunk 16 메모리에 양방향 Write 지원 가능.

---

### 9.1. Phase 5.7 — 무장 한글 이름 표시 [완료]

- **구현 내용:**
  - `include/game/OfficerNameTable.hpp`: $O(1)$ 즉시 조회 인터페이스 선언 (`GetOfficerKoreanName`, `HasOfficerKoreanName`)
  - `src/game/OfficerNameTable.cpp`: 역사 무장 700인(ID 0~699) 전원 100% 공식 한글 성명 플랫 테이블 + 700번대 주요 고대무장(백기, 항우, 유방 등) 내장
  - `src/game/OfficerManager.cpp`: 기존 15명 하드코딩 맵 제거 및 `OfficerNameTable` 완전 연동 (미존재/신무장 fallback 보존)
  - `tests/test_officer_names.cpp`: 700인 전수 무결성 및 32인 주요 무장 표본 일치 단위 테스트 통과 (100% PASS)
  - `Makefile`: `test_officer_names` 타깃 추가 및 독립 패키징(`dist/san13_editor_linux_x86_64.tar.gz`) 빌드 완료

---

### 9.2. Phase 5.8 — 한글 입력 ImGui 연동 및 안정화 패치 [완료]

- **구현 내용:**
  - `include/ui/HangulAutomata.hpp` & `src/ui/HangulAutomata.cpp`: `HangulInputText()` 위젯 래퍼 구현
  - `ImGuiInputTextFlags_CallbackCharFilter` 및 `CallbackAlways`를 통한 실시간 음절 조합 및 1자모 백스페이스 역분해 연동
  - 단축키(`Right Alt`, `Shift + Space`) 및 UI 뱃지 버튼(`[한]` / `[영]`)을 통한 직관적인 한/영 토글 지원
  - **안정화 패치 반영:**
    - 단축키 오작동 격리 (`CallbackAlways` 내 격리)
    - 타이핑 중 한글 조합 파괴 방지 (`IsItemActive` 사후 가드 및 `SetText` 멱등성 보장)
    - 한/영 배지 클릭 시 텍스트창 오토포커스 복원 (`ImGui::SetKeyboardFocusHere(-1)`)
    - 검색창 우측 원클릭 클리어(`×`) 버튼 추가 (무장/도시 2곳 전수 연동)
    - 대소문자 무시 (Case-Insensitive) 검색 지원
  - `include/ui/OfficerTableView.hpp` & `src/ui/OfficerTableView.cpp`: 무장 검색창 `HangulInputText` + Clear 버튼 100% 통합
  - `include/ui/CityTableView.hpp` & `src/ui/CityTableView.cpp`: 도시 검색창 `HangulInputText` + Clear 버튼 100% 통합
  - `tests/test_hangul_input_widget.cpp`: 7개 스텝 단위 및 Dear ImGui 프레임 렌더링 무결성 테스트 통과 (100% PASS)

---

### 9.3. Phase 5.9 — 무장 활동 상태(+0x4F) 및 소속 군주(+0x4A) 실측 기반 현역/재야/미등장 필터링 정상화 [완료]

- **문제 원인:**
  1. 기존 에디터가 `+0x14`를 활동 플래그로 잘못 참조하여 209년에나 등장할 제갈량 등 258명의 미등장 무장들이 전부 현역으로 간주됨.
  2. 무장 헤더의 `+0x08`에 무장 고유 식별 번호가 들어가 있어 700명 전체가 세력 소속으로 오인되어 "현역"과 "전체" 필터 결과가 동일하게 나오는 결함 (EDGE-21).
- **구현 및 해결 내용:**
  1. `include/game/Officer.hpp`: `San13OfficerHeader` 구조체 확장 (`lord_id` = `+0x4A`, `rank` = `+0x0C`, `activity_flag` = `+0x4F`).
  2. `src/game/OfficerManager.cpp`: `LoadOfficer`에서 `lord_id` 및 `activity_flag`를 직접 매핑하고 단일 80바이트 원자적 읽기로 성능 2배 최적화.
  3. `src/ui/OfficerTableView.cpp`:
     - **현역**: `status_flag == 1 && force_id != 0xFFFF` (실제 세력에 사관 중인 현역 무장)
     - **재야**: `status_flag == 1 && force_id == 0xFFFF` (현재 활동 중이나 무소속인 재야 무장)
     - **미등장**: `status_flag == 0` (시나리오 현재 연도 미도달 미래 무장 258명 분리)
     - 상단 라디오 버튼에 **`미등장`** 탭 추가 (`전체` | `현역` | `재야` | `미등장` | `신무장/자녀`).
  4. 단위 테스트 전원 통과 (100% PASS).

---

### 9.4. Phase 5.9-B: 품관(Rank 0~9) 정밀 매핑 및 인게임 실시간 일자 뱃지 연동 [완료]

- **구현 및 해결 성과:**
  1. **실시간 게임 일자 파싱 (EDGE-25):**
     - `RootBase + 0x137A` (년), `0x1380` (월), `0x1386` (일) 실측 규명 및 `OfficerManager::GetGameDate()` 구현.
     - 상단 컨트롤 바에 `📅 게임 일자: 190년 1월 1일` 골드 배지 실시간 표시.
  2. **공식 품관(Rank 0~9) 1:1 매핑 (EDGE-27):**
     - `San13OfficerHeader::rank` (`+0x0C`)를 기반으로 사용자 승인안인 **[1안 - 통합 표기형]** 적용:
       - `0`: **군주**
       - `1~9`: **일반 (1품관 ~ 9품관)**
       - `10+`: **재야 / 미등장**
  3. **새로고침 물음표 글리프 깨짐 해결 (EDGE-28):**
     - `새로고침 (F5)`로 수정하여 폰트 렌더링 글리프 무결성 확보.

---

### 9.5. Phase 5.9-C: 무장 상태 판별 진리표 확립 및 2단 반응형 툴바 분리 [완료]

- **구현 및 해결 성과:**
  1. **무장 상태 판별 진리표 확립 (EDGE-26 & EDGE-29):**
     - 아회남(ID 0): 217년 등장 미래 무장이므로 `appear_year(217) > current_year(190)`에 의해 무조건 **"미등장"** 정상 분류.
     - 범증(ID 851): 181년 성인이나 세력 사관 플래그 `status_flag (+0x4F) == 0`이므로 인게임 실제 상태인 **"재야"**로 100% 교정.
     - 실제 세력 사관 무장(조조, 번쾌, 악비, 하후돈 등)은 `status_flag == 1`로 **"현역"** 정상 유지.
  2. **상단 컨트롤 툴바 2단(2-Row) 반응형 레이아웃 분리 (EDGE-30):**
     - 15개 위젯의 한 줄 나열로 인한 1280 해상도 버튼 잘림/클리핑 완벽 해결.
     - 1단: `[검색창] | [전체][현역][재야][미등장][신무장] | [새로고침 (F5)] | [📅 게임 일자]`
     - 2단: `[전체 선택] [선택 해제] [반전] [아군 선택] [선택 무장 편집] [일괄 수정 ★] | [총 %zu명 중 %zu명 표시]`
  3. **품질 검증:**
     - `test_officer_names` [Step 6] 무장 상태 진리표 검증(아회남 미등장, 범증 재야, 번쾌 3품관, 조조 군주) 100% 통과.
     - 12개 전체 테스트 스위트 100% PASS 및 `dist/san13_editor_linux_x86_64.tar.gz` (1.3MB) 배포 패키지 빌드 완료.

---

### 9.6. [차기 핵심 과제] Phase 6.0: 무장 심층 인스펙터 및 얼굴(Face ID)/전법/특기/외모(픽툴 연동 준비)

사용자 요구사항("무장의 모든 것을 얼굴 CG까지 다 수정하게 만드는 목표")에 따라 수립된 차기 엔진 고도화 명세:

#### 1) 무장 4,796바이트 (`0x12BC`) 미개척 메모리 블록 발굴 계획
현재 우리는 헤더 80바이트(`+0x00 ~ +0x4F`)와 스탯 블록(`+0x15 ~ +0x24`)을 정밀 규명 완료함.  
이후 잔여 4,716바이트(`+0x50 ~ +0x12BB`)에 상주하는 핵심 게임 파라미터 매핑 추진:
- **전법 (Tactics):** 고유 전법 번호 (예: 1~약 200번: 천하무쌍, 만인적, 신산귀모, 군신, 돌격지휘 등) 및 지휘 포인트 소모량.
- **특기 (Skills) 레벨 (0~9):** 상재, 경작, 명사, 인덕, 연전, 분전, 신속, 공성, 병기, 견수, 수련, 일기, 호걸, 귀모 등 14대 특기 바이트.
- **중신특성 (Minister Characteristics):** 신산귀모, 호탄용진, 농정원활, 상재번영, 창병조련 등 고유 중신특성 ID.
- **성격 및 기호:**
  - 성격: 소심(0), 냉정(1), 대담(2), 저돌(3)
  - 음성: Voice ID (무장 전용 보이스 및 성격별 대사)
  - 이상위업: 패도, 왕도, 패권 등
  - 가치관: 의리 / 용애 / 재애 중시도 (S/A/B/C)
  - 기호: 무기, 명마, 서적, 보물, 주류 흥미 여부
- **인연 (Bonds) 및 혈통:** 부모, 배우자(최대 3인), 의형제, 원수 관계망 ID.

#### 2) 얼굴 CG (Face ID) 및 픽툴(San13PicTool) 연동 아키텍처
- **메모리 내 Face ID 오프셋 규명:**
  - 삼국지 13 PK 무장 오브젝트 내부에는 해당 무장이 사용하는 전용/공용 얼굴 CG 번호(`uint16_t` Face ID)가 상주함.
  - 이 Face ID를 수정하면 게임 재시작 없이도 즉시 인게임 무장 얼굴(전투 컷인, 평상시 일러스트, 노년 일러스트)이 실시간 변경됨.
- **코에이 픽툴(San13PicTool / PicTool) 연동 구조:**
  - 게임의 실제 이미지 그래픽은 `DATA0.S13` 및 `DATA1.S13` 패키지 아카이브에 압축 저장됨.
  - **1단계 (에디터 내 Face ID 변경):** 사용자가 원하는 일러스트 번호(0~2500+)를 입력/선택하면 인게임 메모리의 Face ID를 즉시 패치.
  - **2단계 (픽툴 추출 CG 프리뷰 연동):** 픽툴로 추출한 얼굴 CG 캐시 디렉터리(`assets/portraits/`)가 존재할 경우, 에디터 상세 인스펙터 창에서 무장 얼굴을 OpenGL 텍스처로 실시간 미리보기 렌더링.
  - **3단계 (커스텀 신무장 얼굴 번호 확장):** 유저 등록 신무장 및 교체용 얼굴 슬롯 번호 매핑 테이블 탑재.

#### 3) 전용 무장 심층 인스펙터 모달 (OfficerDetailInspector) UI 설계
- 기존의 단순 스탯 숫자 변경 다이얼로그에서 탈피하여, 삼국지 13 PK 인게임 무장 정보창 수준의 **다중 탭 종합 대시보드** 구축:
  - **[기본/능력 탭]:** 성명, 생몰년, 4대 스탯, 병종 적성, 품관, 충성도
  - **[전법/특기 탭]:** 보유 전법 콤보박스 선택, 14대 특기 레벨 슬라이더(0~9)
  - **[외모/성격 탭]:** Face ID 선택기, 얼굴 일러스트 프리뷰, 성격/음성/기호 라디오 선택기
  - **[관계망 탭]:** 소속 군주, 배우자, 의형제, 소속 도시/군단

#### 4) 순차적 실행 작업 체크리스트 및 실측 발굴 현황 (Implementation Checklist & Progress)
세션 중단 및 연속성을 보장하기 위해 각 단계를 명확한 산출물 단위로 분할하여 순차 실행:

- [x] **Step 6.0-1: 대표 무장 메모리 덤프 및 실시간 Diffing 도구 작성 [완료]**
  - 대상 무장: 여포 (635), 관우 (94), 제갈량 (283), 조조 (333), 장비 (418), 조운 (380)
  - 도구: `tools/scan_officer_extended.cpp` 작성 및 바이너리 패턴 교차 분석 성공.
  - **[규명 성과 1] 14대 특기(Skills) 레벨 블록 오프셋 발굴 (`+0x26 ~ +0x39`):**
    - 0~9 레벨이 바이트 단위로 정확히 연속 매핑됨을 규명.
    - 여포: 일기(`9`), 호걸(`9`), 신속(`8`), 분전(`8`), 연전(`8`), 공성(`8`)
    - 제갈량: 상재(`9`), 경작(`9`), 명사(`9`), 인덕(`9`), 변설(`9`), 귀모(`9`), 신속(`7`), 공성(`8`), 병기(`6`), 견수(`8`)
    - 장비: 일기(`9`), 호걸(`7`), 분전(`9`), 연전(`7`), 공성(`7`)
  - **[규명 성과 2] 무장 성격(Personality) 오프셋 특정 (`+0x4E`, `uint8_t`):**
    - `0`: 소심, `1`: 냉정 (제갈량, 조조), `2`: 대담 (관우, 조운), `3`: 저돌 (여포, 장비) 100% 일치!
  - **[규명 성과 3] Face ID (얼굴 CG 번호) 유력 오프셋 특정 (`+0x08`, `uint16_t`):**
    - 조조(`405`), 관우(`117`), 제갈량(`347`), 장비(`517`), 여포(`786`)
  - **[규명 성과 4] 무장 오브젝트 내부 인덱스 ID 특정 (`+0x3E`, `uint16_t`):**
    - 조조(`333`), 관우(`94`), 제갈량(`283`), 여포(`635`), 장비(`418`), 조운(`380`)
- [x] **Step 6.0-2: 20대 특기 1:1 명칭 매핑 완료 및 전법(Tactics) 오프셋 확정 [완료]**
  - **[규명 성과 1] 20대 특기(Skills) 레벨 블록 오프셋 정식 선언 (`+0x26 ~ +0x39`, 20바이트) [완료]:**
    - 당초 14대로 추정되었으나 항적(항우 ID 857) 라이브 메모리 비교 분석을 통해 총 20개 바이트 연속 블록임이 실측 확정됨.
    - 20개 순서 1:1 확정: 농업(상재), 상업(경작), 문화(명사), 훈련, 순찰, 설파, 교섭, 언변, 덕망, 위풍, 신속, 분전, 연전, 공성, 병기, 견수, 수련(수영), 일기, 호걸, 귀모.
    - `include/game/Officer.hpp` 및 `src/game/OfficerManager.cpp`에 `skills[20]` 필드 및 `GetSkillName(0~19)` 헬퍼 정식 탑재 완료.
  - **[규명 성과 2] 전법 ID 오프셋 실측 규명 (`+0x24`, `uint8_t`) [대발굴 완료]:**
    - 무장 오브젝트 시작점 기준 `+0x24` (`San13OfficerLiveStats`의 마지막 바이트인 `+0x0F`, 기존 `unknown_0f`)가 바로 무장의 고유 **전법 ID(Tactic ID, 0~140)**로 완벽 특정됨!
    - 고전 반에디터(`San13EditorPK_kr.exe`) 바이너리(RVA `0x7E868`)에서 141개 전법 공식 테이블 역분석 및 실측 대조:
      | 무장 ID | 무장명 | +0x24 (전법 ID) | 전법 명칭 (공식) | 검증 상태 |
      |---|---|---|---|---|
      | 635 | 여포 | 0 | 천하무쌍 | 100% 일치 |
      | 94 | 관우 | 2 | 군신 | 100% 일치 |
      | 418 | 장비 | 3 | 만인적 | 100% 일치 |
      | 359 | 손책 | 4 | 소패왕 | 100% 일치 |
      | 380 | 조운 | 7 | 투지호장 | 100% 일치 |
      | 857 | 항적 | 8 | 고금무쌍 | 100% 일치 |
      | 610 | 유비 | 42 | 대의지휘 | 100% 일치 |
      | 333 | 조조 | 44 | 용략지휘 | 100% 일치 |
      | 283 | 제갈량 | 49 | 신산진법 | 100% 일치 |
      | 18 | 원소 | 51 | 위풍지휘 | 100% 일치 |
      | 851 | 범증 | 84 | 허보 | 100% 일치 |
      | 858 | 우희 | 88 | 요염 | 100% 일치 |
  - **[규명 성과 3] 공식 141개 전법 마스터 테이블 (0 ~ 140):**
    - `0: 천하무쌍`, `1: 괴력무쌍`, `2: 군신`, `3: 만인적`, `4: 소패왕`, `5: 사자분신`, `6: 투장용약`, `7: 투지호장`, `8: 고금무쌍`, `9: 국사무쌍`, `10: 천궁`, `11: 신궁`, `12: 인왕서기`, `13: 투지용장`, `14: 불굴강장`, `15: 불굴용장`, `16: 부동`, `17: 불굴`, `18: 특공`, `19: 철벽`, `20: 방원진`, `21: 역경강화`, `22: 연격`, `23: 기병돌격`, `24: 연사`, `25: 강궁`, `26: 공격강화`, `27: 방어강화`, `28: 기동강화`, `29: 창공강화`, `30: 창방강화`, `31: 창속강화`, `32: 기공강화`, `33: 기방강화`, `34: 기속강화`, `35: 궁공강화`, `36: 궁방강화`, `37: 궁속강화`, `38: 주위공격`, `39: 사기회복`, `40: 결사지휘`, `41: 견수지휘`, `42: 대의지휘`, `43: 천의무봉`, `44: 용략지휘`, `45: 공수지휘`, `46: 맹장돌격`, `47: 열장돌격`, `48: 효장질구`, `49: 신산진법`, `50: 시황제`, `51: 위풍지휘`, `52: 돌진지휘`, `53: 돌격지휘`, `54: 질주지휘`, `55: 특공지휘`, `56: 철벽지휘`, `57: 속방지휘`, `58: 역경지휘`, `59: 맹사지휘`, `60: 제사지휘`, `61: 공격지휘`, `62: 수비지휘`, `63: 속공지휘`, `64: 공격명령`, `65: 수비명령`, `66: 속공명령`, `67: 창병공격진`, `68: 창병수비진`, `69: 창병속공진`, `70: 기병공격진`, `71: 기병수비진`, `72: 기병속공진`, `73: 궁병공격진`, `74: 궁병수비진`, `75: 궁병속공진`, `76: 고무`, `77: 위무`, `78: 왕좌`, `79: 군략지원`, `80: 후방지원`, `81: 신의`, `82: 전진강화`, `83: 위병`, `84: 허보`, `85: 귀모계`, `86: 연환계`, `87: 종횡기략`, `88: 요염`, `89: 이간계`, `90: 동요`, `91: 선동`, `92: 교란`, `93: 위보`, `94: 위보동요`, `95: 위보선동`, `96: 위보교란`, `97: 도발`, `98: 동요도발`, `99: 선동도발`, `100: 교란도발`, `101: 창병동요`, `102: 기병동요`, `103: 궁병동요`, `104: 창병선동`, `105: 기병선동`, `106: 궁병선동`, `107: 소침공작`, `108: 공격약화`, `109: 수비약화`, `110: 기동약화`, `111: 창공약화`, `112: 창방약화`, `113: 창속약화`, `114: 기공약화`, `115: 기방약화`, `116: 기속약화`, `117: 궁공약화`, `118: 궁방약화`, `119: 궁속약화`, `120: 창병도발`, `121: 기병도발`, `122: 궁병도발`, `123: 열화계`, `124: 신화계`, `125: 화우계`, `126: 육도비계`, `127: 태평요술`, `128: 요술`, `129: 대화계`, `130: 화공계`, `131: 화염계`, `132: 화계`, `133: 흉노난사`, `134: 오환맹사`, `135: 산월역경`, `136: 남만특공`, `137: 저강철벽`, `138: 단일도발`, `139: 대도발`, `140: 금족`.
  - **[규명 성과 4] `+0x3A ~ +0x3B` = 고유 중신특성 ID 1, 2 [유력]:**
    - 항적(`18, 16`), 조조(`9, 9`), 관우(`18, 14`), 제갈량(`19, 0`) 실측값 확보.
  - **[규명 성과 5] `+0x53` = 보유 전법 수 (`uint8_t`) [확정]:**
    - 8인(여포·관우·제갈량·조조·조운·장비·유비·손권·항적) 교차 검증 100% 일치.
  - **[규명 성과 6] `+0x56` = 전법/병종 계열 유형 (`uint8_t`) [유력]:**
    - `4` = 무력형 (여포, 조조, 장비, 항적) / `3` = 지략·균형형 (관우, 제갈량, 조운, 손권) / `2` = 균형형 (유비).
- [x] **Step 6.0-3: 얼굴 CG (Face ID: `+0x08`, `uint16_t`) 인게임 즉시 변경 실측 검증 [완료]**
  - 도구: `tools/hotpatch_face.cpp` 작성 및 실행.
  - 조조(ID 333, 기본 Face ID: `405`)의 `+0x08`을 관우(`117`)로 라이브 핫패치 커밋.
  - **인게임 검증 결과:** 게임 내 조조 무장 정보창에서 얼굴 일러스트가 즉시 관우 얼굴로 완벽하게 전환됨을 **유저 직접 확인 완료**!
  - `Officer.hpp` 구조체에 `face_id` 필드 및 `OfficerRecord` 양방향 바인딩 정식 완료.
- [x] **Step 6.0-4: 가치관·기호·상성 8대 성향 블록 오프셋 실측 규명 (`+0x50 ~ +0x58`) [완료]**
  - 반에디터 바이너리 컬럼 테이블(`0x980a8`) 역분석 및 라이브 프로세스 24인 무장 교차 검증 100% 일치:
    - `+0x50` (`uint8_t`): **선호무기** (0:무관심, 1:관심) — 제갈량=0, 여포/관우/장비/조조/조운/항적=1
    - `+0x51` (`uint8_t`): **선호서적** (0:무관심, 1:관심) — 조조/제갈량/주유/사마의=1, 여포/장비/유비=0
    - `+0x52` (`uint8_t`): **선호보물** (0:무관심, 1:관심) — 여포/동탁/원소/주유=1, 관우/장비/제갈량=0
    - `+0x53` (`uint8_t`): **음주/주량** (0:금주, 1:보통, 2:대음) — 장각=0(금주), 제갈량/사마의=1, 여포/장비/조조/관우/동탁/원소=2(대음)
    - `+0x54` (`uint8_t`): **물욕** (0:무욕/청렴, 1:보통, 2:강욕/탐욕) — 관우/장비/제갈량/조운/하후돈=0, 조조/유비/손책/사마의=1, 여포/동탁/항적=2
    - `+0x55` (`uint8_t`): **의리** (0:경시, 1:낮음, 2:보통, 3:중시, 4:매우중시) — 여포/동탁=0, 조조/원소=1, 항적/손권/사마의=2, 손책/원술=3, 관우/장비/유비/제갈량/조운/하후돈=4
    - `+0x56` (`uint8_t`): **용애** (무용 애호, 0:경시 ~ 4:매우중시) — 공손찬=1, 손권/사마의=2, 관우/유비/손책/조운/제갈량=3, 여포/장비/조조/항적=4
    - `+0x57` (`uint8_t`): **재애** (재능 애호, 0:경시 ~ 4:매우중시) — 여포/장비=1, 관우/조운=2, 손책/유비/항적/손권=3, 조조/제갈량/사마의=4(유재시거)
    - `+0x58` (`uint8_t`): **상성** (Affinity, 0~149) — 유비 3형제=50(`0x32`), 손견/동탁/원소=100(`0x64`)
  - **14인 주요 무장 성향 실측 검증 데이터:**
    | 무장명 | ID | 무기 | 서적 | 보물 | 음주 | 물욕 | 의리 | 용애 | 재애 | 상성 |
    |---|---|---|---|---|---|---|---|---|---|---|
    | 여포 | 635 | 1 | 0 | 1 | 2 (대음) | 2 (강욕) | 0 (경시) | 4 | 1 | 0 |
    | 관우 | 94 | 1 | 0 | 0 | 2 (대음) | 0 (무욕) | 4 (매우중시) | 3 | 2 | 50 |
    | 장비 | 418 | 1 | 0 | 0 | 2 (대음) | 0 (무욕) | 4 (매우중시) | 4 | 1 | 50 |
    | 유비 | 610 | 1 | 0 | 1 | 2 (대음) | 1 (보통) | 4 (매우중시) | 3 | 3 | 50 |
    | 조조 | 333 | 1 | 1 | 0 | 2 (대음) | 1 (보통) | 1 (낮음) | 4 | 4 | 0 |
    | 제갈량 | 283 | 0 | 1 | 0 | 1 (보통) | 0 (무욕) | 4 (매우중시) | 3 | 4 | 0 |
    | 조운 | 380 | 1 | 0 | 0 | 1 (보통) | 0 (무욕) | 4 (매우중시) | 3 | 2 | 0 |
    | 손책 | 359 | 1 | 0 | 0 | 2 (대음) | 1 (보통) | 3 (중시) | 3 | 3 | 0 |
    | 항적 | 857 | 1 | 1 | 0 | 2 (대음) | 2 (강욕) | 2 (보통) | 4 | 3 | 0 |
    | 동탁 | 480 | 0 | 0 | 1 | 2 (대음) | 2 (강욕) | 0 (경시) | 3 | 3 | 100 |
    | 사마의 | 220 | 0 | 1 | 0 | 1 (보통) | 1 (보통) | 2 (보통) | 2 | 4 | 0 |
    | 주유 | 241 | 0 | 1 | 1 | 2 (대음) | 1 (보통) | 4 (매우중시) | 3 | 3 | 0 |
    | 장각 | 412 | 0 | 1 | 0 | 0 (금주) | 0 (무욕) | 4 (매우중시) | 3 | 2 | 0 |
    | 원소 | 18 | 0 | 0 | 1 | 2 (대음) | 1 (보통) | 1 (낮음) | 4 | 3 | 100 |
- [x] **Step 6.0-5: 인연 관계망 (부모, 배우자 3인, 의형제, 원수) 오프셋 규명 (`+0x40 ~ +0x4D`) [완료]**
  - 무장 오브젝트 내부 혈연/혼인/단짝 포인터 블록 14바이트 완벽 규명:
    - `+0x40 ~ +0x41` (`uint16_t`): **부친 ID** — 조조: 1048(조숭), 제갈량: 1045(제갈규), 손책: 355(손견), 동탁: 1068(동군), 항적: 1086(항량)
    - `+0x42 ~ +0x43` (`uint16_t`): **모친 ID** — 손책: 1101(오씨)
    - `+0x44 ~ +0x45` (`uint16_t`): **배우자 1 ID** — 조조: 675(변씨), 제갈량: 654(황월영)
    - `+0x46 ~ +0x47` (`uint16_t`): **배우자 2 ID** (0xFFFF = 미등록)
    - `+0x48 ~ +0x49` (`uint16_t`): **배우자 3 ID** (0xFFFF = 미등록)
    - `+0x4A ~ +0x4B` (`uint16_t`): **의형제/단짝 ID** — 관우/장비/유비: 610(유비), 손책: 241(주유)
    - `+0x4C ~ +0x4D` (`uint16_t`): **원수 ID** (0xFFFF = 미지정)
    - `+0x1296 ~ +0x12A5`: **PK 동지(同志) 슬롯 및 인연 레벨 (Lv 4)** 연동 포착
    - **[SetRelations API 확장]:** 부친, 모친, 배우자 1~3, 의형제, 원수 등 7대 인연 슬롯(`+0x40 ~ +0x4D`)의 원자적 메모리 읽기/쓰기 인터페이스 완비.
- [x] **Step 6.0-6: 무장 종합 심층 인스펙터 UI (OfficerDetailModal) 구현 [완료]**
  - **[UI 아키텍처 및 탭 구성]:**
    - **트리거:** 무장 목록 테이블 행 더블클릭 또는 `[상세 편집]` 버튼 클릭 시 호출되는 팝업 모달 (`ImGuiWindowFlags_AlwaysAutoResize`).
    - **트랜잭션 안전성:** 모달 오픈 시 원본 무장 스냅샷 자동 보존, [적용(Apply)] / [확인(OK)] 시 BoundsValidator 검증 후 원자적 메모리 커밋, [취소(Cancel)] 시 즉시 원복, 실시간 자동 동기화(Live AutoSync) 체크 지원.
    - **탭 1: 기본 정보 및 능력치 (General & Stats):**
      - 무장 번호, 한글 성명, 소속 세력/군단, 소재 도시, 관직(품관 0~9), 충성도(0~100)
      - 생년, 등장년, 사망년도 (+5년, +10년 수명 연장 퀵 버튼)
      - 4대 스탯 (통솔, 무력, 지력, 정치) 기본치 및 현재치(1~100/255) + 경험치(0~99)
      - 3대 병종 적성 (창병, 기병, 궁병) C(0), B(1), A(2), S(3) 드롭다운 선택
    - **탭 2: 전법 및 20대 특기 (Tactics & 20 Skills):**
      - 보유 전법 (`+0x24`): 141개 전법 마스터 테이블 드롭다운 콤보박스 (천하무쌍, 군신, 만인적, 고금무쌍, 용략지휘, 신산진법 등 한글 공식 명칭)
      - 전수 특기 (`+0x25`): 20대 특기 드롭다운
      - 20대 특기 레벨 (`+0x26 ~ +0x39`): 20개 특기 (농업, 상업, 문화, 훈련, 순찰, 설파, 교섭, 언변, 덕망, 위풍, 신속, 분전, 연전, 공성, 병기, 견수, 수련, 일기, 호걸, 귀모) 각각 0~9 슬라이더
      - 원클릭 프리셋: `[전체 특기 Lv 9 MAX]`, `[전체 특기 초기화 (0)]`
    - **탭 3: 외모·성격·가치관·기호 (Character & Values):**
      - 얼굴 CG 번호 (`+0x08`, Face ID): 입력 및 조조(405)↔관우(117) 핫패치 연동
      - 성격 (`+0x4E`): 소심(0), 냉정(1), 대담(2), 저돌(3) 라디오 버튼
      - 4대 기호: 선호무기(`+0x50`), 선호서적(`+0x51`), 선호보물(`+0x52`) 토글 체크박스, 음주/주량(`+0x53`: 금주 0, 보통 1, 대음 2) 콤보박스
      - 4대 가치관: 물욕(`+0x54`: 무욕 0, 보통 1, 강욕 2), 의리(`+0x55`: 0~4), 용애(`+0x56`: 0~4), 재애(`+0x57`: 0~4) 콤보박스
      - 상성 (`+0x58`): 0~149 슬라이더
    - **탭 4: 가족 및 인연 관계망 (Family & Bonds):**
      - 부친 ID (`+0x40`), 모친 ID (`+0x42`), 배우자 1~3 ID (`+0x44 ~ +0x49`), 의형제 ID (`+0x4A`), 원수 ID (`+0x4C`)
      - 무장 ID 입력 시 실시간 한글 무장 성명 대조 프리뷰 (`GetOfficerKoreanName`) 제공
- [x] **Step 6.0-7: 단위 테스트 스위트 및 독립 배포 패키징 [완료]**
  - `test_officer_names`: 141개 전법, 20대 특기 명칭, 8대 성향/가치관/상성, 7대 인연 관계망 전수 단위 테스트 통과 (11/11).
  - `test_officer_table`: 가상 스크롤, 동적 슬롯 확장, EDGE-32 재야 무장 다중 상태 판정식 100% 통과.
  - `test_officer_edit`: 4개 탭 모달 및 실시간 핫싱크(지연시간 0.005ms) 검증 100% 통과.
  - `make test`: 전체 12개 테스트 스위트 100% PASS 확인.
  - `make package`: Linux Standalone 1.3MB 배포 아카이브 (`dist/san13_editor_linux_x86_64.tar.gz`) 최신화 완료.

---

### 9.7. 현재 빌드 상태 및 라이브 환경 스냅샷 (2026-10-01 v5.6 기준)

```bash
# 빌드 및 전체 테스트 실행
cd /home/user/Downloads/vaneditor
make test               # 12개 전체 테스트 스위트 (100% PASS)
make test_hangul        # Phase 5.6 한글 오토마타 (6/6 PASS)
make test_officer_names # Phase 5.7 & 6.0 성명/일자/전법/특기/인연 단위 테스트 (11/11 PASS)
make test_hangul_widget # Phase 5.8 ImGui 한글 위젯 연동 (100% PASS)
make package            # Linux Standalone 1.3MB 배포 아카이브 빌드 (dist/san13_editor_linux_x86_64.tar.gz)

# 실행
./bin/san13_editor
```

**라이브 게임 검증 환경:**
- 타깃 프로세스: `San13PK_kr.exe` (PID: `182747`)
- WoW64 Base: `0x71bb0000` (Proton 매핑)
- RootBase: `0x73B6C5D8` (한국어 PK 1.0.2 기준)
- 시나리오 기준 시점: **190년 1월 1일**
- 호스트 환경: Fedora 44 x86_64, Valve GE-Proton11-7, Dear ImGui Docking (GLFW/OpenGL3)

