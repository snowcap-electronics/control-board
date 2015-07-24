/*
 * Command parsing
 *
 * Copyright 2011 Tuomas Kulve, <tuomas.kulve@snowcap.fi>
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following
 * conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#ifndef SC_CMD_H
#define SC_CMD_H

#include <sc_utils.h>

#ifdef SC_CMD_REMOVE_HELP
#define SC_CMD_HELP(a) (NULL)
#else
#define SC_CMD_HELP(a) (a)
#endif

typedef void (*sc_cmd_cb)(const uint8_t*, uint8_t);

struct sc_cmd {
  const char *cmd;
  const char *help;
  sc_cmd_cb cmd_cb;
};

void sc_cmd_init(void);
void sc_cmd_deinit(void);
void sc_cmd_push_byte(uint8_t byte);
uint16_t sc_cmd_blob_get(uint8_t **blob);
void sc_cmd_register(const char *cmd, const char *help, sc_cmd_cb cb);

#endif



/* Emacs indentatation information
   Local Variables:
   indent-tabs-mode:nil
   tab-width:2
   c-basic-offset:2
   End:
*/
