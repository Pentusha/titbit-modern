{
  description = "Ядерный Титбит: native Linux engine and data import tools";

  inputs.nixpkgs.url = "github:NixOS/nixpkgs/nixos-26.05";

  outputs =
    { self, nixpkgs }:
    let
      # The engine preserves the original 32-bit x86 object layout and ABI.
      # Keep the Python tools independent when adding other host platforms.
      forAllSystems = nixpkgs.lib.genAttrs [ "x86_64-linux" ];
    in
    {
      packages = forAllSystems (
        system:
        let
          pkgs = import nixpkgs {
            inherit system;
            config.allowUnfreePredicate =
              package:
              builtins.elem (nixpkgs.lib.getName package) [
                "titbit"
                "titbit-tools"
              ];
          };
          tools = pkgs.callPackage ./nix/tools.nix { src = ./.; };
          clang = pkgs.wrapClangMulti pkgs.llvmPackages_18.clang;
          engine = pkgs.pkgsi686Linux.callPackage ./nix/engine.nix {
            src = ./.;
            stdenv = pkgs.overrideCC pkgs.pkgsi686Linux.stdenv clang;
            inherit (pkgs)
              llvmPackages_18
              python3
              pkg-config
              binutils
              makeDesktopItem
              ;
            titbitTools = tools;
            win32Headers = pkgs.pkgsCross.mingw32.windows.mingw_w64_headers;
          };
        in
        {
          default = engine;
          titbit = engine;
          titbit-tools = tools;
        }
      );

      apps = forAllSystems (
        system:
        let
          packages = self.packages.${system};
          app = package: command: {
            type = "app";
            program = "${package}/bin/${command}";
            meta.description = package.meta.description;
          };
        in
        {
          default = app packages.titbit "titbit";
          titbit = app packages.titbit "titbit";
          titbit-setup = app packages.titbit-tools "titbit-setup";
          titbit-studio = app packages.titbit-tools "titbit-studio";
        }
      );

      checks = forAllSystems (system: {
        inherit (self.packages.${system}) titbit titbit-tools;
      });

      formatter = forAllSystems (system: nixpkgs.legacyPackages.${system}.nixfmt);
    };
}
