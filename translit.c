/* Copyright (c) 2010-2012, Artem Shinkarov <artyom.shinkaroff@gmail.com>
  
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

#include <glib.h>
#ifdef _WIN32
#include <win32dep.h>
#endif
#define PURPLE_PLUGINS
#include <version.h>

#define PLUGIN_ID                "translit-plugin"
#define PLUGIN_STATIC_NAME       "TRANS"
#define PLUGIN_AUTHOR            "Artem Shinkarov <artyom.shinkaroff@gmail.com>"

#define PREFS_PREFIX             "/plugins/core/" PLUGIN_ID
#define PREFS_AUTO               PREFS_PREFIX "/auto"
#define PREFS_BUDDIES            PREFS_PREFIX "/buddies"

#include <util.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <debug.h>
#include <request.h>
#include <cmds.h>
#include <err.h>

#include "pidgin.h"


#include "gtkplugin.h"
#include "gtkprefs.h"
#include "gtkutils.h"
#include "gtkconvwin.h"


#define __unused __attribute__ ((unused))

/* Plugin global handle.  */
PurplePlugin *plugin_handle = NULL;

/* Russian pseudo-keyboard.  */
static int rus_loaded = 0;

static void
error_notify (PurpleConversation * conv, gchar * message)
{
  if (message)
    {
      PurpleMessageFlags flag = PURPLE_MESSAGE_SYSTEM | PURPLE_MESSAGE_ERROR;
      purple_conv_im_write (PURPLE_CONV_IM (conv), NULL, message, flag,
                            time (NULL));
    }
}

/* Detransliteration callback
     If a person is in the detransliteration list then each message
     from the person is going to be detransliterated.  */
static gboolean
reading_msg (PurpleAccount * account, char **sender,
             char **message, PurpleConversation * conv __unused,
             PurpleMessageFlags * flags __unused)
{

  char *txt;
  char *key;

  PurpleBuddy *buddy;
  buddy = purple_find_buddy (account, *sender);
  const char *name = purple_buddy_get_name (buddy);

  if (message && *message)
    {
      if (-1 == asprintf (&key, "%s/%s", PREFS_PREFIX, name))
        warnx ("asprintf failed");

      purple_debug_misc (PLUGIN_ID, "trying to find = %s\n", key);

      if (purple_prefs_get_string (key) != NULL)
        {
          purple_debug_misc (PLUGIN_ID, "message = %s\n", *message);
          txt = detrans (*message);
          free (*message);
          *message = txt;
        }
      free (key);
    }

  return FALSE;
}



PurpleCmdRet
detrans_cb (PurpleConversation * conv,
            const gchar * cmd __unused, gchar ** args, 
            gchar ** error __unused, void *data __unused)
{

  PurpleBuddy *buddy;
  PurpleAccount *account;
  const char *name;
  char *key;

  account = purple_conversation_get_account (conv);
  buddy = purple_find_buddy (account, args[0]);
  if (buddy == NULL)
    {
      char *t;
      if (-1 == asprintf (&t, "Cannot find buddy '%s'!", args[0]))
        warnx ("asprintf failed");

      error_notify (conv, t);
      free (t);

      return PURPLE_CMD_RET_OK;
    }
  else
    {
      name = purple_buddy_get_name (buddy);
      if (-1 == asprintf (&key, "%s/%s", PREFS_PREFIX, name))
        warnx ("asprintf failed");

      purple_debug_misc (PLUGIN_ID, "trying to add = %s\n", key);
      
      if (purple_prefs_get_string (key) == NULL)
        purple_prefs_add_string (key, "1");

      free (key);
    }

  return PURPLE_CMD_RET_OK;
}

PurpleCmdRet
nodetrans_cb (PurpleConversation * conv,
              const gchar * cmd __unused, gchar ** args, 
              gchar ** error __unused, void *data __unused)
{

  PurpleBuddy *buddy;
  PurpleAccount *account;
  const char *name;
  char *key;

  account = purple_conversation_get_account (conv);
  buddy = purple_find_buddy (account, args[0]);
  if (buddy == NULL)
    {
      char *t;
      if (-1 == asprintf (&t, "Cannot find buddy '%s'!", args[0]))
        warnx ("asprintf failed");

      error_notify (conv, t);
      free (t);
      return PURPLE_CMD_RET_OK;
    }
  else
    {
      name = purple_buddy_get_name (buddy);
      if (-1 == asprintf (&key, "%s/%s", PREFS_PREFIX, name))
        warnx ("asprintf failed");

      purple_debug_misc (PLUGIN_ID, "trying to remove = %s\n", key);
      
      if (purple_prefs_get_string (key) != NULL)
        purple_prefs_remove (key);

      free (key);
    }

  return PURPLE_CMD_RET_OK;
}


PurpleCmdRet
rus_cb (PurpleConversation * conv __unused,
        const gchar * cmd __unused, gchar ** args __unused,
        gchar ** error __unused, void *data __unused)
{

  if (rus_loaded == 0)
    rus_loaded = 1;

  return PURPLE_CMD_RET_OK;
}

PurpleCmdRet
norus_cb (PurpleConversation * conv __unused,
          const gchar * cmd __unused, gchar ** args __unused,
          gchar ** error __unused, void *data __unused)
{

  if (rus_loaded != 0)
    rus_loaded = 0;

  return PURPLE_CMD_RET_OK;
}


/* Guard from recursion in keymap.def  */
static gchar *inserted_symbol = NULL;

static void
insert_text (GtkTextBuffer * buffer, GtkTextIter * iter,
             gchar * text, gint len)
{

  gchar *repl;
  GtkTextIter *start;

  start = gtk_text_iter_copy (iter);
  gtk_text_iter_set_offset (start, 0);

  if (gtk_text_buffer_get_text (buffer, start, iter, FALSE)[0] == '/')
    {
      gtk_text_iter_free (start);
      return;
    }

  gtk_text_iter_free (start);


  if (rus_loaded && len == 1)
    {
#define INPUT(a, b) case a: { repl = b; break; }
      switch (text[0])
        {
#include "keymap-ru.def"
#undef INPUT
        default:
           return;
        }

      if (text == inserted_symbol)
        return;

      inserted_symbol = repl;
      gtk_text_buffer_backspace (buffer, iter, FALSE, TRUE);
      gtk_text_buffer_insert (buffer, iter, repl, strlen (repl));
    }
}


/* Attach GTK handler to each converstion.  */
static void
plugin_attach_conv (PurpleConversation * conv)
{
  PidginConversation *gtkconv;
  GtkTextView *view;
  GtkTextBuffer *buffer;

  gtkconv = PIDGIN_CONVERSATION (conv);
  view = GTK_TEXT_VIEW (gtkconv->entry);
  buffer = gtk_text_view_get_buffer (view);

  g_signal_connect_after (G_OBJECT (buffer), "insert-text",
                          G_CALLBACK (insert_text), NULL);

}


/* Remove GTK attached handlers.  */
static void
plugin_remove_attached (PidginConversation * gtkconv)
{
  GtkTextView *view;
  GtkTextBuffer *buffer;

  view = GTK_TEXT_VIEW (gtkconv->entry);
  buffer = gtk_text_view_get_buffer (view);

  g_signal_handlers_disconnect_matched (G_OBJECT (buffer),
                                        G_SIGNAL_MATCH_FUNC, 0, 0, NULL,
                                        G_CALLBACK (insert_text), NULL);
}


#define DETRANS_DESC \
        "/detrans <user-id>  marks a user and saves it in config "\
        "with de-transliteration flag, whic assumes that user is "\
        "writing in russian translit (see http://en.wikipedia.org"\
        "/wiki/Translit) for more details.\n\n"\
        "All the messages from the marked user are going to be tr"\
        "anslated into russian utf-8 letters.  In order to swithc"\
        " this feauture off use /nodetrans <user-id>.\n\n"

#define NODETRANS_DESC \
        "/nodetrans <user-id>  removes de-transliteration flag fr"\
        "om the given user.  If you don't know what is de-transli"\
        "teration flag, please run /help detrans.\n\n"

#define RUS_DESC \
        "/rus switches russian keyboard layout for all the conver"\
        "sations.  It is useful in case you are not allowed to ad"\
        "d layouts on the system, but you want to type in russian"\
        ".\n\n"\
        "In order to turn this feature off, run /norus\n\n"\

#define NORUS_DESC \
        "/norus turns russian layout for all the conversaions.  S"\
        "ee /help rus for more details.\n\n"

static gboolean
plugin_load (PurplePlugin * plugin)
{
  void *convs_handle;

  detrans_init ();
  
  convs_handle = purple_conversations_get_handle ();

  purple_signal_connect 
    (convs_handle,
     "conversation-created",
     plugin, PURPLE_CALLBACK (plugin_attach_conv), NULL);

  purple_signal_connect 
    (convs_handle,
     "receiving-im-msg",
     plugin, 
     PURPLE_CALLBACK (reading_msg), 
     NULL);

  purple_cmd_register 
    ("detrans",                 /*command name */
     "w",                       /*args */
     0,                         /*priority */
     PURPLE_CMD_FLAG_IM,        /*flags */
     NULL,                      /*prpl id not needed */
     detrans_cb,                /*callback function */
     DETRANS_DESC,              /*help string */
     NULL                       /* user defined data not needed */
    );

  purple_cmd_register 
    ("nodetrans",               /*command name */
     "w",                       /*args */
     0,                         /*priority */
     PURPLE_CMD_FLAG_IM,        /*flags */
     NULL,                      /*prpl id not needed */
     nodetrans_cb,              /*callback function */
     NODETRANS_DESC,            /*help string */
     NULL                       /* user defined data not needed */
    );

  purple_cmd_register 
    ("rus",                     /*command name */
     "",                        /*args */
     0,                         /*priority */
     PURPLE_CMD_FLAG_IM,        /*flags */
     NULL,                      /*prpl id not needed */
     rus_cb,                    /*callback function */
     RUS_DESC,                  /*help string */
     NULL                       /* user defined data not needed */
    );

  purple_cmd_register 
    ("norus",                   /*command name */
     "",                        /*args */
     0,                         /*priority */
     PURPLE_CMD_FLAG_IM,        /*flags */
     NULL,                      /*prpl id not needed */
     norus_cb,                  /*callback function */
     NORUS_DESC,                /*help string */
     NULL                       /* user defined data not needed */
    );


  plugin_handle = plugin;
  return TRUE;
}


static gboolean
plugin_unload (PurplePlugin * plugin __unused)
{
  GList *convs;

  detrans_free ();
  for (convs = purple_get_conversations (); convs != NULL;
       convs = convs->next)
    {
      PurpleConversation *conv = (PurpleConversation *) convs->data;
      if (PIDGIN_IS_PIDGIN_CONVERSATION (conv))
        plugin_remove_attached (PIDGIN_CONVERSATION (conv));
    }

  return TRUE;
}

static PurplePluginInfo info = {
  PURPLE_PLUGIN_MAGIC,                  /* Magic                */
  PURPLE_MAJOR_VERSION,
  PURPLE_MINOR_VERSION,
  PURPLE_PLUGIN_STANDARD,               /* plugin type          */
  NULL,                                 /* ui requirement       */
  0,                                    /* flags                */
  NULL,                                 /* dependencies         */
  PURPLE_PRIORITY_DEFAULT,              /* priority             */
  PLUGIN_ID,                            /* plugin id            */
  NULL,                                 /* name                 */
  "1.1.0",                              /* version              */
  NULL,                                 /* summary              */
  NULL,                                 /* description          */
  PLUGIN_AUTHOR,                        /* author               */
  "http://www.google.com/",             /* website              */
  plugin_load,                          /* load                 */
  plugin_unload,                        /* unload               */
  NULL,                                 /* destroy              */
  NULL,                                 /* ui_info              */
  NULL,                                 /* extra_info           */
  NULL,                                 /* prefs_info           */
  NULL,                                 /* actions              */
  NULL,                                 /* reserved 1           */
  NULL,                                 /* reserved 2           */
  NULL,                                 /* reserved 3           */
  NULL                                  /* reserved 4           */
};



static void
init_plugin (PurplePlugin * plugin __unused)
{
  info.name = "Translit tools";
  info.summary = "De-transliterates of messages | Russian keyboard layout";
  info.description =
    "/detrans command will switch transliteration of input "
    "messages of the user specified.\n\n"
    "/nodetrans command will switch off de-transliteration of the "
    "mesages for the user specified\n\n"
    "/rus command will convert all the english letters you type in "
    "instant message into russian letters, using russian keybord layout. "
    "if you type '/' at the beginning of message conversion will not occur."
    "\n\n/norus switches off russian keyboard layout";

  purple_prefs_add_none (PREFS_PREFIX);
}

PURPLE_INIT_PLUGIN (PLUGIN_STATIC_NAME, init_plugin, info)
