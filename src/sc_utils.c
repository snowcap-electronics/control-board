/***
 * Utils functions
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

#include "sc_utils.h"


/*
 * Convert string to int. No way of returning an error.
 */
int sc_atoi(uint8_t *str, int len)
{
  int valid_len = 0;
  int i;
  int value;
  int base;
  int sign = 1;

  // Do nothing with invalid length
  if (len < 0 || (len == 1 && str[0] == '-')) {
    return 0;
  }

  // Check for negative numbers
  if (str[0] == '-') {
    sign = -1;
    str++;
    len--;
  }

  // Count the valid length (stop on non-digit)
  i = 0;
  do {
    if (str[i] < '0' || str[i] > '9') {
      break;
    }
    if (++i == len) {
      break;
    }
  } while(1);

  valid_len = i;

  // Convert ascii to int starting from the end
  base = 1;
  value = 0;
  for (i = valid_len; i > 0; i--) {
    value += base * (str[i - 1] - '0');
    base *= 10;
  }

  return sign * value;
}



/*
 * Convert int to string. Returns length on the converted string
 * (excluding the trailing \0) or 0 in error.
 */
int sc_itoa(int value, uint8_t *str, int len)
{
  int index = len;
  int extra, i;
  int str_len;
  int negative = 0;
  int start = 0;

  // Check for negative values
  if (value < 0) {
    negative = 1;
    value *= -1;
  }

  // Go through all numbers and store the last digit to the string
  // starting for the last character
  while (--index >= 0) {
    str[index] = (value % 10) + '0';
    if (value < 10) {
      break;
    }
    value /= 10;
  }

  // Check for overflow (needs also space for '\0' and possible '-').
  if (index < 1 || (negative && index < 2)) {
    str[0] = '\0';
    return 0;
  }

  // Add - for negative numbers
  if (negative) {
    str[0] = '-';
    start = 1;
  }

  // Move numbers to the start of the string. Add \0 after the last
  // number or as the last character in case of overflow
  extra = index;
  for (i = 0; i < len - extra; i++) {
    str[i + start] = str[i + extra];
  }
  str_len = len - extra + start;

  str[str_len] = '\0';

  return str_len;
}



/*
 * Convert double to string. Print 'decimals' amount of
 * decimals. Returns length on the converted string (excluding the
 * trailing \0) or 0 in error.
 */
int sc_ftoa(double value, uint8_t decimals, uint8_t *str, int len)
{
  int total_len = 0;
  int value_int;
  double value_frac;

  // len(-x.<decimals>\0) == decimals + 4
  if (decimals + 4 > len) {
    return 0;
  }

  if (value < 0) {
    str[total_len++] = '-';
    value *= -1;
  }

  value_int = (int)value;
  value_frac = value - value_int;

  // Print integer part
  total_len += sc_itoa(value_int, &str[total_len], len - total_len);

  // Check if there is space for decimal part
  if (total_len + decimals + 2 >= len) {
    return 0;
  }

  str[total_len++] = '.';

  // Make sure the fractional part is positive
  if (value_frac < 0) {
    value_frac *= -1;
  }

  // Count the number of decimals
  while(decimals--) {
    value_frac *= 10;

    if (value_frac < 1) {
      str[total_len++] = '0';
    }
  }

  // Print the decimal part, if there is any
  if (value_frac >= 1) {
    total_len += sc_itoa((int)value_frac, &str[total_len], len - total_len);
  }

  return total_len;
}



bool sc_str_equal(const char *a, const char *b, uint8_t n)
{
  uint8_t i = 0;

  while (*a == *b && (*a != '\0' && *b != '\0')) {
    if (n > 0 && ++i == n) {
      break;
    }
    ++a;
    ++b;
  }

  if (*a == *b) {
    return true;
  } else {
    return false;
  }
}



/* Emacs indentatation information
   Local Variables:
   indent-tabs-mode:nil
   tab-width:2
   c-basic-offset:2
   End:
*/
