{
  description = "A voice chat application (Client and Server)";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";
    flake-utils.url = "github:numtide/flake-utils";
  };

  outputs =
    {
      self,
      nixpkgs,
      flake-utils,
    }:
    flake-utils.lib.eachDefaultSystem (
      system:
      let
        pkgs = import nixpkgs { inherit system; };
        inherit (pkgs) lib stdenv;

        # 提取公共依赖，方便 package 和 devShell 复用
        buildDeps = with pkgs; [
          qt6.qtbase
          qt6.qtmultimedia
          openssl         # 对应 CMake 中的 OpenSSL
          libopus         # 对应 CMake 中的 opus
        ] ++ lib.optional stdenv.isLinux pkgs.qt6.qtwayland; # 仅 Linux 需要 Wayland 支持

        nativeDeps = with pkgs; [
          cmake
          ninja
          pkg-config
          qt6.wrapQtAppsHook
        ];
      in
      {
        packages = {
          # 由于 CMakeLists.txt 同时构建了两个目标，
          # 运行这个 package 会在 result/bin 目录下生成 voicephone 和 voicephone-server
          voicephone = stdenv.mkDerivation {
            pname = "voicephone";
            version = "1.0.0";
            src = self;

            nativeBuildInputs = nativeDeps;
            buildInputs = buildDeps;

            # 如果需要在 macOS 上构建特殊的 .app 捆绑包，可以在此配置
            # 但 standard cmake install 在 Nix 下通常会直接放入 bin/
            cmakeFlags = [
              "-DCMAKE_BUILD_TYPE=Release"
            ];
          };
          default = self.packages.${system}.voicephone;
        };

        devShells.default = pkgs.mkShell {
          nativeBuildInputs = nativeDeps;
          buildInputs = buildDeps ++ (with pkgs; [
            gdb
            clang-tools
          ]);

          # 环境变量设置
          shellHook = ''
            export QT_QPA_PLATFORM_PLUGIN_PATH="${pkgs.qt6.qtbase}/lib/qt-6/plugins"
            export QT_PLUGIN_PATH="${pkgs.qt6.qtbase}/lib/qt-6/plugins"
          '' + lib.optionalString stdenv.isLinux ''
            # Linux 特有配置，防止 Wayland 下某些 Qt 插件找不到
            export QT_WAYLAND_SHELL_INTEGRATION=xdg-shell
          '';
        };
      }
    );
}