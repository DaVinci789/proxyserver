#include <stdlib.h> 
#include <stdio.h>
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
    int inside_script = 0; // Flag to track if currently inside a <script> tag

    while (*source) {
        // Check for start of script tag (case-insensitive)
        if (strncasecmp(source, "<script>", strlen("<script>")) == 0) {
            inside_script = 1;
            source += strlen("<script>");
            continue;
        }
        // Check for end of script tag (case-insensitive)
        if (strncasecmp(source, "</script>", strlen("</script>")) == 0) {
            inside_script = 0;
            source += strlen("</script>");
            continue;
        }
        // If not inside a script tag, sanitize the HTML
        if (!inside_script) {
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
        }
        source++;
    } 
    *dest = '\0';
}

static void sanitize_sql(const char *input, char *output)
{
  const char *source = input;
  char *dest = output;

  const char *sql_keywords[] = {"SELECT", "INSERT", "UPDATE", "DELETE", "DROP", "CREATE", "ALTER", "EXEC", "TRUNCATE"};

while (*source) {
        // Check for single quotes and escape them
        if (*source == '\'') {
            strcpy(dest, "''");
            dest += 2;
        }
        // Check for SQL keywords and neutralize them
        else if (isalpha(*source)) {
            char word[256]; // Assuming a maximum length for a word
            int i = 0;
            while (isalpha(*source)) {
                word[i++] = *source;
                source++;
            }
            word[i] = '\0'; // Null-terminate the word

            // Check if the word is a SQL keyword
            for (int j = 0; j < sizeof(sql_keywords) / sizeof(sql_keywords[0]); j++) {
                if (strcasecmp(word, sql_keywords[j]) == 0) {
                    // Neutralize the SQL keyword by replacing it with an empty string
                    word[0] = '\0';
                    break;
                }
            }
            continue; // Skip the increment below to avoid missing the next character
        }
        // Copy other characters as is
        *dest = *source;
        dest++;
        source++;
    }
    *dest = '\0'; // Null-terminate the output string
}

static void sanitize_command(const char *input, char *output)
{
    const char *source = input;
    char *dest = output;

    // Disallowed commands
    const char *commands[] = {"rm", "ls", "cd", "wget", "curl", "shutdown", "reboot"};

    while (*source) {
        // Check for disallowed commands
        for (int i = 0; i < sizeof(commands) / sizeof(commands[0]); i++) {
            size_t cmd_len = strlen(commands[i]);
            if (strncasecmp(source, commands[i], cmd_len) == 0 && !isalpha(*(source + cmd_len))) {
                // Skip the disallowed command
                source += cmd_len;
                continue;
            }
        }
        // Copy other characters as is
        *dest = *source;
        dest++;
        source++;
    }
    *dest = '\0'; // Null-terminate the output string
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
