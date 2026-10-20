{
  lib,
  stdenv,
  llvmPackages_18,
  gcc,
  python3,
  pkg-config,
  binutils,
  SDL2,
  libGL,
  libGLU,
  ffmpeg_6,
  zlib,
  makeDesktopItem,
  src,
  titbitTools,
  win32Headers,
}:
let
  desktopItem = makeDesktopItem {
    name = "titbit";
    desktopName = "Ядерный Титбит";
    comment = "Play Ядерный Титбит";
    exec = "titbit";
    categories = [
      "Game"
      "AdventureGame"
    ];
  };
in
stdenv.mkDerivation {
  pname = "titbit";
  version = lib.trim (builtins.readFile (src + "/VERSION"));

  src = lib.fileset.toSource {
    root = src;
    fileset = lib.fileset.unions [
      (lib.fileset.difference (lib.fileset.fileFilter (file: !(file.hasExt "pyc" || file.hasExt "pyo")) (
        src + "/linux-port"
      )) (src + "/linux-port/docs"))
      (src + "/portable")
    ];
  };

  postUnpack = ''
    sourceRoot+=/linux-port
  '';

  nativeBuildInputs = [
    python3
    pkg-config
    binutils
    llvmPackages_18.llvm
  ];

  buildInputs = [
    SDL2
    libGL
    libGLU
    ffmpeg_6
    zlib
    (lib.getLib gcc.cc)
    gcc.cc.libgcc
  ];

  # Native objects need Nix's multilib compiler wrapper. The MSVC frontend and
  # retargeted IR must use raw Clang, without Linux libc or linker flags.
  postPatch = ''
    patch -d .. -p1 < ${./portable-headers.patch}
    substituteInPlace tools/build_linux.py tools/compile_ms_ir.py \
      --replace-fail /usr/i686-w64-mingw32/include ${win32Headers}/include
    substituteInPlace tools/compile_ms_ir.py tools/ms_ir_to_elf.py \
      --replace-fail "default='clang++-18'" \
        "default='${llvmPackages_18.clang-unwrapped}/bin/clang++'"
    # Clang 18's default resource path does not account for Nix's lib output.
    substituteInPlace tools/compile_ms_ir.py \
      --replace-fail "resource = subprocess.check_output([compiler, '-print-resource-dir'], text=True).strip()" \
        "resource = '${lib.getLib llvmPackages_18.clang-unwrapped}/lib/clang/18'"
    substituteInPlace tools/link_native.py tools/check_cpp_runtime.py \
      --replace-fail "'-L/usr/lib/i386-linux-gnu'," ""
  '';

  dontConfigure = true;
  enableParallelBuilding = true;

  preBuild = ''
    mkdir -p "$NIX_BUILD_TOP/titbit-toolchain"
    ln -s ${stdenv.cc}/bin/clang "$NIX_BUILD_TOP/titbit-toolchain/clang-18"
    ln -s ${stdenv.cc}/bin/clang++ "$NIX_BUILD_TOP/titbit-toolchain/clang++-18"
    ln -s ${llvmPackages_18.llvm}/bin/opt "$NIX_BUILD_TOP/titbit-toolchain/opt-18"
    ln -s ${llvmPackages_18.llvm}/bin/llvm-ar "$NIX_BUILD_TOP/titbit-toolchain/llvm-ar-18"
    export PATH="$NIX_BUILD_TOP/titbit-toolchain:$PATH"
  '';

  buildPhase = ''
    runHook preBuild
    jobs=$NIX_BUILD_CORES
    if (( jobs > 64 )); then jobs=64; fi
    # Use Nix dependencies instead of --minimal, which downloads and builds
    # dependencies in the user's cache. Print stage logs on failure.
    if ! python3 tools/build_linux.py --jobs "$jobs"; then
      find build/logs -name '*.log' -type f -exec tail -n 80 {} \;
      exit 1
    fi
    runHook postBuild
  '';

  doCheck = true;
  checkPhase = ''
    runHook preCheck
    python3 tools/check_cpp_runtime.py
    ./build/titbit --help
    runHook postCheck
  '';

  installPhase = ''
    runHook preInstall
    install -Dm755 build/titbit "$out/libexec/titbit"
    install -Dm755 build/cpp-runtime-test/smoke "$out/libexec/titbit-runtime-smoke"
    mkdir -p "$out/bin"
    substitute ${./titbit.sh} "$out/bin/titbit" \
      --subst-var-by shell ${stdenv.shell} \
      --subst-var-by engine "$out/libexec/titbit" \
      --subst-var-by setup ${titbitTools}/bin/titbit-setup
    chmod +x "$out/bin/titbit"
    ln -s ${titbitTools}/bin/titbit-setup "$out/bin/titbit-setup"
    ln -s ${titbitTools}/bin/titbit-studio "$out/bin/titbit-studio"
    mkdir -p "$out/share/applications"
    cp ${desktopItem}/share/applications/* "$out/share/applications/"
    runHook postInstall
  '';

  doInstallCheck = true;
  installCheckPhase = ''
    runHook preInstallCheck
    "$out/bin/titbit" --help
    "$out/bin/titbit-setup" --help
    runHook postInstallCheck
  '';

  meta = {
    description = "Reconstructed Ядерный Титбит engine with a native Linux platform layer";
    homepage = "https://github.com/offx366/titbit-modern";
    license = lib.licenses.unfree;
    platforms = [ "i686-linux" ];
    mainProgram = "titbit";
  };
}
