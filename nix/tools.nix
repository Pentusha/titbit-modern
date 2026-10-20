{
  lib,
  stdenvNoCC,
  python3,
  makeWrapper,
  src,
}:
let
  python = python3.withPackages (ps: [
    ps.pillow
    ps.tkinter
  ]);
in
stdenvNoCC.mkDerivation {
  pname = "titbit-tools";
  version = lib.trim (builtins.readFile (src + "/VERSION"));

  src = lib.fileset.toSource {
    root = src;
    fileset = lib.fileset.unions [
      (src + "/linux-port/tools")
      (src + "/linux-port/editor")
      (src + "/linux-port/config/assets.json")
    ];
  };

  postUnpack = ''
    sourceRoot+=/linux-port
  '';

  nativeBuildInputs = [ makeWrapper ];
  dontBuild = true;
  doCheck = true;

  checkPhase = ''
    runHook preCheck
    PYTHONPATH="$PWD/tools" ${python}/bin/python3 -m unittest discover -s tools/tests -p 'test_*.py'
    runHook postCheck
  '';

  installPhase = ''
    runHook preInstall
    mkdir -p "$out/share/titbit/tools" "$out/share/titbit/config"
    cp tools/{titbit_setup,titbit_assets,disc_image,start_studio}.py "$out/share/titbit/tools/"
    cp config/assets.json "$out/share/titbit/config/"
    cp -r editor "$out/share/titbit/"
    makeWrapper ${python}/bin/python3 "$out/bin/titbit-setup" \
      --add-flags "$out/share/titbit/tools/titbit_setup.py"
    makeWrapper ${python}/bin/python3 "$out/bin/titbit-studio" \
      --add-flags "$out/share/titbit/tools/start_studio.py"
    runHook postInstall
  '';

  doInstallCheck = true;
  installCheckPhase = ''
    runHook preInstallCheck
    "$out/bin/titbit-setup" --help
    "$out/bin/titbit-studio" --help
    runHook postInstallCheck
  '';

  meta = {
    description = "Ядерный Титбит data importer and Titbit Studio";
    homepage = "https://github.com/offx366/titbit-modern";
    license = lib.licenses.unfree;
    platforms = lib.platforms.unix;
    mainProgram = "titbit-setup";
  };
}
