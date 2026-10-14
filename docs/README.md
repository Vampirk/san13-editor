# 삼국지 13 PK 한국어 메모리 에디터 기술 문서 포털 (Documentation Hub)

본 디렉터리는 **삼국지 13 PK 한국어 메모리 에디터 & 트레이너**의 메모리 구조체 역분석 명세, 시스템 아키텍처, 구현 로드맵 및 품질 검증 문서를 분야별로 체계화한 공식 문서 저장소입니다.

---

## 1. 문서 분류 체계 (Documentation Taxonomy)

```text
docs/
├── README.md               # [현재 문서] 전체 문서 인덱스 및 분야별 탐색 가이드
│
├── specs/                  # [기술 명세 & 단일 진실 공급원 (SSOT)]
│   ├── OFFSETS.md          # 메모리 주소, 구조체 레이아웃, 오프셋 확정 명세서
│   ├── ARCHITECTURE.md     # 3계층 시스템 아키텍처, 데이터 흐름, 디렉터리 구조
│   └── OFFICER_NAME_SYSTEM.md # 전체 무장 이름 시스템, XL13(1655) 포맷, PK 100인 명단
│
├── plans/                  # [구현 로드맵 및 개발 계획]
│   ├── 01_PROTAGONIST_PLAN.md      # 주인공(플레이어) 무장 감지 및 제어 계획
│   ├── 02_CITY_EXPANSION_PLAN.md   # 도시 60개 자원 및 300개 부락 종합 인스펙터 계획
│   ├── 03_CORE_ENHANCEMENT_PLAN.md # Phase 1~5 코어 확장 (다중인연, 세력, 명품) 계획
│   ├── 04_UI_ENHANCEMENT_PLAN.md   # 핵심 버그 교정, 반응형 UI 및 테마 폴리싱 계획
│   └── 05_BACKLOG.md               # [진행 중] 버그 제보 / 기능 개선 / 픽툴 현대화 백로그
│
├── quality/                # [품질 보증 및 릴리즈 관리]
│   ├── VERIFICATION.md     # 19개 단위/통합 테스트 스위트 및 실측 검증 규격
│   ├── QA_CHECKLIST.md     # 릴리즈 배포 전 필수 QA 점검표 (체크리스트)
│   └── CHANGELOG.md        # 버전별 변경 이력 및 이슈 해결 기록
│
└── archive/                # [레거시 연구 기록]
    └── SAN13PK_Memory_Spec.md # 초기 메모리 발굴 및 연구 원본 보존본
```

---

## 2. 분야별 문서 안내 (Quick Links & Overview)

### 📌 1. 기술 명세 (Specs)
- **[`specs/OFFICER_NAME_SYSTEM.md`](specs/OFFICER_NAME_SYSTEM.md)**:
  - 삼국지 13 PK 전체 무장 이름 시스템 및 게임 원본 데이터(`DATA1.S13` Entry 1655 / `pk_1655.tbl`) 역분석 명세.
  - `XL13` 컨테이너 구조 및 성(6535), 명(7985), 자(9435) 균일 블록 오프셋 산출 공식.
  - 0 ~ 1450 무장 슬롯 전체 분류표 및 **PK 공식 추가 사실무장 100인(1151 윤례 ~ 1250 이씨)** 전수 실명/자 매핑 목록.
  - 유저 등록 신무장(`editdata_pk.s13`) 연동 아키텍처.
- **[`specs/OFFSETS.md`](specs/OFFSETS.md)**:
  - Proton WoW64 환경에서의 `RootBase` 계산 공식 및 게임 버전별 해시.
  - 무장 (`COfficer` 4,796B), 도시 (`CCity` 368B), 요충/부락 성채 (`CChiiki` 20B), 명품 (`CItem` 36B), 전법(141종) 등 100% 실측 검증된 메모리 레이아웃.
  - 시나리오 일자(`0x137A/0x137C/0x1380`) 및 폐기된 초기 가설 목록 기록.
- **[`specs/ARCHITECTURE.md`](specs/ARCHITECTURE.md)**:
  - Presentation Layer (Dear ImGui), Data Model Layer, Abstracted Memory Core (Linux `process_vm_readv` / Windows `ReadProcessMemory`) 3계층 아키텍처 다이어그램.
  - 디렉터리 구성 및 원자적 구조체 직렬화, 멀티 트랜잭션 롤백(Undo) 설계.

### 📌 2. 개발 로드맵 및 계획 (Plans)
- **[`plans/01_PROTAGONIST_PLAN.md`](plans/01_PROTAGONIST_PLAN.md)**:
  - 주인공 무장 실시간 탐색 및 개인 소지금 제어 로드맵.
- **[`plans/02_CITY_EXPANSION_PLAN.md`](plans/02_CITY_EXPANSION_PLAN.md)**:
  - 60개 도시 자원 및 300개 부락 개발도, 소속 인물 연결 리스트 순회 인스펙터 구축 로드맵.
- **[`plans/03_CORE_ENHANCEMENT_PLAN.md`](plans/03_CORE_ENHANCEMENT_PLAN.md)**:
  - Phase 1~5 코어 확장 (다중 의형제/친애 슬롯, 세력 실명화 매핑, 명품 매니저 역색인).
- **[`plans/04_UI_ENHANCEMENT_PLAN.md`](plans/04_UI_ENHANCEMENT_PLAN.md)**:
  - 시나리오 일자/부락 요새화 버그 수정, 명품 시장 위치 표출, 창 반응형 폰트/테이블 스케일링, 삼국지 앤틱 다크 골드 테마 폴리싱.
- **[`plans/05_BACKLOG.md`](plans/05_BACKLOG.md)** ⭐ **현재 작업 기준 문서**:
  - 사용자 버그 제보, 기능 개선 요청, 기술 부채, 장기 과제(San13PicTool 현대화)를 한 곳에서 관리하는 백로그.
  - 01~04 계획서는 완료된 마일스톤 기록이며, 신규 작업은 모두 이 문서에 먼저 등록합니다.

### 📌 3. 품질 및 검증 (Quality & Release)
- **[`quality/VERIFICATION.md`](quality/VERIFICATION.md)**:
  - 19개 자동화 단위/라이브 통합 테스트 스위트(오프라인 5 + 라이브 14)의 구성과 실행 방법, 승인 기준(Acceptance Criteria).
- **[`quality/QA_CHECKLIST.md`](quality/QA_CHECKLIST.md)**:
  - UI 렌더링, 실시간 메모리 핫싱크, 한계 돌파 모드, 한국어 IME 한글 입력 무결성 체크리스트.
- **[`quality/CHANGELOG.md`](quality/CHANGELOG.md)**:
  - Phase 1 ~ 6.8 마일스톤(듀얼 플랫폼 릴리즈까지) 및 QA 엣지 케이스(EDGE-01 ~ EDGE-54) 변경 로그.

---

## 3. 기여 및 문서 갱신 원칙
1. **단일 진실 공급원(SSOT)**: 오프셋이나 구조체 변경 시 반드시 `specs/OFFSETS.md`를 1차로 갱신합니다.
2. **검증 수준 준수**: 모든 주소와 오프셋은 `[확정]`, `[유력]`, `[미확인]` 3단계 중 하나로 명시합니다.
3. **폐기 이력 보존**: 과거 잘못 추측되었던 오프셋은 삭제하지 않고 `OFFSETS.md`의 `폐기된 초기 가설 목록`에 교정 내용과 함께 보존합니다.
4. **작업 등록 우선**: 버그·기능 작업은 착수 전에 `plans/05_BACKLOG.md`에 항목(ID)을 먼저 등록하고, 완료 시 `CHANGELOG.md`(필요 시 EDGE 번호 부여)로 이관합니다.
5. **버전 표기 구분**: 문서 헤더의 `기준 판본`(현재 **v6.0**)은 문서/명세 개정 번호이며, 에디터 앱 릴리즈 버전(현재 **v1.0.0**, `src/main.cpp`의 `APP_VERSION`)과는 별개입니다.
6. **완료 계획서 보존**: `plans/01~04`와 `archive/`는 당시 시점의 기록이므로 본문 수치(테스트 개수 등)를 소급 수정하지 않습니다.
