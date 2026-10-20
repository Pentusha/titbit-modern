"""Build with modern LLVM/MSVC tools. VC6 is an optional reference mode only."""
import sys
if '--legacy-vc6' in sys.argv:
    sys.argv.remove('--legacy-vc6')
    from build_public import main
else:
    from build_windows import main
if __name__ == "__main__":
    sys.exit(main(default_compat=True))
