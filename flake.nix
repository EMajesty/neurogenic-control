{
  description = "NEUROGENIC RP2040 firmware development environment";
  inputs.nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";
  inputs.pico-sdk = { url = "git+https://github.com/raspberrypi/pico-sdk.git?ref=refs/tags/2.1.1&submodules=1"; flake = false; };
  outputs = { self, nixpkgs, pico-sdk }:
    let
      systems = [ "x86_64-linux" "aarch64-linux" ];
      eachSystem = f: nixpkgs.lib.genAttrs systems (system: f (import nixpkgs { inherit system; }));
    in {
      devShells = eachSystem (pkgs: {
        default = pkgs.mkShell {
          packages = with pkgs; [ cmake ninja clang gcc-arm-embedded python3 git picotool ];
          PICO_SDK_PATH = "${pico-sdk}";
          PICO_TOOLCHAIN_PATH = "${pkgs.gcc-arm-embedded}";
          # Pico SDK 2.1.1 pioasm omitted <cstdint>; GCC 16 no longer supplies it transitively.
          CXXFLAGS = "-include cstdint";
        };
        hardware = pkgs.mkShell { packages = with pkgs; [ python3 kicad ]; };
      });
    };
}
