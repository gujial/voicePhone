{
  description = "A voice chat application";

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
      in
      {
        devShell = pkgs.mkShell {
          nativeBuildInputs = with pkgs; [
            cmake
            ninja
            pkg-config
            qt6.wrapQtAppsHook
          ];

          buildInputs = with pkgs; [
            gcc
            stdenv.cc.cc.lib

            qt6.qtbase
            qt6.qtmultimedia
            qt6.qtwayland

            libopus

            gdb
            clang-tools
          ];

          QT_QPA_PLATFORM_PLUGIN_PATH = "${pkgs.qt6.qtbase}/lib/qt-6/plugins";
          QT_PLUGIN_PATH = "${pkgs.qt6.qtbase}/lib/qt-6/plugins";
        };
        package = pkgs.stdenv.mkDerivation {
          pname = "voicePhone";
          version = "1.0.0";
          src = self;
          nativeBuildInputs = with pkgs; [
            cmake
            ninja
            pkg-config
            qt6.wrapQtAppsHook
          ];
          buildInputs = with pkgs; [
            gcc
            qt6.qtbase
            qt6.qtmultimedia
            qt6.qtwayland
            libopus
          ];
          cmakeFlags = [ ];
          installPhase = ''
            mkdir -p $out/bin
            cp -r build/* $out/bin/
          '';
        };
      }
    );
}
