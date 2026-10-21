{
  perSystem =
    { pkgs, ... }:
    let
      tools = pkgs.callPackage ./_package.nix { src = ../..; };
      app = command: {
        type = "app";
        program = "${tools}/bin/${command}";
        meta.description = tools.meta.description;
      };
    in
    {
      packages.titbit-tools = tools;
      apps = {
        titbit-setup = app "titbit-setup";
        titbit-studio = app "titbit-studio";
      };
      checks.titbit-tools = tools;
    };
}
