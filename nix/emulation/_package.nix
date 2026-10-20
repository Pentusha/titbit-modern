{
  lib,
  stdenvNoCC,
  qemu-user,
  mesaDrivers,
  engine,
  tools,
}:
let
  launchEngine = stdenvNoCC.mkDerivation {
    pname = "titbit-qemu-launcher";
    inherit (engine) version;
    dontUnpack = true;
    installPhase = ''
      mkdir -p "$out/bin"
      cat > "$out/bin/titbit-engine" <<'SCRIPT'
      #!${stdenvNoCC.shell}
      # Host GPU drivers cannot be loaded into the emulated i386 process.
      exec ${qemu-user}/bin/qemu-i386 \
        -E LIBGL_ALWAYS_SOFTWARE=1 \
        -E LIBGL_DRIVERS_PATH=${mesaDrivers}/lib/dri \
        -E LD_LIBRARY_PATH=${mesaDrivers}/lib \
        -E __GLX_VENDOR_LIBRARY_NAME=mesa \
        -E __EGL_VENDOR_LIBRARY_FILENAMES=${mesaDrivers}/share/glvnd/egl_vendor.d/50_mesa.json \
        ${engine}/libexec/titbit "$@"
      SCRIPT
      chmod +x "$out/bin/titbit-engine"
    '';
  };
in
stdenvNoCC.mkDerivation {
  pname = "titbit";
  inherit (engine) version;
  dontUnpack = true;
  installPhase = ''
    mkdir -p "$out/bin" "$out/share/applications"
    substitute ${../engine/titbit.sh} "$out/bin/titbit" \
      --subst-var-by shell ${stdenvNoCC.shell} \
      --subst-var-by engine ${launchEngine}/bin/titbit-engine \
      --subst-var-by setup ${tools}/bin/titbit-setup
    chmod +x "$out/bin/titbit"
    ln -s ${tools}/bin/titbit-setup "$out/bin/titbit-setup"
    ln -s ${tools}/bin/titbit-studio "$out/bin/titbit-studio"
    cp ${engine}/share/applications/* "$out/share/applications/"
  '';
  doInstallCheck = true;
  installCheckPhase = ''
    runHook preInstallCheck
    "$out/bin/titbit" --help
    mkdir -p build
    ${qemu-user}/bin/qemu-i386 ${engine}/libexec/titbit-runtime-smoke
    "$out/bin/titbit-setup" --help
    runHook postInstallCheck
  '';
  meta = {
    description = "Ядерный Титбит via QEMU i386 user emulation and software OpenGL";
    homepage = engine.meta.homepage;
    license = lib.licenses.unfree;
    platforms = [
      "x86_64-linux"
      "aarch64-linux"
    ];
    mainProgram = "titbit";
  };
}
