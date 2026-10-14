# 검증 및 실측 증거 보고서 (VERIFICATION)

> **기준 판본:** v6.0  
> **최종 갱신일:** 2026-10-14  
> **실측 타깃:** Romance of the Three Kingdoms 13 PK (San13PK_kr.exe, 32-bit PE)  
> **실행 환경:** Fedora Linux x86_64, Valve GE-Proton, PID 182747, RootBase `0x73B6C5D8`  

---

## 1. 검증 유형의 명확한 구분 (Important Distinction)

본 프로젝트의 검증은 다음 두 단계로 엄격히 구분하여 해석해야 합니다:

1. **메모리 드라이버 쓰기 검증 (Driver Memory Write):**
   - 에디터가 Linux 시스템 콜(`process_vm_writev` 또는 `/proc/$PID/mem`)을 통해 타깃 프로세스의 가상 메모리 바이트를 성공적으로 변경했는지를 나타냅니다.
   - 단위 테스트 및 핫싱크 로그에 표시되는 소요 시간(예: `0.005ms`)은 **에디터 내부의 함수 호출 및 메모리 복사 소요 시간**일 뿐이며, 삼국지 13 게임 엔진 내부 렌더러가 프레임을 갱신하거나 UI를 다시 그리는 시점(V-Sync, 렌더 루프 주기)을 의미하지 않습니다.
2. **인게임 엔진 반영 검증 (Live In-Game Reflection):**
   - 실제로 게임 창을 열어 무장 정보창, 세력도, 전법 선택창, 전투 컷인 일러스트 등에서 변경된 값이 시각적·로직적으로 반영되었는지를 인간 사용자가 직접 확인한 검증입니다.
   - 게임 엔진이 자체적으로 캐싱하고 있는 창(예: 이미 열려 있는 무장 상세창)은 창을 닫았다가 다시 열어야 갱신됩니다(EDGE-16).

---

## 2. 단위 테스트 스위트 및 타깃 분리 (20개 테스트 요약)

테스트는 게임 프로세스 의존성에 따라 `make test_offline`과 `make test_live`로 분리되어 관리됩니다:

| 번호 | 바이너리 | 대상 모듈 | 주요 검증 내용 | Makefile 타깃 | 프로세스 의존 |
| :---: | :--- | :--- | :--- | :---: | :---: |
| 1 | `bin/test_bounds` | `BoundsValidator` | Safe/LimitBreak 클램핑, 포화 연산, 연도 역전 방지 | `test_offline` | **불필요 (오프라인 통과)** |
| 2 | `bin/test_hangul_automata` | `HangulAutomata` | 2벌식 조합, 연음, 1자모 백스페이스 역분해 (6/6) | `test_offline` | **불필요 (오프라인 통과)** |
| 3 | `bin/test_hangul_input_widget` | `HangulInputText` | ImGui 입력 위젯 연동, 대소문자 무시 검색 (헤드리스) | `test_offline` | **불필요 (오프라인 통과)** |
| 4 | `bin/test_officer_names` | `OfficerNameTable` | 700인 성명, 고대무장, 상태 진리표, 전법/특기/인연 (11/11) | `test_offline` | **불필요 (오프라인 통과)** |
| 5 | `bin/test_tactic_manager` | `TacticManager` | 바닐라 141종 전법 명칭, `editdata_pk.s13` 등록 전법(ID 250~) 로드 | `test_offline` | 불필요 (⚠ 게임 실행 중이면 라이브 쓰기·원복 수행, TECH-04) |
| 6 | `bin/test_memory_driver` | `LinuxMemoryDriver` | 프로세스 바인딩, 포인터 체인 연산, 패턴 스캔 | `test_live` | 필요 (`San13PK_kr.exe`) |
| 7 | `bin/test_officer_manager` | `OfficerManager` | 800명 무장 로드, 스탯 읽기/쓰기 클램핑 | `test_live` | 필요 (`San13PK_kr.exe`) |
| 8 | `bin/test_city_manager` | `CityManager` | 60개 도시 자원 읽기/쓰기, 내구도 오버플로 방지 | `test_live` | 필요 (`San13PK_kr.exe`) |
| 9 | `bin/test_gui_window` | `UIManager` | GLFW 창 생성, OpenGL3 렌더 루프, 폰트 로드 | `test_live` | 필요 (GUI 스모크 120F) |
| 10 | `bin/test_officer_table` | `OfficerTableView` | 가상 스크롤, 정렬, 검색, EDGE-32 재야 판정식 | `test_live` | 필요 (GUI 스모크 120F) |
| 11 | `bin/test_officer_edit` | `OfficerTableView` | 인라인/모달 편집, 실시간 핫싱크 커밋, 복구 | `test_live` | 필요 (GUI 스모크 120F) |
| 12 | `bin/test_city_table` | `CityTableView` | 도시 가상 스크롤, 자원 수정 및 치트 | `test_live` | 필요 (GUI 스모크 120F) |
| 13 | `bin/test_batch_actions` | `OfficerTableView` | 50인 일괄 버프/수명 연장, 트랜잭션 롤백 | `test_live` | 필요 (GUI 스모크 120F) |
| 14 | `bin/test_settlement_manager` | `SettlementManager` | 300개 부락 로드, 도시 관할 바인딩, 부락 지명 매핑 | `test_live` | 필요 (`San13PK_kr.exe`) |
| 15 | `bin/test_city_officers` | `CityManager` | 도시 소속/재야/포로 무장 연결 리스트 순회, 순환 방지 가드 | `test_live` | 필요 (`San13PK_kr.exe`) |
| 16 | `bin/test_city_detail_modal` | `CityDetailModal` | 4탭 인스펙터 내정/숙련도/부락 쓰기, Safe 클램핑, 원복 | `test_live` | 필요 (GUI 스모크 120F) |
| 17 | `bin/test_multi_bonds` | `OfficerManager` | 의형제 3 / 친애 6 / 원수 3 다중 인연 슬롯 읽기·쓰기 및 원복 | `test_live` | 필요 (`San13PK_kr.exe`) |
| 18 | `bin/test_force_manager` | `ForceManager` | 세력 청크 파싱, 190년 시나리오 군주 실명 매핑 | `test_live` | 필요 (`San13PK_kr.exe`) |
| 19 | `bin/test_item_manager` | `ItemManager` | `CItem` 36B 구조체, 234종 명칭, 라이브 소유자/소재지 판독 | `test_live` | 필요 (`San13PK_kr.exe`) |
| 20 | `bin/test_protagonist` | `OfficerManager` | 주인공 감지(VTable RVA), 소지금 쓰기 후 무조건 원복 | `test_live` | 필요 (`San13PK_kr.exe`, 세이브 무관) |

- **`make test_offline`**: CI 환경이나 게임이 미실행 중일 때도 100% 통과할 수 있는 핵심 도메인 로직/오토마타/규격 단위 테스트.
- **`make test_live`**: Proton 환경에서 실제 게임이 구동 중일 때 `/proc/$PID/mem` 접근 및 메모리 핫싱크 무결성을 검증하는 실시간 테스트.
- **`make test`**: 두 타깃을 순차적으로 전수 실행.

> **주의 사항**
> - 라이브 테스트는 모두 `LinuxMemoryDriver`를 직접 사용하므로 **Linux(Proton) 환경 전용**입니다. Windows 빌드는 `--smoke-test` 실행 검증(3.6절)으로 대체합니다.
> - 라이브 테스트는 실제 게임 메모리에 쓴 뒤 원본 스냅샷으로 원복합니다(EDGE-36). 테스트 도중 강제 중단 시 원복이 누락될 수 있으므로 **세이브 백업 후 실행**하십시오.
> - 일부 라이브 테스트는 190년 「반동탁 연합」 시나리오와 주인공 강월(ID 700) 세이브를 전제로 작성되어 있습니다.

---

## 3. 인게임 실시간 검증 완료 항목 (Live In-Game Verified)

사용자가 실제 게임 플레이 화면에서 육안으로 확인한 검증 실측치입니다:

### 3.1. 전법 (Tactics) 핫패치 검증
- **검증 1: 항적 (ID 857)**
  - 변경 전: `8` (고금무쌍, 소모 7pt)
  - 변경 후: `0` (천하무쌍, 소모 7pt)
  - 인게임 확인: 무장 정보창 및 전투 화면에서 "천하무쌍"으로 정상 변경 확인.
- **검증 2: 조조 (ID 333)**
  - 변경 전: `44` (용략지휘, 소모 6pt)
  - 변경 후: `2` (군신, 소모 7pt)
  - 인게임 확인: 조조의 고유 전법이 "군신"으로 정상 교체 확인.

### 3.2. 얼굴 CG (Face ID) 핫패치 검증
- **검증 대상: 조조 (ID 333)**
  - 변경 전: Face ID `405` (조조 고유 일러스트)
  - 변경 후: Face ID `117` (관우 고유 일러스트)
  - 인게임 확인: 게임 재시작 없이 메모리 패치 즉시 조조의 얼굴이 관우 얼굴로 교체되어 표출됨 확인.

### 3.3. 8대 성향/가치관/기호 검증 (24인 표본)
- 관우(94): 무기 선호(1), 서적 무관심(0), 보물 무관심(0), 대음(2), 무욕(0), 의리 매우중시(4), 용애 중시(3), 재애 보통(2), 상성 50.
- 여포(635): 대음(2), 강욕(2), 의리 경시(0), 용애 매우중시(4), 재애 경시(1), 상성 0.
- 장각(412): 금주(0), 무욕(0), 의리 매우중시(4).
- 24인 무장 메모리 덤프와 인게임 무장 정보창이 100% 일치함을 확인.

### 3.4. 가족 및 인연 관계망 검증
- 조조(333): 부친 1048(조숭), 배우자 1 675(변씨), 배우자 2~3 0xFFFF.
- 제갈량(283): 부친 1045(제갈규), 배우자 1 654(황월영).
- 손책(359): 부친 355(손견), 모친 1101(오씨), 단짝 241(주유).
- 관우(94), 장비(418): 의형제 리더 610(유비).

### 3.5. 도시 내정 3대치, 병과 숙련도, 소속 인물 통계 검증 (Phase 1)
- **검증 대상 1: 양평 (ID 0)**
  - 민심: 100 (`+0x0D2`, 화면 100 일치) [확정]
  - 상업 기본치: 1,000 (`+0x0D6`, 화면 1,000 일치) [유력]
  - 농업 기본치: 1,050 (`+0x0E0`, 화면 1,050 일치) [확정]
  - 문화 기본치: 1,000 (`+0x0EA`, 화면 1,000 일치) [확정]
  - 창병 숙련도: 200 (`+0x0FC`, 화면 200 일치) [확정]
  - 기병 숙련도: 300 (`+0x0FE`, 화면 300 일치) [확정]
  - 궁병 숙련도: 200 (`+0x100`, 화면 200 일치) [확정]
- **검증 대상 2: 금성 (ID 23)**
  - 민심: 350 (`+0x0D2`, 화면 350 일치) [확정]
  - 상업 기본치: 1,800 (`+0x0D6`, 메모리 1,800 + 마을 보너스 500 = 화면 2,300 일치) [유력]
  - 농업 기본치: 2,000 (`+0x0E0`, 화면 2,000 일치) [확정]
  - 문화 기본치: 2,400 (`+0x0EA`, 화면 2,400 일치) [확정]
  - 창병 숙련도: 700 (`+0x0FC`, 화면 700 일치) [확정]
  - 기병 숙련도: 700 (`+0x0FE`, 화면 700 일치) [확정]
  - 궁병 숙련도: 700 (`+0x100`, 화면 700 일치) [확정]
  - 소속 무장 수: 12명 (`+0x144`, 화면 12명 일치) [유력]
  - 수감 포로 수: 0명 (`+0x14C`, 화면 0명 일치) [유력]
  - 재야 무장 수: 1명 (`+0x154`, 화면 1명 일치) [유력]
- **단위 테스트 통과:** `bin/test_city_manager` [Step 3-2] 17개 전수 검증 통과 및 `San13CityRaw` `sizeof == 0x170` 컴파일타임 static_assert 보장.

### 3.6. 듀얼 플랫폼 바이너리 및 배포 패키지 실측 검증 (Phase 6.8)
- **리눅스 네이티브 배포 패키지 (`dist/san13_editor_linux_x86_64.tar.gz` / 1.4MB):**
  - 독립 디렉터리 압축 해제 후 `./run.sh --smoke-test` 단독 실행 검증.
  - 실행 중인 `San13PK_kr.exe` 자동 감지, `RootBase (0x73b6c5d8)` 바인딩, 나눔고딕 폰트 로드 및 120프레임 무결성 통과.
- **윈도우 네이티브 배포 패키지 (`dist/san13_editor_windows_x64.zip` / 1.7MB):**
  - MinGW-w64 정적 스트립 바이너리(`san13_editor.exe`, 2.5MB) + 한글 폰트 + 안내서 구성.
  - GE-Proton WoW64 가상 윈도우 환경에서 `proton run san13_editor.exe --smoke-test` 실행 검증.
  - Win32 API(`EnumProcessModulesEx`, `OpenProcess`, `ReadProcessMemory`) 기반으로 게임 프로세스 정상 바인딩 및 120프레임 무결성 통과.

---

## 4. 미확인 및 추가 관찰 필요 항목 (Unverified)

안정적인 운영을 위해 아직 완전히 확정되지 않은 항목을 투명하게 명시합니다:

1. **배우자 2 및 배우자 3 인게임 다처(多妻) 반영 여부:**
   - 오프셋 `+0x46`, `+0x48`에 값을 썼을 때 게임 엔진이 2번째, 3번째 배우자를 인게임 관계창에 정상 표시하는지는 추가 시나리오 관찰 필요.
2. **`+0x14` 미확인 1바이트 플래그:**
   - 0 또는 1 등으로 변화하나 구체적인 엔진 플래그 의미 미확정.
3. **중신특성 (`+0x3A`, `+0x3B`) ID 인게임 발동 여부:**
   - 오프셋은 특정되었으나 실제 중신 임명 시 효과 부여 여부는 미확인.
4. **시나리오 변경 시 일자 오프셋 불변 여부:**
   - 190년 시나리오 외 184년, 208년, 225년 시나리오에서도 `+0x137A` 일자 오프셋이 동일한지는 추가 확인 필요.
