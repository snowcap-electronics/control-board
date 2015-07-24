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
#define SC_LOG_MODULE_TAG SC_LOG_MODULE_UNSPECIFIED

#include "sc.h"

#include <stdio.h>        // sscanf
#include <string.h>       // memset

static void parse_command_help(const uint8_t *param, uint8_t param_len);
static void parse_command_echo(const uint8_t *param, uint8_t param_len);

static void parse_command_ping(const uint8_t *param, uint8_t param_len);
static void parse_command_blob(const uint8_t *param, uint8_t param_len);
static void parse_command_power(const uint8_t *param, uint8_t param_len);
static void parse_command(void);

/*
 * Buffer for incoming commands.
 * FIXME: Separate handling for SPI/UART1-3?
 */
#ifndef SC_CMD_MAX_RECV_BUF_LEN
#define SC_CMD_MAX_RECV_BUF_LEN      16
#endif
#define MIN_CMD_LEN           3        // 8 bit command, >=8 bit value, \n
static uint8_t receive_buffer[SC_CMD_MAX_RECV_BUF_LEN];
static uint8_t recv_i = 0;
static bool echo = false;

/*
 * Blob transfers
 */
#ifndef SC_BLOB_MAX_SIZE
#define SC_BLOB_MAX_SIZE      256
#endif
static uint8_t blob_buf[SC_BLOB_MAX_SIZE];
static uint16_t blob_len = 0;
static uint16_t blob_i = 0;
static mutex_t blob_mtx;

/*
 * Command storage
 */
#ifndef SC_CMD_MAX_COMMANDS
#define SC_CMD_MAX_COMMANDS   16
#endif
static struct sc_cmd commands[SC_CMD_MAX_COMMANDS];
static const struct sc_cmd cmds[] = {
  {"help",          SC_CMD_HELP("Show help"), parse_command_help},
  {"echo",          SC_CMD_HELP("Enable (0/1) echo"), parse_command_echo},
  {"blob",          SC_CMD_HELP("Transfer binary blob (n bytes)"), parse_command_blob},
  {"power",         SC_CMD_HELP("Set power (standby/stop)"), parse_command_power},
  {"ping",          SC_CMD_HELP("Ask for pong"), parse_command_ping},
};


/*
 * Initialize command module
 */
void sc_cmd_init(void)
{
  uint8_t i;

  chMtxObjectInit(&blob_mtx);
  memset(commands, 0, sizeof(commands));

  for (i = 0; i < SC_ARRAY_SIZE(cmds); ++i) {
    sc_cmd_register(cmds[i].cmd, cmds[i].help, cmds[i].cmd_cb);
  }
}




/*
 * Deinitialize command module
 */
void sc_cmd_deinit(void)
{
  uint8_t i;
  recv_i = 0;
  for (i = 0; i < SC_CMD_MAX_COMMANDS; ++i) {
    commands[i].cmd = NULL;
  }
}




/*
 * Receive byte. This can be called only from the main thread.
 * FIXME: per uart buffers. Now we happily mix all uarts together.
 */
void sc_cmd_push_byte(uint8_t byte)
{

  // If in a middle of blob transfer, store raw data without interpretation
  chMtxLock(&blob_mtx);
  if (blob_i < blob_len) {

    // FIXME: HACK: Assume \n is from the previous command and not
    // part of the blob
    if (blob_i == 0 && byte == '\n') {
      chMtxUnlock(&blob_mtx);
      return;
    }

    if (blob_i+1 >= SC_BLOB_MAX_SIZE) {
      chDbgAssert(0, "blob_i out of bounds");
      chMtxUnlock(&blob_mtx);
      return;
    }
    blob_buf[blob_i++] = byte;

    if (blob_i % 1024 == 0) {
      SC_DBG("1K received\r\n");
    }

    if (blob_i == blob_len) {
      msg_t drdy;
      // Create and send blob ready notification
      drdy = sc_event_msg_create_type(SC_EVENT_TYPE_BLOB_AVAILABLE);
      sc_event_msg_post(drdy, SC_EVENT_MSG_POST_FROM_NORMAL);
    }
  }
  chMtxUnlock(&blob_mtx);

  // Convert all different types of newlines to single \n
  if (byte == '\r') {
    byte = '\n';
  }

  // Do nothing on \n if there's no previous command in the buffer
  if ((byte == '\n') && (recv_i == 0 || receive_buffer[recv_i - 1] == '\n')) {
    return;
  }

  if (echo) {
    if (byte == '\n') {
      SC_LOG_PRINTF("\r\n");
    } else {
      SC_LOG_PRINTF("%c", byte);
    }
  }

  // Store the received character
  receive_buffer[recv_i] = byte;

  // Check for full command in the buffer
  if (byte == '\n') {

    // Null terminate
    receive_buffer[recv_i] = '\0';

    // Parse the received command
    parse_command();

    // Mark buffer empty
    recv_i = 0;
    return;
  }

  // Discard all data in buffer if buffer is full.
  if (++recv_i == SC_CMD_MAX_RECV_BUF_LEN) {
    // FIXME: increase some overflow stat?
    recv_i = 0;
  }
}



/*
 * Register a command and handler
 */
void sc_cmd_register(const char *cmd,
                     const char *help,
                     sc_cmd_cb cb)
{
  int i = 0;

  if (cmd == NULL) {
    chDbgAssert(0, "Command not specified");
    return;
  }

  while (i < SC_CMD_MAX_COMMANDS && commands[i].cmd != NULL) {
    if (sc_str_equal(commands[i].cmd, cmd, 0)) {
      chDbgAssert(0, "Command already registered");
      return;
    }
    i++;
  }

  if (i == SC_CMD_MAX_COMMANDS) {
    chDbgAssert(0, "Too many commands registered!");
    return;
  }

  chDbgAssert(i < SC_CMD_MAX_COMMANDS, "Logic bug");
  chDbgAssert(commands[i].cmd == NULL, "Logic bug");

  commands[i].cmd = cmd;
  commands[i].help = help;
  commands[i].cmd_cb = cb;
}

/*
 * Parse a received command.
 */
static void parse_command(void)
{
  int i;
  uint8_t cmd_len = 0;
  bool found_param = false;
  bool found_cmd = false;

  // Find the start of the first parameter to allow using sc_str_equal later
  for (i = 1; i < recv_i; ++i) {
    if (receive_buffer[i] == ' ') {
      cmd_len = i;
      found_param = true;
      break;
    }
  }

  if (!found_param) {
    cmd_len = recv_i;
  }

  for (i = 0; i < SC_CMD_MAX_COMMANDS; i++) {
    if (commands[i].cmd == NULL) {
      break;
    }
    if (sc_str_equal(commands[i].cmd, (char*)receive_buffer, cmd_len)) {
      uint8_t *cmd = NULL;
      uint8_t len = 0;

      found_cmd = true;

      if (found_param) {
        cmd = &receive_buffer[cmd_len + 1];
        len = recv_i - (cmd_len + 1);
      }
      commands[i].cmd_cb(cmd, len);
      break;
    }
  }

  if (!found_cmd) {
    SC_LOG_PRINTF("Unknown command: %s\r\n", receive_buffer);
  }
}



/*
 * Parse ping command
 */
static void parse_command_ping(const uint8_t *param, uint8_t param_len)
{
  (void)param;
  (void)param_len;

  msg_t pingmsg;

  pingmsg = sc_event_msg_create_type(SC_EVENT_TYPE_PING);
  sc_event_msg_post(pingmsg, SC_EVENT_MSG_POST_FROM_NORMAL);

  // Application is expected to reply with pong
}



/*
 * Parse blob command (bXXXXX)
 */
static void parse_command_blob(const uint8_t *param, uint8_t param_len)
{
  uint16_t bytes;

  (void)param_len;

  if (sscanf((char*)param, "%hu", &bytes) < 1) {
    return;
  }

  if (bytes == 0) {
    // FIXME: Should just print a warning (can't assert on user input)?
    chDbgAssert(0, "Failed to parse blob size");
    return;
  }

  if (bytes >= SC_BLOB_MAX_SIZE) {
    // FIXME: Should just print a warning (can't assert on user input)?
    chDbgAssert(0, "Requested blob size too large");
    return;
  }

  chMtxLock(&blob_mtx);
  blob_i = 0;
  blob_len = bytes;
  chMtxUnlock(&blob_mtx);
}



/*
 * Return pointer to newly received binary blob
 */
uint16_t sc_cmd_blob_get(uint8_t **blob)
{
  uint16_t len = 0;

  chMtxLock(&blob_mtx);
  if (blob_len > 0 && blob_i == blob_len) {
    len = blob_len;
  }
  chMtxUnlock(&blob_mtx);

  // There might still be a small change that the blob gets restarted between the check above and returning below.
  if (len) {
    *blob = blob_buf;
  } else {
    chDbgAssert(0, "No binary blob available");
    *blob = NULL;
  }
  return len;
}



/*
 * Parse power command
 */
static void parse_command_power(const uint8_t *param, uint8_t param_len)
{
  if (sc_str_equal((char*)param, "standby", param_len)) {
    //sc_pwr_mode_standby(true, true);
  } else if (sc_str_equal((char*)param, "stop", param_len)) {
    //sc_pwr_mode_stop(true);
  }
}

#ifndef SC_CMD_REMOVE_HELP
static THD_WORKING_AREA(sc_cmd_help_thread, 1024);
THD_FUNCTION(scCmdHelpThread, arg)
{
  uint8_t i;

  (void)arg;

  chRegSetThreadName(__func__);

  for (i = 0; i < SC_CMD_MAX_COMMANDS; ++i) {
    const char *help = "No help available";

    if (commands[i].cmd == NULL) {
      break;
    }

    if (commands[i].help) {
      help = commands[i].help;
    }

    SC_LOG_PRINTF("%-16s %s\r\n", commands[i].cmd, help);

    // Prevent overflowing the transmit buffers
    chThdSleepMilliseconds(50);
  }
}
#endif

/*
 * Parse help command
 */
static void parse_command_help(const uint8_t *param, uint8_t param_len)
{

  (void)param_len;
  (void)param;

#ifdef SC_CMD_REMOVE_HELP
  SC_LOG_PRINTF("No help available\r\n");
#else
  // Launch a thread to print longer stuff
  chThdCreateStatic(sc_cmd_help_thread,
                    sizeof(sc_cmd_help_thread),
                    NORMALPRIO,
                    scCmdHelpThread,
                    NULL);
#endif
}



/*
 * Parse echo command
 */
static void parse_command_echo(const uint8_t *param, uint8_t param_len)
{
  (void)param_len;

  switch (param[0]) {
  case '0':
    echo = 0;
    break;
  case '1':
    echo = 1;
    break;
  default:
    // Invalid value, ignoring command
    return;
  }
  SC_LOG_PRINTF("echo %s\r\n", echo ? "on" : "off");
}



/* Emacs indentatation information
   Local Variables:
   indent-tabs-mode:nil
   tab-width:2
   c-basic-offset:2
   End:
*/
