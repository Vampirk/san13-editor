# San13PK Linux Memory Editor & Trainer Suite

> 삼국지 13 with 파워업키트 (Romance of the Three Kingdoms 13 PK) Linux/Proton 전용 네이티브 실시간 메모리 에디터입니다.

---

## 1. 프로젝트 소개

본 프로젝트는 리눅스 환경(Steam Proton WoW64)에서 실행 중인 삼국지 13 PK 한국어판(v1.0.2)의 게임 메모리를 실시간으로 읽고 패치할 수 있는 독립 실행형(GUI) 에디터입니다.

- **삼국지 앤틱 다크 골드 테마 & 반응형 UI:** 딥 차콜 오크 & 브론즈 테두리, 창 크기별 폰트/테이블 자동 스케일링(`FontGlobalScale`).
- **네이티브 한글 지원:** 2벌식 한글 오토마타 엔진 내장 및 나눔고딕 폰트 렌더링.
- **실시간 핫싱크(Live Hot-Sync):** 세이브 파일 언팩/리팩 없이 게임 플레이 중 실시간 메모리 패치.
- **무장 종합 관리 (2,165명 슬롯 지원):**
  - 능력치(통솔/무력/지력/정치 기본·현재치, 경험치, 병종 적성 C/B/A/S, 관직, 충성, 수명)
  - 141개 전법 마스터 테이블 콤보박스 및 20대 특기 0~9 레벨 슬라이더
  - 외모(Face ID 얼굴 CG), 성격, 3대 선호물, 음주, 물욕, 의리, 용애, 재애, 상성
  - 7대 혈통 및 다중 인연 관계망 (부친, 모친, 배우자 1~3, 의형제 1~3, 친애 1~6, 원수 1~3)
  - 플레이어 주인공 감지 및 개인 소지 금 실시간 편집
- **60개 도시 종합 인스펙터 (4탭 시스템):**
  - 7열 그리드 뷰 (ID, 도시명, 지배 세력/태수, 자금, 군량, 병력, 내구도)
  - 경제/자원 탭, 내정 탭 (민심, 상업, 농업, 문화)
  - **관할 부락 및 CChiiki 요새화:** 300개 부락 장악도 및 견진/군악대/요양사/병량고/방채 성채 건축 실시간 반영
  - 군사/숙련도 탭 (창병, 기병, 궁병 숙련도) 및 도시 소속 인물 체인 순회
- **명품(Item) 매니저 & 시장 위치 연동:**
  - 234종 공식 명품 + 확장 슬롯, 능력치 보정, 부가 특기
  - 소유 무장 원클릭 수여/몰수 및 **미소유 보물 시장(소재지 도시) 위치 추적/필터**
- **대규모 일괄 수정 및 트랜잭션 롤백:** 다중 선택, 일괄 버프, 수명 일괄 연장, 실행 취소(Undo 롤백).

---

## 2. 요구 환경 (Prerequisites)

- **운영체제:** Linux x86_64 (Fedora, Ubuntu, Arch, SteamOS 등)
- **타깃 게임:** Steam판 Romance of the Three Kingdoms 13 with Power Up Kit (한국어판 최신 v1.0.2)
- **실행 레이어:** Valve Proton (GE-Proton 또는 Proton Experimental)
- **필수 빌드 패키지:**
  - `g++` (C++17 지원)
  - `make`
  - `glfw` 개발 라이브러리 (`glfw-devel` 또는 `libglfw3-dev`)
  - `OpenGL` 및 `dl` 개발 라이브러리

---

## 3. 빌드 및 테스트 방법 (Build & Test)

```bash
# 저장소 루트로 이동
cd ~/Downloads/vaneditor

# 1. 메인 에디터 및 전체 단위/통합 테스트 빌드
make -j$(nproc)

# 2. 16개 전체 단위/통합 테스트 스위트 실행
make test

# 3. 120프레임 무결성 GUI 스모크 테스트
./bin/san13_editor --smoke-test

# 4. Linux Standalone 배포 아카이브 패키징
make package
# 결과물: dist/san13_editor_linux_x86_64.tar.gz (약 1.4MB)
```

---

## 4. 실행 방법 (Usage)

1. Steam에서 **삼국지 13 PK**를 실행하고 시나리오에 진입합니다.
2. 터미널에서 에디터를 실행합니다:

```bash
./bin/san13_editor
```

3. 에디터가 게임 프로세스(`San13PK_kr.exe`)를 자동으로 감지하고 `RootBase` 주소를 바인딩합니다.
4. 무장/도시/명품 테이블에서 원하는 항목을 **더블클릭**하거나 우클릭하여 상세 편집 창을 열고 실시간 수정을 적용합니다.

> **참고 (ptrace 권한 안내):**  
> Linux 보안 정책(`yama ptrace_scope`)에 따라 일반 유저 권한에서 타깃 프로세스의 메모리 읽기/쓰기가 차단될 경우, `sudo sysctl kernel.yama.ptrace_scope=0`을 실행하거나 `dist/run.sh` 런처를 이용하십시오.

---

## 5. 상세 기술 문서 안내 (Documentation Hub)

자세한 역분석 결과 및 시스템 사양은 [`docs/README.md`](docs/README.md)를 참고하십시오:

- **기술 명세 (Specs):**
  - [`docs/specs/OFFSETS.md`](docs/specs/OFFSETS.md): 메모리 오프셋 및 구조체 규명 명세서 (검증 수준 표기)
  - [`docs/specs/ARCHITECTURE.md`](docs/specs/ARCHITECTURE.md): 3계층 시스템 아키텍처 및 디렉터리 구조
- **개발 로드맵 (Plans):**
  - [`docs/plans/01_PROTAGONIST_PLAN.md`](docs/plans/01_PROTAGONIST_PLAN.md): 주인공 무장 기능 계획
  - [`docs/plans/02_CITY_EXPANSION_PLAN.md`](docs/plans/02_CITY_EXPANSION_PLAN.md): 도시/부락 확장 계획
  - [`docs/plans/03_CORE_ENHANCEMENT_PLAN.md`](docs/plans/03_CORE_ENHANCEMENT_PLAN.md): 코어 확장 계획
  - [`docs/plans/04_UI_ENHANCEMENT_PLAN.md`](docs/plans/04_UI_ENHANCEMENT_PLAN.md): 버그 수정, 반응형, 테마 폴리싱 계획
- **품질 관리 (Quality):**
  - [`docs/quality/VERIFICATION.md`](docs/quality/VERIFICATION.md): 16개 테스트 스위트 및 실측 증거 보고서
  - [`docs/quality/QA_CHECKLIST.md`](docs/quality/QA_CHECKLIST.md): 배포 전 필수 QA 점검표
  - [`docs/quality/CHANGELOG.md`](docs/quality/CHANGELOG.md): 버전별 마일스톤 및 변경 이력 로그

---

## 6. 알려진 한계 및 면책 조항 (Limitations & Disclaimer)

- **비공식 연구 프로젝트:** 본 소프트웨어는 개인적 리버스 엔지니어링 연구 및 편의 제공 목적의 비공식 오픈소스 프로젝트이며, 주식회사 코에이 테크모 게임스(KOEI TECMO GAMES CO., LTD.)와 어떠한 제휴나 후원 관계도 없습니다.
- **저작권 보호:** 본 저장소에는 삼국지 13 게임 바이너리, 음원, 그래픽 리소스 등 코에이 테크모의 저작권 보호 자산이 일체 포함되어 있지 않으며 번들되지 않습니다.
- **세이브 데이터 백업 권장:** 실시간 메모리 패치 특성상 비정상적인 극단값 입력으로 인한 게임 크래시를 방지하기 위해 정기적인 인게임 세이브 백업을 권장합니다.
