# San13PK Linux Memory Editor & Trainer Suite

> 삼국지 13 with 파워업키트 (Romance of the Three Kingdoms 13 PK) Linux/Proton 전용 네이티브 실시간 메모리 에디터입니다.

---

## 1. 프로젝트 소개

본 프로젝트는 리눅스 환경(Steam Proton WoW64)에서 실행 중인 삼국지 13 PK 한국어판(v1.0.2)의 게임 메모리를 실시간으로 읽고 패치할 수 있는 독립 실행형(GUI) 에디터입니다.

- **모던 UI:** Dear ImGui(Docking 브랜치) + GLFW/OpenGL3 기반의 다크 테마 가변 그리드 UI.
- **네이티브 한글 지원:** 2벌식 한글 오토마타 엔진 내장 및 나눔고딕 폰트 렌더링.
- **실시간 핫싱크(Live Hot-Sync):** 세이브 파일 언팩/리팩 없이 게임 플레이 중 실시간 메모리 패치.
- **4개 탭 무장 심층 인스펙터:**
  - 능력치(통솔/무력/지력/정치 기본·현재치, 경험치, 병종 적성 C/B/A/S, 관직, 충성, 수명)
  - 141개 전법 마스터 테이블 콤보박스 및 20대 특기 0~9 레벨 슬라이더
  - 외모(Face ID 얼굴 CG), 성격, 3대 선호물, 음주, 물욕, 의리, 용애, 재애, 상성
  - 7대 혈통 및 인연 관계망 (부친, 모친, 배우자 1~3, 의형제/단짝, 원수 및 한글 성명 프리뷰)
- **60개 도시 자원 에디터:** 자금, 군량, 병력, 내구도, 민심 실시간 조정 및 원클릭 치트.
- **대규모 일괄 수정:** 다중 선택, 일괄 버프, 수명 일괄 연장, 실행 취소(Undo 롤백).

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

# 1. 메인 에디터 및 전체 단위 테스트 빌드
make -j$(nproc)

# 2. 12개 전체 단위 테스트 스위트 실행
make test

# 3. 19개 오프셋 발굴 및 리버스 엔지니어링 도구 일괄 빌드
make tools

# 4. Linux Standalone 배포 아카이브 패키징
make package
# 결과물: dist/san13_editor_linux_x86_64.tar.gz (약 1.3MB)
```

---

## 4. 실행 방법 (Usage)

1. Steam에서 **삼국지 13 PK**를 실행하고 시나리오에 진입합니다.
2. 터미널에서 에디터를 실행합니다:

```bash
./bin/san13_editor
```

3. 에디터가 게임 프로세스(`San13PK_kr.exe`)를 자동으로 감지하고 `RootBase` 주소를 바인딩합니다.
4. 무장 테이블에서 원하는 무장을 **더블클릭**하거나 우클릭하여 `[상세 능력치 편집...]`을 열고 실시간 수정을 적용합니다.

> **참고 (ptrace 권한 안내):**  
> Linux 보안 정책(`yama ptrace_scope`)에 따라 일반 유저 권한에서 타깃 프로세스의 메모리 읽기/쓰기가 차단될 경우, `sudo sysctl kernel.yama.ptrace_scope=0`을 실행하거나 `dist/run.sh` 런처를 이용하십시오.

---

## 5. 상세 기술 문서 안내 (Documentation)

자세한 역분석 결과 및 시스템 사양은 `docs/` 디렉터리를 참고하십시오:

- [docs/OFFSETS.md](docs/OFFSETS.md): 메모리 오프셋 및 구조체 규명 명세서 (검증 수준 표기)
- [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md): 시스템 아키텍처 및 디렉터리 구조
- [docs/VERIFICATION.md](docs/VERIFICATION.md): 단위 테스트 및 인게임 실측 증거 보고서
- [docs/CHANGELOG.md](docs/CHANGELOG.md): 마일스톤 이력 및 32개 EDGE QA 해결 내역

---

## 6. 알려진 한계 및 면책 조항 (Limitations & Disclaimer)

- **비공식 연구 프로젝트:** 본 소프트웨어는 개인적 리버스 엔지니어링 연구 및 편의 제공 목적의 비공식 오픈소스 프로젝트이며, 주식회사 코에이 테크모 게임스(KOEI TECMO GAMES CO., LTD.)와 어떠한 제휴나 후원 관계도 없습니다.
- **저작권 보호:** 본 저장소에는 삼국지 13 게임 바이너리, 음원, 그래픽 리소스 등 코에이 테크모의 저작권 보호 자산이 일체 포함되어 있지 않으며 번들되지 않습니다.
- **세이브 데이터 백업 권장:** 실시간 메모리 패치 특성상 비정상적인 극단값 입력으로 인한 게임 크래시를 방지하기 위해 정기적인 인게임 세이브 백업을 권장합니다.
