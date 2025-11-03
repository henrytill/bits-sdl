{
  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixpkgs-unstable";
    flake-utils.url = "github:numtide/flake-utils";
  };

  outputs =
    {
      self,
      nixpkgs,
      flake-utils,
      ...
    }:
    let
      name = "bits-sdl";
      src = self;
      overlay = final: prev: {
        bits-sdl = final.stdenv.mkDerivation {
          inherit name src;
          nativeBuildInputs = with final; [
            pkg-config
          ];
          buildInputs = with final; [
            freetype
            lua5_1
            SDL2
          ];
          doCheck = true;
          installFlags = [ "DESTDIR=${placeholder "out"}" ];
        };
      };
    in
    flake-utils.lib.eachDefaultSystem (
      system:
      let
        pkgs = import nixpkgs {
          inherit system;
          overlays = [ overlay ];
        };
      in
      {
        packages.bits-sdl = pkgs.bits-sdl;
        packages.default = self.packages.${system}.bits-sdl;
        devShell = pkgs.mkShell {
          inputsFrom = [ pkgs.bits-sdl ];
          packages = with pkgs; [
            clang-tools
            yaml-language-server
          ];
        };
      }
    );
}
