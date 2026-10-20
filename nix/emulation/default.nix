{ inputs, lib, ... }:
{
  perSystem =
    {
      pkgs,
      system,
      config,
      ...
    }:
    lib.mkIf pkgs.stdenv.hostPlatform.isLinux (
      let
        package = pkgs.callPackage ./_package.nix {
          engine = inputs.self.packages.x86_64-linux.titbit;
          tools = config.packages.titbit-tools;
          mesaDrivers = inputs.nixpkgs.legacyPackages.x86_64-linux.pkgsi686Linux.mesa;
        };
        app = {
          type = "app";
          program = "${package}/bin/titbit";
          meta.description = package.meta.description;
        };
      in
      lib.mkMerge [
        {
          packages.titbit-emulated = package;
          apps.titbit-emulated = app;
          checks.titbit-emulated = package;
        }
        (lib.mkIf (system == "aarch64-linux") {
          packages = {
            default = package;
            titbit = package;
          };
          apps = {
            default = app;
            titbit = app;
          };
        })
      ]
    );
}
