/* validate_uri.c - Cleans uris from unsafe structures
 * TEAM MEMBERS:
 *      Debbie Lim
 *      Jai Manacsa
 *      Utsav Bhandari
 */

#include <stdlib.h> 
#include <string.h> 
#include <stdbool.h> 
#include <ctype.h>

#define MAXLINE 8192

// Check for valid characters
static bool valid_uri_char(char c)
{
  return isalnum(c) || c == '.' 
    || c == '-' 
    || c == '~' 
    || c == ':' 
    || c == '/'
    || c == '?'
    || c == '#'
    || c == '['
    || c == ']'
    || c == '@'
    || c == '!'
    || c == '$'
    || c == '&'
    || c == '\''
    || c == '('
    || c == ')'
    || c == '*'
    || c == '+'
    || c == ','
    || c == ';'
    || c == '='
    || c == '%';
}

static bool validate_uri(const char *uri)
{
  if (!uri || *uri == '\0') {
    return false;
  }
  while (*uri) {
    if(!valid_uri_char(*uri)) {
      return false;
    }
    uri++;
  }
  return true;
}


// Check for ".." sequences
static bool safe_path(const char *path)
{
  return strstr(path, "..") == NULL;
}


// XSS protection (HTML/javascript code) 
static void sanitize_html(const char *input, char *output)
{
  const char *source = input;
  char *dest = output;
  while (*source) {
    switch (*source) {
    case '<': 
      strcpy(dest, "&lt;"); 
      dest += 4; 
      break;
    case '>':
      strcpy(dest, "&gt;");
      dest += 4;
      break;
    case '&':
      strcpy(dest, "&amp;");
      dest += 5;
      break; 
    case '"':
      strcpy(dest, "&quot;");
      dest += 6;
      break;
    case '\'':
      strcpy(dest, "&#39;");
      dest += 5;
      break;
    default:
      *dest = *source;
      dest++;
      break;
    }
    source++;
  } 
  *dest = '\0';
}

static void sanitize_sql(const char *input, char *output)
{
  const char *source = input;
  char *dest = output;
  while (*source) {
    if (*source == '\'') {
      *dest++ = '\\';
    }
    *dest++ = *source++;
  }
  *dest = '\0';
}

static void sanitize_command(const char *input, char *output)
{
  const char *source = input;
  char *dest = output;
  while (*source) {
    if (*source == ';' || *source == '&' || *source == '|') {
      *dest++ = '\\';
    }
    *dest++ = *source++;
  }
  *dest = '\0';
}


char *sanitize_uri(const char *uri)
{
  const char *input_uri = uri;
  
  if (validate_uri(input_uri) == false || safe_path(input_uri) == false) {
    return NULL; // Return NULL to indicate failure
  } 
  
  else {
    char *no_html_uri = malloc(MAXLINE);
    if (no_html_uri == NULL) return NULL;
    sanitize_html(input_uri, no_html_uri);
    
    char *no_sql_uri = malloc(MAXLINE);
    if (no_sql_uri == NULL) {
      free(no_html_uri);
      return NULL;
    }
    sanitize_sql(no_html_uri, no_sql_uri);
    
    char *no_command_uri = malloc(MAXLINE);
    if (no_command_uri == NULL) {
      free(no_html_uri);
      free(no_sql_uri);
      return NULL;
    }
    sanitize_command(no_sql_uri, no_command_uri);
    free(no_html_uri);
    free(no_sql_uri);
    
    return no_command_uri;
  }
}
