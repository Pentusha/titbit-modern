{ lib, withSystem, ... }:
let
  withGameData =
    { system, source }:
    withSystem system (
      { pkgs, config, ... }:
      let
        tools = config.packages.titbit-tools;
        data = pkgs.callPackage ./_assets.nix { inherit source tools; };
      in
      pkgs.callPackage ./_package.nix {
        inherit data tools;
        game = config.packages.titbit;
      }
    );
in
{
  options.flake.lib = lib.mkOption {
    type = lib.types.attrsOf lib.types.unspecified;
    default = { };
  };

  config.flake.lib = {
    inherit withGameData;
    withMagnet =
      {
        system,
        magnet,
        hash,
      }:
      withSystem system (
        { pkgs, ... }:
        withGameData {
          inherit system;
          source = pkgs.fetchtorrent {
            url = magnet;
            inherit hash;
          };
        }
      );
  };

  config.perSystem =
    { pkgs, config, ... }:
    {
      checks.titbit-game-data = pkgs.callPackage ./_check.nix {
        tools = config.packages.titbit-tools;
      };
    };
}
