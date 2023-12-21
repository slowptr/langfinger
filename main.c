#include "LF_Chrome.c"

// LF_[Module]_[Function]  = public
// LF_[Module]__[Function] = private

int
main ()
{
  printf ("langfinger v0.0.1\n");

  struct LF_Chrome_data_struct chrome_data;
  LF_Chrome_populate_data (&chrome_data);

  if (chrome_data.profile_list->length == 0)
    {
      printf ("No profiles found.\n");
      return 0;
    }

  for (int i = 0; i < chrome_data.profile_list->length; i++)
    {
      struct LF_Chrome_profile_data_struct *profile_data
          = LF_Utils_arraylist_get (chrome_data.profile_list, i);

      printf ("Profile: %s\n", profile_data->path);
      for (int i = 0; i < profile_data->login_data_list->length; i++)
        {
          struct LF_Chrome_login_data_struct *login_data
              = LF_Utils_arraylist_get (profile_data->login_data_list, i);
          printf ("[login] %s\n", login_data->action_url);
          printf ("            %s\n", login_data->username_value);
          printf ("            %s\n", login_data->password_value);
        }
      for (int i = 0; i < profile_data->cookie_data_list->length; i++)
        {
          // cookies should probably be saved separately, depending on the
          // profile
          struct LF_Chrome_cookie_data_struct *cookie_data
              = LF_Utils_arraylist_get (profile_data->cookie_data_list, i);
          printf ("[cookie] %s\n", cookie_data->host_key);
          printf ("            %s\n", cookie_data->name);
          printf ("            %s\n", cookie_data->value);
        }
    }

  LF_Chrome_free_data (&chrome_data);
  return 0;
}
