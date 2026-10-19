"""Build the public source tree with installed VC98 libraries."""
import sys
from build_public import main
if __name__ == "__main__":
    sys.exit(main(default_compat=False))
