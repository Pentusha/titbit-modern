{ inputs, lib, ... }:
{
  perSystem =
    { pkgs, config, ... }:
    let
      guest = inputs.nixpkgs.lib.nixosSystem {
        system = "x86_64-linux";
        modules = [
          inputs.self.modules.nixos.titbit-vm
          { virtualisation.host.pkgs = pkgs; }
        ];
      };
      package = pkgs.callPackage ./_package.nix {
        vm = guest.config.system.build.vm;
        tools = config.packages.titbit-tools;
        version = lib.trim (builtins.readFile ../../VERSION);
      };
      app = {
        type = "app";
        program = "${package}/bin/titbit";
        meta.description = package.meta.description;
      };
    in
    lib.mkMerge [
      {
        packages.titbit-vm = package;
        apps.titbit-vm = app;
      }
      (lib.mkIf pkgs.stdenv.hostPlatform.isDarwin {
        packages = {
          default = package;
          titbit = package;
        };
        apps = {
          default = app;
          titbit = app;
        };
      })
    ];
}
