{ pkgs ? import <nixpkgs> {} }:

pkgs.mkShell {
  nativeBuildInputs = with pkgs; [
    # 构建工具
    cmake
    ninja
    pkg-config
    qt6.wrapQtAppsHook
  ];

  buildInputs = with pkgs; [
    # C++ 编译器和标准库
    gcc
    stdenv.cc.cc.lib
    
    # Qt6 依赖
    qt6.qtbase
    qt6.qtmultimedia
    qt6.qtwayland
    
    # 开发工具（可选）
    gdb
    clang-tools
  ];

  # 设置 Qt 相关环境变量
  QT_QPA_PLATFORM_PLUGIN_PATH = "${pkgs.qt6.qtbase}/lib/qt-6/plugins";
  QT_PLUGIN_PATH = "${pkgs.qt6.qtbase}/lib/qt-6/plugins";
}
