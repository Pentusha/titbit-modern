{
  callPackage,
  runCommand,
  python3,
  writeShellScriptBin,
  tools,
}:
let
  fixture = runCommand "titbit-synthetic-disc" { } ''
    ${python3}/bin/python3 ${./tests/fixture.py} "$out"
  '';
  fixtureTools = tools.overrideAttrs (old: {
    postPatch = (old.postPatch or "") + ''
      cp ${fixture}/assets.json config/assets.json
    '';
  });
  data = callPackage ./_assets.nix {
    source = "${fixture}/torrent";
    tools = fixtureTools;
  };
  game =
    (writeShellScriptBin "titbit" ''
      if [ "''${1-}" = "--help" ]; then
        echo 'fixture game help'
        exit 0
      fi
      exec ${python3}/bin/python3 -c \
        'import json, os, sys; open(os.environ["TITBIT_TEST_ARGS"], "w").write(json.dumps(sys.argv[1:]))' "$@"
    '').overrideAttrs
      (_: {
        version = "test";
        meta = {
          description = "Argument recorder for the game-data launcher check";
          homepage = "https://github.com/offx366/titbit-modern";
        };
      });
  wrapped = callPackage ./_package.nix {
    inherit game data;
    tools = fixtureTools;
  };
in
runCommand "titbit-game-data-check"
  {
    TITBIT_WRAPPER = "${wrapped}/bin/titbit";
    TITBIT_DATA = data;
    TITBIT_FIXTURE = fixture;
    TITBIT_PREPARE = ./prepare.py;
    TITBIT_IMPORTER = "${fixtureTools}/bin/titbit-setup";
  }
  ''
    ${python3}/bin/python3 ${./tests/runtime.py}
    touch "$out"
  ''
