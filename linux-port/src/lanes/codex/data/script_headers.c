/* The original SeeR header TEXT is preserved under src/script_headers.
 * tools/build_original.py generates escaped C literals from those sources.
 * These are text assets, not generated pseudocode or machine-code payloads. */
static const char queststate_text[] =
#include "queststate_text.inc"
;
static const char input_text[] =
#include "input_text.inc"
;
static const char scener_text[] =
#include "scener_text.inc"
;

const char *queststate_header = queststate_text; /* native 0x4de098 */
const char *input_header = input_text;           /* native 0x4de09c */
const char *scener_header = scener_text;         /* native 0x4de0a0 */
