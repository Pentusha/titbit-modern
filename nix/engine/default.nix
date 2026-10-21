{ lib, ... }:
{
  perSystem =
    {
      pkgs,
      system,
      config,
      ...
    }:
    lib.mkIf (system == "x86_64-linux") (
      let
        clang = pkgs.wrapClangMulti pkgs.llvmPackages_18.clang;
        engine = pkgs.pkgsi686Linux.callPackage ./_package.nix {
          src = ../..;
          stdenv = pkgs.overrideCC pkgs.pkgsi686Linux.stdenv clang;
          inherit (pkgs)
            llvmPackages_18
            python3
            pkg-config
            binutils
            makeDesktopItem
            ;
          titbitTools = config.packages.titbit-tools;
          win32Headers = pkgs.pkgsCross.mingw32.windows.mingw_w64_headers;
        };
        app = {
          type = "app";
          program = "${engine}/bin/titbit";
          meta.description = engine.meta.description;
        };
      in
      {
        packages = {
          default = engine;
          titbit = engine;
        };
        apps = {
          default = app;
          titbit = app;
        };
        checks.titbit = engine;
      }
    );
}
