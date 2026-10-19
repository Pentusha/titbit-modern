/* Actual one-byte native cleanup guards: each is accessed with BYTE tests
 * and OR stores in the source-built initializer/atexit functions. They are
 * separate variables, not an enlarged allocation for unknown adjacent data. */
unsigned char renderer_cleanup_guard = 0;        /* 5026cc:4215b0 */
unsigned char sound_bank_cleanup_guard = 0;      /* 5ee700:427da0 */
unsigned char sax_names_cleanup_guard = 0;       /* 5ee73c:42efa0/42f100/42f260 */
unsigned char xml_dom_cleanup_guard = 0;         /* 5ee7c4:432f80 */
unsigned char xml_writer_cleanup_guard = 0;      /* 5ee86c:439ed0 */
unsigned char xml_document_cleanup_guard = 0;    /* 5ee92c:43d510 */
unsigned char xml_parser_cleanup_guard = 0;      /* 5ee9c8:43f700/43f860/43fde0 */
unsigned char xml_parser_cleanup_guard2 = 0;     /* 5ee9c9:4400a0 */
unsigned char stl_second_cleanup_guard = 0;      /* 5f58b8:4b9c47/4c3350 */

/* Actual16-element matrix initialized by source4215e0 before use. */
float scene_identity_matrix[16] = {0};             /* 502678 */
