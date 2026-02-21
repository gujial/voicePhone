{
  description = "A voice chat application";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";
    flake-utils.url = "github:numtide/flake-utils";
  };

  outputs = { self, nixpkgs, flake-utils }:
    flake-utils.lib.eachDefaultSystem (system:
      let
        pkgs = import nixpkgs { inherit system; };
      in
      {
        # Define the package
        packages.default = pkgs.stdenv.mkDerivation {
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
            qt6.qtbase
            qt6.qtmultimedia
            qt6.qtwayland
            libopus
          ];
        };

        # Define the development shell
        devShells.default = pkgs.mkShell {
          nativeBuildInputs = with pkgs; [
            cmake
            ninja
            pkg-config
            qt6.wrapQtAppsHook
          ];
          buildInputs = with pkgs; [
            qt6.qtbase
            qt6.qtmultimedia
            qt6.qtwayland
            libopus
            gdb
            clang-tools
          ];
          
          # These help Qt find plugins during development
          shellHook = ''
            export QT_QPA_PLATFORM_PLUGIN_PATH="${pkgs.qt6.qtbase}/lib/qt-6/plugins"
            export QT_PLUGIN_PATH="${pkgs.qt6.qtbase}/lib/qt-6/plugins"
          '';
        };
      }
    );
}