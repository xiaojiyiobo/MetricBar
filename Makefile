CXX := x86_64-w64-mingw32-g++-win32
WINDRES := x86_64-w64-mingw32-windres
STRIP := x86_64-w64-mingw32-strip
BUILD_DIR ?= /mnt/d/AI-Projects/02_Builds/MetricBar
CPPFLAGS := -DUNICODE -D_UNICODE -D_WIN32_WINNT=0x0601
CXXFLAGS := -std=c++17 -O2 -DNDEBUG -Wall -Wextra -Wpedantic
LDFLAGS := -static -Wl,--kill-at,--no-insert-timestamp,--subsystem,windows
LIBS := -lole32 -luuid -lshell32 -ladvapi32 -lwinhttp -lcomctl32 -luxtheme -lgdi32 -luser32

.PHONY: all tests clean

all: $(BUILD_DIR)/VpsTraySpeed.dll

$(BUILD_DIR):
	mkdir -p "$@"

$(BUILD_DIR)/VpsTraySpeed.res: src/VpsTraySpeed.rc | $(BUILD_DIR)
	$(WINDRES) "$<" -O coff -o "$@"

$(BUILD_DIR)/VpsTraySpeed.dll: src/DeskBand.cpp src/NetParsing.cpp src/NetParsing.h src/VpsTraySpeed.def $(BUILD_DIR)/VpsTraySpeed.res
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -shared src/DeskBand.cpp src/NetParsing.cpp \
		$(BUILD_DIR)/VpsTraySpeed.res src/VpsTraySpeed.def -o "$@" $(LDFLAGS) $(LIBS)
	$(STRIP) "$@"

tests: $(BUILD_DIR)/NetParsingTests.exe $(BUILD_DIR)/ComContractTests.exe $(BUILD_DIR)/IntegrationHostTests.exe

$(BUILD_DIR)/NetParsingTests.exe: tests/NetParsingTests.cpp src/NetParsing.cpp src/NetParsing.h | $(BUILD_DIR)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) tests/NetParsingTests.cpp src/NetParsing.cpp \
		-o "$@" -static -Wl,--no-insert-timestamp
	$(STRIP) "$@"

$(BUILD_DIR)/ComContractTests.exe: tests/ComContractTests.cpp | $(BUILD_DIR)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) tests/ComContractTests.cpp -o "$@" \
		-static -Wl,--no-insert-timestamp -lole32 -luuid
	$(STRIP) "$@"

$(BUILD_DIR)/IntegrationHostTests.exe: tests/IntegrationHostTests.cpp | $(BUILD_DIR)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) tests/IntegrationHostTests.cpp -o "$@" \
		-static -Wl,--no-insert-timestamp -lole32 -luuid -lpsapi -lgdi32
	$(STRIP) "$@"

clean:
	rm -f "$(BUILD_DIR)/VpsTraySpeed.dll" "$(BUILD_DIR)/VpsTraySpeed.res" \
		"$(BUILD_DIR)/NetParsingTests.exe" "$(BUILD_DIR)/ComContractTests.exe" \
		"$(BUILD_DIR)/IntegrationHostTests.exe"
