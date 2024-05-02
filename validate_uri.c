<<<<<<< HEAD
/* validate_uri.c - Cleans uris from unsafe structures
 * TEAM MEMBERS:
 *      Debbie Lim
 *      Jai Manacsa
 *      Utsav Bhandari
 */

#include <stdlib.h> 
#include <stdio.h> 
#include <string.h> 
#include <stdbool.h> 
=======
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
>>>>>>> refs/remotes/origin/main
#include <ctype.h>

#define MAXLINE 8192

// Check for valid characters
static bool valid_uri_char(char c)
{
  return isalnum(c) || c == '.' || c == '-' || c == '~' || c == ':' || c == '/' || c == '?' || c == '#' || c == '[' || c == ']' || c == '@' || c == '!' || c == '$' || c == '&' || c == '\'' || c == '(' || c == ')' || c == '*' || c == '+' || c == ',' || c == ';' || c == '=' || c == '%';
}

static char *validate_uri(const char *uri)
{
  if (!uri || *uri == '\0')
  {
    return NULL;
  }

  char *sanitized_uri = malloc(MAXLINE);
  if (sanitized_uri == NULL) 
  {
    return NULL;
  }  

  int i = 0;
  while (*uri && i < MAXLINE - 1)
  {
    if (!valid_uri_char(*uri))
    {
      sanitized_uri[i++] = '%';
    }
    else
    {
      sanitized_uri[i++] = *uri; // Copy allowed character as is
    }
    uri++;
  }
  sanitized_uri[i] = '\0';
  return sanitized_uri;
}

// Check for ".." sequences
static bool safe_path(const char *path)
{
  return strstr(path, "..") == NULL;
}

// XSS protection (HTML/javascript code)
bool has_html_tags(const char *input)
{
  const char *source = input;
  bool inside_tag = false;
  const char *tags[] = {"<html>", "<head>", "<title>", "<body>", "<script>", "<div>", "<span>", "<p>", "<a>", "<img>", "<table>", "<tr>", "<td>", "<h1>", "<h2>", "<h3>", "<h4>", "<h5>", "<h6>", "<form>", "<input>", "<button>", "<label>", "<textarea>", "<select>", "<option>", "<style>", "<link>", "<meta>", "<!--", "<!--[if"};

  while (*source)
  {
    if (*source == '<')
    {
      // Check if the next characters match any of the tags
      for (int i = 0; i < sizeof(tags) / sizeof(tags[0]); i++)
      {
        if (strncmp(source, tags[i], strlen(tags[i])) == 0)
        {
          // Found a tag
          inside_tag = true;
          break;
        }
      }
    }
    else if (*source == '>')
    {
      inside_tag = false;
    }

    if (inside_tag)
    {
      return true; // If we are inside a tag, return true immediately
    }

    source++;
  }

  return false; // If no specific tags were found, return false
}
bool has_sql_injection(const char *input)
{
  const char *sql_keywords[] = {"SELECT", "INSERT", "UPDATE", "DELETE", "DROP", "CREATE", "ALTER", "EXEC", "TRUNCATE"};

  for (int i = 0; i < sizeof(sql_keywords) / sizeof(sql_keywords[0]); i++)
  {
    const char *keyword = sql_keywords[i];
    const char *found = strstr(input, keyword);
    if (found != NULL)
    {
      // Make sure the match is a standalone word
      if ((found == input || !isalnum(*(found - 1))) && !isalnum(*(found + strlen(keyword))))
      {
        return true;
      }
    }
  }
  return false;
}

bool has_commands(const char *uri)
{
  // List of illegal commands
  const char *commands[] = {"rm", "ls", "cd", "wget", "curl", "shutdown", "reboot"};

  // Convert URI to lowercase for case-insensitive matching
  size_t uri_length = strlen(uri);
  char *lowercase_uri = (char *)malloc(uri_length + 1);
  if (lowercase_uri == NULL)
  {
    // Memory allocation failed
    return false;
  }
  strcpy(lowercase_uri, uri);
  for (int i = 0; i <= uri_length; i++)
  {
    lowercase_uri[i] = tolower(lowercase_uri[i]);
  }

  // Check if any illegal command is present in the URI
  for (int i = 0; i < sizeof(commands) / sizeof(commands[0]); i++)
  {
    const char *cmd = commands[i];
    if (strstr(lowercase_uri, cmd) != NULL)
    {
      free(lowercase_uri); // Free dynamically allocated memory
      return true;
    }
  }
  free(lowercase_uri); // Free dynamically allocated memory
  return false;
}

char *sanitize_uri(const char *uri)
{
  char *sanitized_uri = validate_uri(uri);
  printf("Sanitized URI: %s \n", sanitized_uri);
  if (sanitized_uri == NULL)
  {
    printf("Error: Invalid URI.\n");
    return NULL;
  }
  if (has_html_tags(sanitized_uri))
  {
    printf("Error: HTML tags detected in the URI.\n");
    return NULL;
  }
  if (has_sql_injection(sanitized_uri))
  {
    printf("Error: SQL injection detected in the URI.\n");
    return NULL;

  }
  if (has_commands(sanitized_uri))
  {
    printf("Error: Disallowed commands detected in the URI.\n");
    return NULL;
  }
  if(!safe_path(sanitized_uri))
  {
    printf("Error: Unsafe path detected in the URI. \n");
    return NULL;
  }
  return sanitized_uri;
}
