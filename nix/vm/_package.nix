{
  lib,
  stdenvNoCC,
  coreutils,
  vm,
  tools,
  version,
}:
stdenvNoCC.mkDerivation {
  pname = "titbit";
  inherit version;
  dontUnpack = true;
  installPhase = ''
    mkdir -p "$out/bin"
    substitute ${./titbit.sh} "$out/bin/titbit" \
      --subst-var-by shell ${stdenvNoCC.shell} \
      --subst-var-by coreutils ${coreutils}/bin \
      --subst-var-by vm ${vm}/bin/run-titbit-vm \
      --subst-var-by setup ${tools}/bin/titbit-setup
    chmod +x "$out/bin/titbit"
    ln -s ${tools}/bin/titbit-setup "$out/bin/titbit-setup"
    ln -s ${tools}/bin/titbit-studio "$out/bin/titbit-studio"
  '';
  doInstallCheck = true;
  installCheckPhase = ''
    runHook preInstallCheck
    "$out/bin/titbit" --help
    runHook postInstallCheck
  '';
  meta = {
    description = "Ядерный Титбит in an x86 Linux VM with software OpenGL";
    homepage = "https://github.com/offx366/titbit-modern";
    license = lib.licenses.unfree;
    platforms = [
      "x86_64-linux"
      "aarch64-linux"
      "aarch64-darwin"
    ];
    mainProgram = "titbit";
  };
}
