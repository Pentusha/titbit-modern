{
  lib,
  stdenvNoCC,
  python3,
  source,
  tools,
}:
stdenvNoCC.mkDerivation {
  pname = "titbit-data";
  inherit (tools) version;
  inherit source;
  dontUnpack = true;
  dontConfigure = true;
  dontBuild = true;
  installPhase = ''
    runHook preInstall
    PYTHONPATH=${tools}/share/titbit/tools ${python3}/bin/python3 ${./prepare.py} \
      --importer ${tools}/bin/titbit-setup --source "$source" --dest "$out"
    runHook postInstall
  '';
  doInstallCheck = true;
  installCheckPhase = ''
    runHook preInstallCheck
    ${tools}/bin/titbit-setup --verify "$out"
    runHook postInstallCheck
  '';
  meta = {
    description = "Verified original Ядерный Титбит game data";
    homepage = "https://github.com/offx366/titbit-modern";
    license = lib.licenses.unfree;
    platforms = lib.platforms.unix;
  };
}
