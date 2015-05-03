/* Copyright (c) 2010-2015, Artem Shinkarov <artyom.shinkaroff@gmail.com>
  
   Permission to use, copy, modify, and/or distribute this software for any
   purpose with or without fee is hereby granted, provided that the above
   copyright notice and this permission notice appear in all copies.
  
   THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
   WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
   MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
   ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
   WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
   ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
   OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.  */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <weechat-plugin.h>

#include "detrans.h"


#define PLUGIN_NAME "detrans"

WEECHAT_PLUGIN_NAME (PLUGIN_NAME);
WEECHAT_PLUGIN_DESCRIPTION (N_("Convert transliterated messages in russian"));
WEECHAT_PLUGIN_AUTHOR ("Artem Shinkarov <artyom.shinkaroff@gmail.com>");
WEECHAT_PLUGIN_VERSION ("0.1");
WEECHAT_PLUGIN_LICENSE ("ISC");

struct t_weechat_plugin *  weechat_plugin = NULL;
struct t_hook *  detrans_hook = NULL;

const char *  detrans_user = "posobie@gmail.com";
static char **  detrans_users = NULL;
size_t detrans_users_len = 0;


static inline int
user_in_detrans_users (const char *  user)
{
  for (size_t i = 0; i < detrans_users_len; i++)
    if (!strcmp (user, detrans_users[i]))
      return 1;

  return 0;
}


char *
detrans_cb (void *  data, const char *  modifier,
	    const char *  modifier_data, const char *  message)
{
  (void) data;
  (void) modifier;
  (void) modifier_data;

  if (!message)
    return NULL;

  char *  sender = strchr (message, '!');
  if (!sender)
    {
      weechat_printf (NULL, _("%s%s: cannot find '!' in the message [%s]"),
		      weechat_prefix ("error"), PLUGIN_NAME, message);
      return strdup (message);
    }

  sender += 1;

  if (user_in_detrans_users (detrans_user))
    {
      char *  msg_body = strchr (message + 1, ':');
      if (!msg_body)
	{
	  weechat_printf (NULL,
			  _("%s%s: cannot find second ':' in the message [%s]"),
			  weechat_prefix ("error"), PLUGIN_NAME, message);
	  return strdup (message);
	}

      msg_body += 1;

      char *  detransed_msg = detrans (msg_body);
      char *  new_msg;

      if (-1 == asprintf (&new_msg, _("%.*s%s"), msg_body - message, message,
		          detransed_msg))
	{
	  weechat_printf (NULL, _("%s%s: asprintf failed"),
			  weechat_prefix ("error"), PLUGIN_NAME);
	  return strdup (message);
	}

      free (detransed_msg);
      return new_msg;
    }

  return strdup (message);

}

void
free_detrans_users ()
{
  for (size_t i = 0; i < detrans_users_len; i++)
    free (detrans_users[i]);

  free (detrans_users);
  detrans_users = NULL;
  detrans_users_len = 0;
}

void
load_detrans_users (const char *users)
{
  detrans_users_len = 1;
  for (size_t i = 0; i < strlen (users); i++)
    if (users[i] == ',')
      detrans_users_len++;

  detrans_users = malloc (sizeof (char *) * detrans_users_len);

  const char *  end = users + strlen (users);
  const char *  start = users;
  const char *  comma;
  size_t l = 0;

  while ((comma = strchr (start, ',')) != NULL)
    {
      size_t len = comma - start;
      char *  t = malloc (len + 1);

      t = strncpy (t, start, len);
      t[len] = '\0';
      detrans_users[l++] = t;

      start = comma + 1;
      while (start < end && (*start == ' ' || *start == '\t'))
	start++;
    }

  if (start < end)
    detrans_users[l] = strdup (start);

}



int
detrans_users_config_cb (void *  data, const char *  option, const char *  value)
{
  (void) data;
  (void) option;

  if (detrans_users != NULL)
    free_detrans_users ();

  load_detrans_users (value);

  return WEECHAT_RC_OK;
}

int
weechat_plugin_init (struct t_weechat_plugin *  plugin, int argc, char *  argv[])
{
  /* make C compiler happy */
  (void) argc;
  (void) argv;

  weechat_plugin = plugin;
  detrans_init ();

  weechat_printf (NULL, "Hello from %s plugin!",
		  weechat_plugin_get_name (plugin));

  weechat_hook_config ("plugins.var.detrans.users", &detrans_users_config_cb,
		       NULL);

  struct t_config_option *  option = weechat_config_get ("plugins.var.detrans.users");
  const char *  detrans_users_opt = NULL;

  if (option != NULL)
    {
      detrans_users_opt = weechat_config_string (option);
      load_detrans_users (detrans_users_opt);
    }
  else
    {
      weechat_printf (NULL,
		      "%s: please set a list of users which messages have to be "
		      "transliterated to 'plugins.var.detrans.users'. "
		      "A list of users like: \"user1@host1,user2@host2,...\"",
		      PLUGIN_NAME);
    }

  for (size_t i = 0; i < detrans_users_len; i++)
    weechat_printf (NULL, "%s: user (%zu) [%s]", PLUGIN_NAME, i,
		    detrans_users[i]);

  detrans_hook = weechat_hook_modifier ("irc_in2_privmsg", &detrans_cb, NULL);

  return WEECHAT_RC_OK;
}


int
weechat_plugin_end (struct t_weechat_plugin *plugin)
{
  /* make C compiler happy */
  (void) plugin;

  detrans_free ();
  if (detrans_hook)
    {
      weechat_unhook (detrans_hook);
      detrans_hook = NULL;
    }

  free_detrans_users ();

  return WEECHAT_RC_OK;
}
