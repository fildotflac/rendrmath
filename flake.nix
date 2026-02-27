{
  description = "ImGui SDL3 Vulkan Nix Project";

  inputs = {
    nixpkgs.url = "github:nixos/nixpkgs/nixos-unstable";
    flake-utils.url = "github:numtide/flake-utils";
  };

  outputs = { self, nixpkgs, flake-utils }:
    flake-utils.lib.eachDefaultSystem (system:
      let
        pkgs = import nixpkgs { inherit system; };
      in
      {
        devShells.default = pkgs.mkShell {
          # Tools that run on your host during the build
          nativeBuildInputs = with pkgs; [
            cmake
            ninja
            pkg-config  # SDL3 often requires this to be present for CMake to find it
          ];

          # Libraries your project links against
          buildInputs = with pkgs; [
            sdl3
            vulkan-headers
            vulkan-loader
            vulkan-validation-layers
            libxkbcommon
            wayland
            xorg.libX11
          ];

          shellHook = ''
            # Force CMake to see the Nix-provided libraries
            export CMAKE_PREFIX_PATH="${pkgs.sdl3}:${pkgs.vulkan-loader}:$CMAKE_PREFIX_PATH"
            
            # Runtime paths for Vulkan and Wayland
            export LD_LIBRARY_PATH="$LD_LIBRARY_PATH:${pkgs.lib.makeLibraryPath [ pkgs.vulkan-loader pkgs.libxkbcommon pkgs.wayland ]}"
          '';
        };
      });
}