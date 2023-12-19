#include <Windows.h>
#include <stdio.h>
#include <stdlib.h>

#define LF_UTILS__DIR_SEARCH "\\*"

struct LF_Utils_arraylist_struct
{
  void **array;
  int length;
  int size;
};
struct LF_Utils_arraylist_struct *
LF_Utils_arraylist_new (int size)
{
  struct LF_Utils_arraylist_struct *out;
  out = malloc (sizeof (struct LF_Utils_arraylist_struct));
  out->array = malloc (sizeof (void *) * size);
  out->length = 0;
  out->size = size;
  return out;
}
struct LF_Utils_arraylist_struct *
LF_Utils_arraylist_new_default ()
{
  return LF_Utils_arraylist_new (10);
}
void
LF_Utils_arraylist_free (struct LF_Utils_arraylist_struct *arraylist)
{
  free (arraylist->array);
  free (arraylist);
}
void
LF_Utils_arraylist_add (struct LF_Utils_arraylist_struct *arraylist,
                        void *item)
{
  if (arraylist->length == arraylist->size)
    {
      arraylist->size *= 2;
      arraylist->array
          = realloc (arraylist->array, sizeof (void *) * arraylist->size);
    }
  arraylist->array[arraylist->length] = item;
  arraylist->length++;
}
void *
LF_Utils_arraylist_get (struct LF_Utils_arraylist_struct *arraylist, int index)
{
  if (index < 0 || index >= arraylist->length)
    {
      return NULL;
    }
  return arraylist->array[index];
}

void
LF_Utils__loop_through_dir (char *dir_path,
                            void (*callback) (char *, WIN32_FIND_DATA, void *),
                            void **data)
{
  char search[MAX_PATH];
  strcpy (search, dir_path);
  strcat (search, LF_UTILS__DIR_SEARCH);

  WIN32_FIND_DATA ffd;
  HANDLE h_find = FindFirstFile (search, &ffd);
  if (h_find == INVALID_HANDLE_VALUE)
    {
      fprintf (stderr, "FindFirstFile failed (%lu)\n", GetLastError ());
      exit (-1);
    }

  do
    {
      callback (dir_path, ffd, *data);
    }
  while (FindNextFile (h_find, &ffd) != 0);

  FindClose (h_find);
}

void
LF_Utils_read_file (char *path, char *out, int out_len)
{
  HANDLE file_handle = CreateFile (path, GENERIC_READ, 0, NULL, OPEN_EXISTING,
                                   FILE_ATTRIBUTE_NORMAL, NULL);
  if (file_handle == INVALID_HANDLE_VALUE)
    {
      fprintf (stderr, "unable to open local state file (%lu)\n",
               GetLastError ());
      exit (-1);
    }

  DWORD bytes_read;
  if (!ReadFile (file_handle, out, out_len, &bytes_read, NULL))
    {
      fprintf (stderr, "unable to read local state file (%lu)\n",
               GetLastError ());
      exit (-1);
    }

  CloseHandle (file_handle);
}

char *
LF_Utils_find_str_in_range (char *str, const char *before_str,
                            const char *after_str, int *out_len)
{
  char *start_ptr = strstr (str, before_str);
  if (start_ptr == NULL)
    {
      fprintf (stderr, "unable to find string start [%s]\n", before_str);
      exit (-1);
    }
  start_ptr += strlen (before_str);

  char *end_ptr = strstr (start_ptr, after_str);
  if (end_ptr == NULL)
    {
      fprintf (stderr, "unable to find string end [%s]\n", after_str);
      exit (-1);
    }

  *out_len = end_ptr - start_ptr;
  char *out = malloc (*out_len + 1);
  strncpy (out, start_ptr, *out_len);
  out[*out_len] = '\0';

  return out;
}
