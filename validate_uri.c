#include <stdbool.h> 
#include <ctype.h>

// Check for valid characters
bool valid_uri_char(char c) {
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

bool validate_uri(const char *uri) {
  if (!uri || *uri == "\0") {
      return false;
  }
  while (*uri) {
      if(!valid_uri_char(*url)) {
          return false;
      }
      url++;
  }
  return true;
}


// Check for ".." sequences
bool safe_path(const char *path) {
  return strstr(path, "..") == NULL;
}


// XSS protection (HTML/javascript code) 
void sanitize_html(const char *input, char *output) {
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
      }
      source++;
  } 
  *dest = "\0";
}

void sanitize_sql(const char *input, char *output) {
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

void sanitize_command(const char *input, char *output) {
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


char *sanitize_uri(const char *uri) {
  char *input_uri = uri;
  
  if (validate_uri(input_uri) == false || safepath(input_uri) == false) {
    return NULL; // Return NULL to indicate failure
  } 
  
  else {
    char *no_html_uri = malloc(strlen(input_uri) + 1); 
    if (no_html_uri == NULL) return NULL;
    sanitize_html(input_uri, no_html_uri);
    
    char *no_sql_uri = malloc(strlen(no_html_uri) + 1); 
    if (no_sql_uri == NULL) {
      free(no_html_uri);
      return NULL;
    }
    sanitize_sql(no_html_uri, no_sql_uri);
    
    char *no_command_uri = malloc(strlen(no_sql_uri) + 1);
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
