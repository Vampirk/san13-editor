CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -O2 -Iinclude -Ivendor/imgui -Ivendor/imgui/backends
LDFLAGS = 
UI_LDFLAGS = -lglfw -lGL -ldl

BIN_DIR = bin
BUILD_DIR = build
SRC_DIR = src
TEST_DIR = tests
IMGUI_DIR = vendor/imgui

# Source files
CORE_SRCS = $(SRC_DIR)/core/LinuxMemoryDriver.cpp $(SRC_DIR)/core/PlatformUtil.cpp
CORE_OBJS = $(patsubst $(SRC_DIR)/%.cpp, $(BUILD_DIR)/%.o, $(CORE_SRCS))

GAME_SRCS = $(SRC_DIR)/game/GameVersion.cpp $(SRC_DIR)/game/OfficerManager.cpp $(SRC_DIR)/game/OfficerNameTable.cpp $(SRC_DIR)/game/TacticManager.cpp $(SRC_DIR)/game/ForceManager.cpp $(SRC_DIR)/game/CityManager.cpp $(SRC_DIR)/game/SettlementManager.cpp $(SRC_DIR)/game/SettlementNameTable.cpp $(SRC_DIR)/game/ItemManager.cpp $(SRC_DIR)/game/ItemNameTable.cpp
GAME_OBJS = $(patsubst $(SRC_DIR)/%.cpp, $(BUILD_DIR)/%.o, $(GAME_SRCS))

UI_SRCS = $(SRC_DIR)/ui/FontHelper.cpp $(SRC_DIR)/ui/UIManager.cpp $(SRC_DIR)/ui/HangulAutomata.cpp $(SRC_DIR)/ui/OfficerTableView.cpp $(SRC_DIR)/ui/CityTableView.cpp $(SRC_DIR)/ui/CityDetailModal.cpp $(SRC_DIR)/ui/ItemTableView.cpp
UI_OBJS = $(patsubst $(SRC_DIR)/%.cpp, $(BUILD_DIR)/%.o, $(UI_SRCS))

IMGUI_SRCS = $(IMGUI_DIR)/imgui.cpp \
             $(IMGUI_DIR)/imgui_draw.cpp \
             $(IMGUI_DIR)/imgui_tables.cpp \
             $(IMGUI_DIR)/imgui_widgets.cpp \
             $(IMGUI_DIR)/backends/imgui_impl_glfw.cpp \
             $(IMGUI_DIR)/backends/imgui_impl_opengl3.cpp
IMGUI_OBJS = $(patsubst $(IMGUI_DIR)/%.cpp, $(BUILD_DIR)/imgui/%.o, $(IMGUI_SRCS))

ALL_HEADERS = $(wildcard include/core/*.hpp) $(wildcard include/game/*.hpp) $(wildcard include/ui/*.hpp)
ALL_OBJS = $(CORE_OBJS) $(GAME_OBJS)

# Test targets
TEST_MEMORY_SRC = $(TEST_DIR)/test_memory_driver.cpp
TEST_MEMORY_BIN = $(BIN_DIR)/test_memory_driver

TEST_BOUNDS_SRC = $(TEST_DIR)/test_bounds.cpp
TEST_BOUNDS_BIN = $(BIN_DIR)/test_bounds

TEST_OFFICER_SRC = $(TEST_DIR)/test_officer_manager.cpp
TEST_OFFICER_BIN = $(BIN_DIR)/test_officer_manager

TEST_CITY_SRC = $(TEST_DIR)/test_city_manager.cpp
TEST_CITY_BIN = $(BIN_DIR)/test_city_manager

TEST_SETTLEMENT_SRC = $(TEST_DIR)/test_settlement_manager.cpp
TEST_SETTLEMENT_BIN = $(BIN_DIR)/test_settlement_manager

TEST_CITY_OFFICERS_SRC = $(TEST_DIR)/test_city_officers.cpp
TEST_CITY_OFFICERS_BIN = $(BIN_DIR)/test_city_officers

TEST_GUI_SRC = $(TEST_DIR)/test_gui_window.cpp
TEST_GUI_BIN = $(BIN_DIR)/test_gui_window

TEST_OFFICER_TABLE_SRC = $(TEST_DIR)/test_officer_table.cpp
TEST_OFFICER_TABLE_BIN = $(BIN_DIR)/test_officer_table

TEST_OFFICER_EDIT_SRC = $(TEST_DIR)/test_officer_edit.cpp
TEST_OFFICER_EDIT_BIN = $(BIN_DIR)/test_officer_edit

TEST_CITY_TABLE_SRC = $(TEST_DIR)/test_city_table.cpp
TEST_CITY_TABLE_BIN = $(BIN_DIR)/test_city_table

TEST_CITY_MODAL_SRC = $(TEST_DIR)/test_city_detail_modal.cpp
TEST_CITY_MODAL_BIN = $(BIN_DIR)/test_city_detail_modal

TEST_BATCH_SRC = $(TEST_DIR)/test_batch_actions.cpp
TEST_BATCH_BIN = $(BIN_DIR)/test_batch_actions

TEST_HANGUL_SRC = $(TEST_DIR)/test_hangul_automata.cpp
TEST_HANGUL_BIN = $(BIN_DIR)/test_hangul_automata

TEST_HANGUL_WIDGET_SRC = $(TEST_DIR)/test_hangul_input_widget.cpp
TEST_HANGUL_WIDGET_BIN = $(BIN_DIR)/test_hangul_input_widget

TEST_OFFICER_NAMES_SRC = $(TEST_DIR)/test_officer_names.cpp
TEST_OFFICER_NAMES_BIN = $(BIN_DIR)/test_officer_names

TEST_TACTIC_SRC = $(TEST_DIR)/test_tactic_manager.cpp
TEST_TACTIC_BIN = $(BIN_DIR)/test_tactic_manager

TEST_MULTI_BONDS_SRC = $(TEST_DIR)/test_multi_bonds.cpp
TEST_MULTI_BONDS_BIN = $(BIN_DIR)/test_multi_bonds

TEST_FORCE_SRC = $(TEST_DIR)/test_force_manager.cpp
TEST_FORCE_BIN = $(BIN_DIR)/test_force_manager

TEST_ITEM_SRC = $(TEST_DIR)/test_item_manager.cpp
TEST_ITEM_BIN = $(BIN_DIR)/test_item_manager

TEST_PROTAGONIST_SRC = $(TEST_DIR)/test_protagonist.cpp
TEST_PROTAGONIST_BIN = $(BIN_DIR)/test_protagonist

APP_SRC = $(SRC_DIR)/main.cpp
APP_BIN = $(BIN_DIR)/san13_editor
DIST_DIR = dist
PACKAGE_DIR = $(DIST_DIR)/san13_editor_linux_x86_64
PACKAGE_TAR = $(DIST_DIR)/san13_editor_linux_x86_64.tar.gz
PACKAGE_WIN_DIR = $(DIST_DIR)/san13_editor_windows_x64
PACKAGE_WIN_ZIP = $(DIST_DIR)/san13_editor_windows_x64.zip

all: $(APP_BIN)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp $(ALL_HEADERS)
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(BUILD_DIR)/imgui/%.o: $(IMGUI_DIR)/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(TEST_MEMORY_BIN): $(CORE_OBJS) $(TEST_MEMORY_SRC)
	@mkdir -p $(BIN_DIR)
	$(CXX) $(CXXFLAGS) $(TEST_MEMORY_SRC) $(CORE_OBJS) -o $@ $(LDFLAGS)

$(TEST_BOUNDS_BIN): $(TEST_BOUNDS_SRC) include/game/Bounds.hpp
	@mkdir -p $(BIN_DIR)
	$(CXX) $(CXXFLAGS) $(TEST_BOUNDS_SRC) -o $@ $(LDFLAGS)

$(TEST_OFFICER_BIN): $(ALL_OBJS) $(TEST_OFFICER_SRC)
	@mkdir -p $(BIN_DIR)
	$(CXX) $(CXXFLAGS) $(TEST_OFFICER_SRC) $(ALL_OBJS) -o $@ $(LDFLAGS)

$(TEST_CITY_BIN): $(ALL_OBJS) $(TEST_CITY_SRC)
	@mkdir -p $(BIN_DIR)
	$(CXX) $(CXXFLAGS) $(TEST_CITY_SRC) $(ALL_OBJS) -o $@ $(LDFLAGS)

$(TEST_SETTLEMENT_BIN): $(ALL_OBJS) $(TEST_SETTLEMENT_SRC)
	@mkdir -p $(BIN_DIR)
	$(CXX) $(CXXFLAGS) $(TEST_SETTLEMENT_SRC) $(ALL_OBJS) -o $@ $(LDFLAGS)

$(TEST_CITY_OFFICERS_BIN): $(ALL_OBJS) $(TEST_CITY_OFFICERS_SRC)
	@mkdir -p $(BIN_DIR)
	$(CXX) $(CXXFLAGS) $(TEST_CITY_OFFICERS_SRC) $(ALL_OBJS) -o $@ $(LDFLAGS)

$(TEST_GUI_BIN): $(ALL_OBJS) $(UI_OBJS) $(IMGUI_OBJS) $(TEST_GUI_SRC)
	@mkdir -p $(BIN_DIR)
	$(CXX) $(CXXFLAGS) $(TEST_GUI_SRC) $(ALL_OBJS) $(UI_OBJS) $(IMGUI_OBJS) -o $@ $(UI_LDFLAGS)

$(TEST_OFFICER_TABLE_BIN): $(ALL_OBJS) $(UI_OBJS) $(IMGUI_OBJS) $(TEST_OFFICER_TABLE_SRC)
	@mkdir -p $(BIN_DIR)
	$(CXX) $(CXXFLAGS) $(TEST_OFFICER_TABLE_SRC) $(ALL_OBJS) $(UI_OBJS) $(IMGUI_OBJS) -o $@ $(UI_LDFLAGS)

$(TEST_OFFICER_EDIT_BIN): $(ALL_OBJS) $(UI_OBJS) $(IMGUI_OBJS) $(TEST_OFFICER_EDIT_SRC)
	@mkdir -p $(BIN_DIR)
	$(CXX) $(CXXFLAGS) $(TEST_OFFICER_EDIT_SRC) $(ALL_OBJS) $(UI_OBJS) $(IMGUI_OBJS) -o $@ $(UI_LDFLAGS)

$(TEST_CITY_TABLE_BIN): $(ALL_OBJS) $(UI_OBJS) $(IMGUI_OBJS) $(TEST_CITY_TABLE_SRC)
	@mkdir -p $(BIN_DIR)
	$(CXX) $(CXXFLAGS) $(TEST_CITY_TABLE_SRC) $(ALL_OBJS) $(UI_OBJS) $(IMGUI_OBJS) -o $@ $(UI_LDFLAGS)

$(TEST_CITY_MODAL_BIN): $(ALL_OBJS) $(UI_OBJS) $(IMGUI_OBJS) $(TEST_CITY_MODAL_SRC)
	@mkdir -p $(BIN_DIR)
	$(CXX) $(CXXFLAGS) $(TEST_CITY_MODAL_SRC) $(ALL_OBJS) $(UI_OBJS) $(IMGUI_OBJS) -o $@ $(UI_LDFLAGS)

$(TEST_BATCH_BIN): $(ALL_OBJS) $(UI_OBJS) $(IMGUI_OBJS) $(TEST_BATCH_SRC)
	@mkdir -p $(BIN_DIR)
	$(CXX) $(CXXFLAGS) $(TEST_BATCH_SRC) $(ALL_OBJS) $(UI_OBJS) $(IMGUI_OBJS) -o $@ $(UI_LDFLAGS)

$(TEST_HANGUL_BIN): $(BUILD_DIR)/ui/HangulAutomata.o $(IMGUI_OBJS) $(TEST_HANGUL_SRC)
	@mkdir -p $(BIN_DIR)
	$(CXX) $(CXXFLAGS) $(TEST_HANGUL_SRC) $(BUILD_DIR)/ui/HangulAutomata.o $(IMGUI_OBJS) -o $@ $(UI_LDFLAGS)

$(TEST_HANGUL_WIDGET_BIN): $(ALL_OBJS) $(UI_OBJS) $(IMGUI_OBJS) $(TEST_HANGUL_WIDGET_SRC)
	@mkdir -p $(BIN_DIR)
	$(CXX) $(CXXFLAGS) $(TEST_HANGUL_WIDGET_SRC) $(ALL_OBJS) $(UI_OBJS) $(IMGUI_OBJS) -o $@ $(UI_LDFLAGS)

$(TEST_OFFICER_NAMES_BIN): $(BUILD_DIR)/core/PlatformUtil.o $(BUILD_DIR)/game/OfficerNameTable.o $(BUILD_DIR)/game/TacticManager.o $(TEST_OFFICER_NAMES_SRC)
	@mkdir -p $(BIN_DIR)
	$(CXX) $(CXXFLAGS) $(TEST_OFFICER_NAMES_SRC) $(BUILD_DIR)/core/PlatformUtil.o $(BUILD_DIR)/game/OfficerNameTable.o $(BUILD_DIR)/game/TacticManager.o -o $@ $(LDFLAGS)

$(TEST_TACTIC_BIN): $(ALL_OBJS) $(TEST_TACTIC_SRC)
	@mkdir -p $(BIN_DIR)
	$(CXX) $(CXXFLAGS) $(TEST_TACTIC_SRC) $(ALL_OBJS) -o $@ $(LDFLAGS)

$(TEST_MULTI_BONDS_BIN): $(ALL_OBJS) $(TEST_MULTI_BONDS_SRC)
	@mkdir -p $(BIN_DIR)
	$(CXX) $(CXXFLAGS) $(TEST_MULTI_BONDS_SRC) $(ALL_OBJS) -o $@ $(LDFLAGS)

$(TEST_FORCE_BIN): $(ALL_OBJS) $(TEST_FORCE_SRC)
	@mkdir -p $(BIN_DIR)
	$(CXX) $(CXXFLAGS) $(TEST_FORCE_SRC) $(ALL_OBJS) -o $@ $(LDFLAGS)

$(TEST_ITEM_BIN): $(ALL_OBJS) $(TEST_ITEM_SRC)
	@mkdir -p $(BIN_DIR)
	$(CXX) $(CXXFLAGS) $(TEST_ITEM_SRC) $(ALL_OBJS) -o $@ $(LDFLAGS)

$(TEST_PROTAGONIST_BIN): $(ALL_OBJS) $(TEST_PROTAGONIST_SRC)
	@mkdir -p $(BIN_DIR)
	$(CXX) $(CXXFLAGS) $(TEST_PROTAGONIST_SRC) $(ALL_OBJS) -o $@ $(LDFLAGS)

$(APP_BIN): $(ALL_OBJS) $(UI_OBJS) $(IMGUI_OBJS) $(APP_SRC)
	@mkdir -p $(BIN_DIR)
	$(CXX) $(CXXFLAGS) $(APP_SRC) $(ALL_OBJS) $(UI_OBJS) $(IMGUI_OBJS) -o $@ $(UI_LDFLAGS)

TOOL_SRCS = $(wildcard tools/*.cpp)
TOOL_BINS = $(patsubst tools/%.cpp, $(BIN_DIR)/%, $(TOOL_SRCS))
TOOL_COMMON_OBJS = $(BUILD_DIR)/core/LinuxMemoryDriver.o $(BUILD_DIR)/core/PlatformUtil.o $(BUILD_DIR)/game/GameVersion.o $(BUILD_DIR)/game/OfficerNameTable.o $(BUILD_DIR)/game/TacticManager.o $(BUILD_DIR)/game/CityManager.o

$(BIN_DIR)/san13_tool: tools/san13_tool.c
	@mkdir -p $(BIN_DIR)
	gcc -O2 $< -o $@

$(BIN_DIR)/%: tools/%.cpp $(CORE_OBJS) $(GAME_OBJS)
	@mkdir -p $(BIN_DIR)
	$(CXX) $(CXXFLAGS) $< $(TOOL_COMMON_OBJS) -o $@ $(LDFLAGS)

tools: $(ALL_OBJS) $(TOOL_BINS) $(BIN_DIR)/san13_tool
	@echo "=== All Reverse-Engineering Tools Built Successfully in $(BIN_DIR)/ ==="

editor: $(APP_BIN)
	@echo "=== Starting San13PK Korean Memory Editor & Trainer ==="
	./$(APP_BIN)

package: $(APP_BIN)
	@echo "=== Building Standalone Linux Distribution Package ==="
	@rm -rf $(PACKAGE_DIR) $(PACKAGE_TAR)
	@mkdir -p $(PACKAGE_DIR)/assets/fonts
	@cp $(APP_BIN) $(PACKAGE_DIR)/
	@strip -s $(PACKAGE_DIR)/san13_editor
	@cp $(DIST_DIR)/run.sh $(PACKAGE_DIR)/
	@chmod +x $(PACKAGE_DIR)/run.sh
	@cp $(DIST_DIR)/san13_editor.desktop $(PACKAGE_DIR)/
	@cp $(DIST_DIR)/README_LINUX.md $(PACKAGE_DIR)/
	@cp assets/fonts/NanumGothic.ttf $(PACKAGE_DIR)/assets/fonts/
	@tar -czf $(PACKAGE_TAR) -C $(DIST_DIR) san13_editor_linux_x86_64
	@rm -rf $(PACKAGE_DIR)
	@echo "=== Package Created: $(PACKAGE_TAR) ==="
	@ls -lh $(PACKAGE_TAR)

package_win: $(WIN_BIN)
	@echo "=== Building Standalone Windows Distribution Package ==="
	@rm -rf $(PACKAGE_WIN_DIR) $(PACKAGE_WIN_ZIP)
	@mkdir -p $(PACKAGE_WIN_DIR)/assets/fonts
	@cp $(WIN_BIN) $(PACKAGE_WIN_DIR)/
	@x86_64-w64-mingw32-strip -s $(PACKAGE_WIN_DIR)/san13_editor.exe
	@cp $(DIST_DIR)/README_WINDOWS.txt $(PACKAGE_WIN_DIR)/
	@cp assets/fonts/NanumGothic.ttf $(PACKAGE_WIN_DIR)/assets/fonts/
	@cd $(DIST_DIR) && zip -q -r san13_editor_windows_x64.zip san13_editor_windows_x64
	@rm -rf $(PACKAGE_WIN_DIR)
	@echo "=== Windows Package Created: $(PACKAGE_WIN_ZIP) ==="
	@ls -lh $(PACKAGE_WIN_ZIP)

package_all: package package_win

test_memory: $(TEST_MEMORY_BIN)
	@echo "=== Running Phase 4.1 Core Memory Driver Unit Tests ==="
	./$(TEST_MEMORY_BIN)

test_bounds: $(TEST_BOUNDS_BIN)
	@echo "=== Running Phase 4.2 Value Bounds & QA Edge Case Tests ==="
	./$(TEST_BOUNDS_BIN)

test_officer: $(TEST_OFFICER_BIN)
	@echo "=== Running Phase 4.3 Officer Manager Domain Unit Tests ==="
	./$(TEST_OFFICER_BIN)

test_city: $(TEST_CITY_BIN)
	@echo "=== Running Phase 4.4 City Manager Domain Unit Tests ==="
	./$(TEST_CITY_BIN)

test_settlement: $(TEST_SETTLEMENT_BIN)
	@echo "=== Running Phase 3.1 Settlement Manager Domain Unit Tests ==="
	./$(TEST_SETTLEMENT_BIN)

test_city_officers: $(TEST_CITY_OFFICERS_BIN)
	@echo "=== Running Phase 3.2 City Officer Linked-List Unit Tests ==="
	./$(TEST_CITY_OFFICERS_BIN)

test_gui: $(TEST_GUI_BIN)
	@echo "=== Running Phase 5.1 Dear ImGui GUI Pipeline Smoke Test ==="
	./$(TEST_GUI_BIN) --smoke-test

test_officer_table: $(TEST_OFFICER_TABLE_BIN)
	@echo "=== Running Phase 5.2 Officer Table View & Virtual Scroll Test ==="
	./$(TEST_OFFICER_TABLE_BIN) --smoke-test

test_officer_edit: $(TEST_OFFICER_EDIT_BIN)
	@echo "=== Running Phase 5.3 Officer Stat Editing & Hot-Sync Test ==="
	./$(TEST_OFFICER_EDIT_BIN) --smoke-test

test_city_table: $(TEST_CITY_TABLE_BIN)
	@echo "=== Running Phase 5.4 City Resource Editor & Table View Test ==="
	./$(TEST_CITY_TABLE_BIN) --smoke-test

test_city_modal: $(TEST_CITY_MODAL_BIN)
	@echo "=== Running Phase 4 4-Tab City Inspector & Direct Editing Tests ==="
	./$(TEST_CITY_MODAL_BIN) --smoke-test

test_batch: $(TEST_BATCH_BIN)
	@echo "=== Running Phase 5.5 Batch Actions & Multi-Select Test ==="
	./$(TEST_BATCH_BIN) --smoke-test

test_hangul: $(TEST_HANGUL_BIN)
	@echo "=== Running Phase 5.6 Native Hangul 2-Set Automata Unit Tests ==="
	./$(TEST_HANGUL_BIN)

test_hangul_widget: $(TEST_HANGUL_WIDGET_BIN)
	@echo "=== Running Phase 5.8 Hangul Input ImGui Integration Test ==="
	./$(TEST_HANGUL_WIDGET_BIN)

test_officer_names: $(TEST_OFFICER_NAMES_BIN)
	@echo "=== Running Phase 5.7 Officer Korean Name Table Unit Tests ==="
	./$(TEST_OFFICER_NAMES_BIN)

test_tactic: $(TEST_TACTIC_BIN)
	@echo "=== Running Phase 1 Custom Tactic Unit Tests ==="
	./$(TEST_TACTIC_BIN)

test_tactic_live: $(TEST_TACTIC_BIN)
	@echo "=== Running Phase 1 Custom Tactic Live Write/Restore Tests ==="
	./$(TEST_TACTIC_BIN) --live

test_multi_bonds: $(TEST_MULTI_BONDS_BIN)
	@echo "=== Running Phase 2 Multi-Bonds Unit Tests ==="
	./$(TEST_MULTI_BONDS_BIN)

test_force: $(TEST_FORCE_BIN)
	@echo "=== Running Phase 3 Force Manager Unit Tests ==="
	./$(TEST_FORCE_BIN)

test_item: $(TEST_ITEM_BIN)
	@echo "=== Running Phase 5 Item Manager Domain Unit Tests ==="
	./$(TEST_ITEM_BIN)

test_protagonist: $(TEST_PROTAGONIST_BIN)
	@echo "=== Running Phase 6.6 Protagonist Domain Unit Tests ==="
	./$(TEST_PROTAGONIST_BIN)

test_offline: test_bounds test_hangul test_hangul_widget test_officer_names test_tactic
	@echo "=== All Offline Unit & Integration Tests Passed (No Live Process Required) ==="

test_live: test_memory test_officer test_city test_settlement test_city_officers test_city_modal test_gui test_officer_table test_officer_edit test_city_table test_batch test_multi_bonds test_force test_item test_protagonist test_tactic_live
	@echo "=== All Live In-Game Memory Tests Passed (Active Process Verified) ==="

test: test_offline test_live

gui: $(TEST_GUI_BIN)
	./$(TEST_GUI_BIN)

clean:
	rm -rf $(BUILD_DIR) $(BIN_DIR)/test_memory_driver $(BIN_DIR)/test_bounds $(BIN_DIR)/test_officer_manager $(BIN_DIR)/test_city_manager $(BIN_DIR)/test_settlement_manager $(BIN_DIR)/test_city_officers $(BIN_DIR)/test_city_detail_modal $(BIN_DIR)/test_gui_window $(BIN_DIR)/test_officer_table $(BIN_DIR)/test_officer_edit $(BIN_DIR)/test_city_table $(BIN_DIR)/test_batch_actions $(BIN_DIR)/test_hangul_automata $(BIN_DIR)/test_hangul_input_widget $(BIN_DIR)/test_officer_names $(TEST_TACTIC_BIN) $(TEST_MULTI_BONDS_BIN) $(TEST_FORCE_BIN) $(TEST_ITEM_BIN) $(APP_BIN) $(PACKAGE_DIR) $(PACKAGE_TAR) $(PACKAGE_WIN_DIR) $(PACKAGE_WIN_ZIP)

.PHONY: all test_memory test_bounds test_officer test_city test_settlement test_city_officers test_city_modal test_gui test_officer_table test_officer_edit test_city_table test_batch test_hangul test_hangul_widget test_officer_names test_tactic test_multi_bonds test_force test_item test test_offline test_live gui editor package package_win package_all tools clean

# ---------------------------------------------------------------------------
# Windows 크로스 컴파일 (MinGW-w64): make win  ->  bin/win/san13_editor.exe
# GLFW는 vendor/glfw 소스를 직접 컴파일 (CMake 불필요)
# ---------------------------------------------------------------------------
WIN_CXX = x86_64-w64-mingw32-g++
WIN_CC  = x86_64-w64-mingw32-gcc
WIN_BUILD = build/win
WIN_BIN = bin/win/san13_editor.exe
WIN_CXXFLAGS = -std=c++17 -Wall -Wextra -O2 -Iinclude -Ivendor/imgui -Ivendor/imgui/backends -Ivendor/glfw/include
WIN_CFLAGS = -O2 -D_GLFW_WIN32 -Ivendor/glfw/include -Wno-unused-parameter
WIN_LDFLAGS = -static -static-libgcc -static-libstdc++ -lgdi32 -lopengl32 -lwinmm -lole32 -lpsapi

WIN_GLFW_SRCS = context init input monitor platform vulkan window egl_context osmesa_context null_init null_monitor null_window null_joystick win32_module win32_time win32_thread win32_init win32_joystick win32_monitor win32_window wgl_context
WIN_GLFW_OBJS = $(addprefix $(WIN_BUILD)/glfw/, $(addsuffix .o, $(WIN_GLFW_SRCS)))

WIN_CORE_SRCS = src/core/Win32MemoryDriver.cpp src/core/PlatformUtil.cpp
WIN_OBJS = $(patsubst src/%.cpp, $(WIN_BUILD)/%.o, $(WIN_CORE_SRCS) $(GAME_SRCS) $(UI_SRCS) $(APP_SRC)) \
           $(patsubst $(IMGUI_DIR)/%.cpp, $(WIN_BUILD)/imgui/%.o, $(IMGUI_SRCS)) $(WIN_GLFW_OBJS)

win: $(WIN_BIN)

$(WIN_BUILD)/glfw/%.o: vendor/glfw/src/%.c
	@mkdir -p $(dir $@)
	$(WIN_CC) $(WIN_CFLAGS) -c $< -o $@

$(WIN_BUILD)/imgui/%.o: $(IMGUI_DIR)/%.cpp
	@mkdir -p $(dir $@)
	$(WIN_CXX) $(WIN_CXXFLAGS) -c $< -o $@

$(WIN_BUILD)/%.o: src/%.cpp $(ALL_HEADERS)
	@mkdir -p $(dir $@)
	$(WIN_CXX) $(WIN_CXXFLAGS) -c $< -o $@

$(WIN_BIN): $(WIN_OBJS)
	@mkdir -p $(dir $@)
	$(WIN_CXX) $(WIN_OBJS) -o $@ $(WIN_LDFLAGS)

.PHONY: win
