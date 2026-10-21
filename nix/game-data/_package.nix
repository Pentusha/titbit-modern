{
  lib,
  stdenvNoCC,
  game,
  data,
  tools,
}:
stdenvNoCC.mkDerivation {
  pname = "titbit";
  inherit (game) version;
  dontUnpack = true;
  installPhase = ''
    runHook preInstall
    mkdir -p "$out/bin"
    substitute ${./titbit.sh} "$out/bin/titbit" \
      --subst-var-by shell ${stdenvNoCC.shell} \
      --subst-var-by game ${game}/bin/titbit \
      --subst-var-by data ${data} \
      --subst-var-by setup ${tools}/bin/titbit-setup
    chmod +x "$out/bin/titbit"
    ln -s ${tools}/bin/titbit-setup "$out/bin/titbit-setup"
    ln -s ${tools}/bin/titbit-studio "$out/bin/titbit-studio"
    if [ -d ${game}/share/applications ]; then
      mkdir -p "$out/share"
      cp -r ${game}/share/applications "$out/share/"
    fi
    runHook postInstall
  '';
  doInstallCheck = true;
  installCheckPhase = ''
    runHook preInstallCheck
    "$out/bin/titbit" --help
    runHook postInstallCheck
  '';
  passthru.gameData = data;
  meta = {
    description = "${game.meta.description}, with declaratively supplied game data";
    inherit (game.meta) homepage;
    license = lib.licenses.unfree;
    platforms = [ stdenvNoCC.hostPlatform.system ];
    mainProgram = "titbit";
  };
}
