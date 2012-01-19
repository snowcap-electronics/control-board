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

  // FIXME: Add always \r\n to the buffer??
  // FIXME: in error: e\r\n ??

  // Go through all numbers and store the last digit to the string
  // starting for the last character
  while (--index >= 0) {
	str[index] = (value % 10) + '0';
	if (value < 10) {
	  break;
	}
	value /= 10;
  }

  // Check for overflow (needs also space for \0).
  if (index < 1) {
	str[0] = '\0';
	return 0;
  }

  // Move numbers to the start of the string. Add \0 after the last
  // number or as the last character in case of overflow
  extra = index;
  for (i = 0; i < extra; i++) {
	str[i] = str[i + extra];
  }

  str_len = len - extra;
  str[str_len] = '\0';

  return str_len;
}
