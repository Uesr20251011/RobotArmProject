QT += core gui widgets
TEMPLATE = app
TARGET = RobotArmUI
CONFIG += c++17 debug_and_release
!contains(QMAKE_COMPILER, msvc):error("Select the Qt MSVC x64 kit, then use scripts/Generate-VS.ps1.")
!contains(QT_ARCH, x86_64):error("RobotArmProject requires x64.")

SOURCES += main.cpp MainWindow.cpp
HEADERS += MainWindow.h
FORMS += MainWindow.ui
QMAKE_CXXFLAGS += /utf-8 /DNOMINMAX
CONFIG(debug, debug|release): BUILD_CONFIG = Debug
else: BUILD_CONFIG = Release
DESTDIR = $$clean_path($$PWD/../build/bin/x64/$$BUILD_CONFIG)
MOC_DIR = $$clean_path($$PWD/../build/qt/$$BUILD_CONFIG/moc)
UI_DIR = $$clean_path($$PWD/../build/qt/$$BUILD_CONFIG/ui)
OBJECTS_DIR = $$clean_path($$PWD/../build/qt/$$BUILD_CONFIG/obj)
RCC_DIR = $$clean_path($$PWD/../build/qt/$$BUILD_CONFIG/rcc)

# Native DLL includes, imports and runtime copying come from RobotArmUI.props.
# Generate-VS.ps1 attaches that property sheet and native project references.
