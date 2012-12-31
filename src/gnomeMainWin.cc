/*
 * gnomeMainWin.cc
 *
 * Description: gnome implementation of GUI main window
 *
 * Copyright (c) 1999, David Z. Creemer.
 * Copyright (c) 2000, Brian Craft.
 * See the LICENSE file. All rights not granted therein are reserved.
 *
 * @author David Z. Creemer
 * @author Brian Craft
 * $Revision: 1.27 $
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

#include "sys.h"
#include <cstdio>
#include <stack>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include <gdk/gdkx.h>
#include <gnome.h>
#include <libgnomeui/gnome-about.h>

using namespace std;

#include "Voice.h"

#include "MainWin.h"
#include "Error.h"
#include "App.h"
#include "EventStream.h"
#include "ParseEventStream.h"
#include "xvoice.h"
#include "mouseGrid.h"
#include "EventRecord.h"

/*
 * We export this for Target. There's no clean way to pass it in, and Target
 * currently only handles one Display, anyway. We can worry about it when we
 * get requests from folk with dual head systems.
 */
Display    *display;

static void dismiss(int vh, VVocabResult ev, const char *ph, int val, 
    void * mw);

static int vhdl; /* XXX this should go away */

class gnomeMainWindow : public MainWindow
{
  public:

    gnomeMainWindow();
    ~gnomeMainWindow();

    void mic(int);

    void state(int);
    void reco(const char *text, bool firm);
    void errorMsg(int sev, const char* fn, const char* fmt, ...);
    void error(int err, const char *msg);
    void vocab(const char *name, bool active);
    void initVocabs();
    void setTarget(const char *);

    void main(bool, bool);
    void installTimeout(int (*fn)(void*), int to, void *data);

    void set_status(const char *status_string);

    friend MainWindow *getMainWindow(int argc, char **argv, 
        const struct poptOption* opt);
  protected:

    void createMainWindow();

    friend void dismiss(int vh, VVocabResult ev, const char *ph, int val, 
      void * mw);
    friend void err_dialog_destroy(GtkWidget *dialog, gpointer data);
  protected:

    GtkWidget *_mainwin;
    GtkWidget *_targetWindow;
    GtkWidget *_textWindow;
    GtkWidget *_statusBar;
    GtkWidget *_micButton;
    GtkWidget *_vocabWindow;
    guint _mic_signal;
    list<GtkWidget*> _errStack;
    bool _errVocab;
};

gnomeMainWindow::gnomeMainWindow() :
  _mainwin(NULL)
{
}

gnomeMainWindow::~gnomeMainWindow()
{
  // FIXME should this be removed?
#if 0
  gint width,height;
  /*
   * I would really like the hpaned to remember its position.
   * but there's no way to query it. I'd also like the main
   * window to remember its size, but all the docs claim that's
   * the responsibilty of the WM.
   */
  if (_mainwin->window != NULL) {
    gdk_window_get_size(_mainwin->window, &width, &height);
    printf("width %d height %d\n",width, height);
    gnome_config_set_int("/xvoice/layout/width", width);
    gnome_config_set_int("/xvoice/layout/height", height);
    gnome_config_set_int("/xvoice/layout/hpan", height);
  }
#endif
  //delete _target;
}

static void CloseUpShop(GtkWidget* UNUSED(widget), gpointer UNUSED(data))
{

  //deallocate memory???

  VClose(vhdl);
  gtk_main_quit();
}

static VVocab msgVocab[] = {
  { 0, "dismiss" },
  { -1, NULL }
};

static void dismiss(int UNUSED(vh), VVocabResult ev, char const* UNUSED(ph), int UNUSED(val), void* mw)
{
  gnomeMainWindow *gmw = (gnomeMainWindow*)mw;
  GtkWidget *dialog = gmw->_errStack.front();
  switch (ev) {
    case V_VOCAB_FAIL:
      break;
    case V_VOCAB_SUCCESS:
      break;
    case V_VOCAB_RECO:
      gtk_widget_destroy(dialog);
      break;
  }
}

// Helper app for closing dialog boxes. Used in signal connects.
void err_dialog_destroy(GtkWidget *dialog, gpointer data)
{
  gnomeMainWindow *gmw = (gnomeMainWindow*)data;

  gmw->_errStack.remove(dialog);
  if (gmw->_errStack.size() == 0) VDisableVocab(vhdl, "msgVocab");
}

// Helper app for closing xvoice. Used in signal connects.
void app_destroy(GtkWidget* UNUSED(dialog), gpointer UNUSED(data))
{
  exit(-1);
}

void gnomeMainWindow::installTimeout(int (*fn)(void*), int UNUSED(to), void *data)
{
  gtk_timeout_add(800, fn, data);
}

void gnomeMainWindow::initVocabs()
{
  VDefineVocab(vhdl, "msgVocab", dismiss, msgVocab, (void *)this);

  // Pass initial information about the root window to the mouse grid
  initGrid(GDK_DISPLAY(), GDK_ROOT_WINDOW(), (MainWindow*)this);
}

void gnomeMainWindow::errorMsg(int sev, const char* fn, const char* fmt, ...)
{
  GtkWidget *dialog;
  fstring msg;
  va_list ap;
  va_start(ap, fmt);
  msg.vappendf(fmt, ap);
  msg.appendf(" in %s", fn);

  if (sev == E_CONFIG) {
    dialog = gnome_ok_dialog_parented
      (msg.c_str(), GTK_WINDOW(_mainwin));
    gtk_signal_connect(GTK_OBJECT(dialog), "destroy",
        GTK_SIGNAL_FUNC(err_dialog_destroy), this);
  } else if (sev == E_SEVERE) {
    dialog = gnome_warning_dialog_parented
      (msg.c_str(), GTK_WINDOW(_mainwin));
    gtk_signal_connect(GTK_OBJECT(dialog), "destroy",
        GTK_SIGNAL_FUNC(err_dialog_destroy), this);
  } else { // sev == E_FATAL
    dialog = gnome_error_dialog_parented
      (msg.c_str(), GTK_WINDOW(_mainwin));
    gtk_signal_connect(GTK_OBJECT(dialog), "destroy",
        GTK_SIGNAL_FUNC(app_destroy), this);
  }

  _errStack.push_front(dialog);
  if (_errStack.size() == 1 && VGetState()&V_STATE_RUNNING)
    VEnableVocab(vhdl, "msgVocab");
  va_end(ap);
}

void gnomeMainWindow::error(int UNUSED(err), char const* msg)
{
  errorMsg(E_CONFIG, "", msg);
}

/* 
 * If active is true, indicate that the named vocab in enabled by
 * adding it to the visual vocab list. If active is false, indicate
 * that the named vocab is disabled by removing it from the list. If
 * the named vocab does not exist and active is false, do nothing. 
 */
void gnomeMainWindow::vocab(const char *name, bool active)
{
  if (active) {
    /* This extra casting step is necessary to compile under gcc 2.91 */
    gchar *n = (gchar *)name;
    gtk_clist_append( GTK_CLIST(_vocabWindow), &n);
  } else {
    int i;
    char *text;
    /*
     * XXX - I'm breaking the clist API here, because it's
     * braindamaged that you can't get a row count out of it.
     */
    for (i = 0; i < GTK_CLIST(_vocabWindow)->rows; ++i) {
      gtk_clist_get_text(GTK_CLIST(_vocabWindow), i, 0, &text);
      if (!strcmp(name, text)) {
        gtk_clist_remove(GTK_CLIST(_vocabWindow), i);
        break;
      }
    }
  }
  gtk_clist_moveto(GTK_CLIST(_vocabWindow), GTK_CLIST(_vocabWindow)->rows,
      0, 1, 0);

}

void gnomeMainWindow::setTarget(const char *name)
{
  dbgprintf(("setting target to %s\n", name));
  gtk_clist_set_text( GTK_CLIST( _targetWindow ), 0, 2, name);
}

static void notify (int socket_handle, int (*recv_fn)(void*),
    void *recv_data, void *client_data)
{
  int *x_input_id = (int*) client_data;

  if (recv_fn == NULL) {
    gdk_input_remove(*x_input_id);
  }

  *x_input_id = gdk_input_add(socket_handle,(GdkInputCondition) 1,
      (GdkInputFunction)recv_fn, recv_data);
  return;
}

void ToggleMic(GtkWidget* widget, gpointer UNUSED(data))
{
  dbgprintf(("state %d\n", (GTK_TOGGLE_BUTTON (widget)->active)));
  if (GTK_TOGGLE_BUTTON (widget)->active) {
    vCommand (CMICON);
  } else {
    vCommand (CMICOFF);
  }
}

void gnomeMainWindow::mic(int state)
{
  gtk_signal_handler_block(GTK_OBJECT(_micButton), _mic_signal);
  gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON(_micButton), state);
  gtk_signal_handler_unblock(GTK_OBJECT(_micButton), _mic_signal);
  gnome_appbar_set_status(GNOME_APPBAR(_statusBar), 
      state?"Mic is on":"Mic is off");
}

/* 
 * previously these were invoked by a menu button.
 * Now they are invoked by the voice module when the state changes.
 * The menu options (when they reappear) will call the Voice module to 
 * change the state.
 */

void gnomeMainWindow::state(int state)
{
  int command = (state & V_STATE_COMMAND);
  int dictate = (state & V_STATE_DICTATE);

  if (! state & V_STATE_RUNNING) {
    gtk_clist_set_text( GTK_CLIST( _targetWindow ), 0, 1, "Engine down");
  }
  if (command && dictate)
    gtk_clist_set_text( GTK_CLIST( _targetWindow ), 0, 1, "Command/Dictate" );
  else if (command)
    gtk_clist_set_text( GTK_CLIST( _targetWindow ), 0, 1, "Command" );
  else if (dictate)
    gtk_clist_set_text( GTK_CLIST( _targetWindow ), 0, 1, "Dictate" );
  else
    gtk_clist_set_text( GTK_CLIST( _targetWindow ), 0, 1, "Idle" );
}

/*
 * Insert text (black if the word is firm, greyed-out otherwise)
 * into xvoice window, followed by a comma
 */
void gnomeMainWindow::reco(const char *text, bool firm)
{
  if (firm)
    gtk_text_insert (GTK_TEXT (_textWindow), NULL,
        &_textWindow->style->black, NULL, text, -1);
  else
    gtk_text_insert (GTK_TEXT (_textWindow), NULL,
        &_textWindow->style->mid[0], NULL, text, -1);
  gtk_text_insert (GTK_TEXT (_textWindow), NULL,
      &_textWindow->style->black, NULL, ", ", -1);
}

static void rebuild_cb(GtkWidget*, gpointer)
{
  loadGrammars();
}

/*
 * Utility functions for setting up radio button groups
 * on a property box.
 * The group has a name, and a value (which button is pressed).
 * The value is set as object data on the widget w.
 */

static void radio_cb(GtkRadioButton *btn, gpointer prop)
{
  char *name = (char *)gtk_object_get_data(GTK_OBJECT(btn), "name");
  gpointer data = gtk_object_get_data(GTK_OBJECT(btn), "data");
  gtk_object_set_data(GTK_OBJECT(prop), name, data);
}

static void radio_buttons(gpointer *vals, const char **labels, int count,
    gpointer def, GtkWidget *prop, char const* name,
    GtkWidget **btns, GtkSignalFunc usr_cb)
{
  int i;
  GSList *grp = NULL;

  for (i = 0; i < count; ++i) {
    dbgprintf(("vals %d def %d\n", vals[i], def));
    btns[i] = gtk_radio_button_new_with_label(grp, labels[i]);
    gtk_object_set_data(GTK_OBJECT(btns[i]), "name", const_cast<char*>(name));
    gtk_object_set_data(GTK_OBJECT(btns[i]), "data", vals[i]);
    if (def == vals[i]) {
      dbgprintf(("def\n"));
      gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(btns[i]), TRUE);
      gtk_object_set_data(GTK_OBJECT(prop), name, vals[i]);
    }
    gtk_signal_connect(GTK_OBJECT(btns[i]), "pressed",
        GTK_SIGNAL_FUNC(radio_cb), prop);
    gtk_signal_connect(GTK_OBJECT(btns[i]), "pressed",
        GTK_SIGNAL_FUNC(usr_cb), prop);
    grp = gtk_radio_button_group(GTK_RADIO_BUTTON(btns[i]));
  }
}

static void set_prop_changed(GtkWidget* UNUSED(rb), gpointer prop)
{
  gnome_property_box_changed(GNOME_PROPERTY_BOX(prop));
}

GSList *addRadioButton (GtkWidget *box, gchar *label, GSList *group)
{
  GtkWidget *radio;
  radio = gtk_radio_button_new_with_label(group, label);
  gtk_box_pack_start (GTK_BOX(box), radio, TRUE, TRUE, 5);
  return(gtk_radio_button_group (GTK_RADIO_BUTTON(radio)));
}

void delete_dialog_cb(GtkWidget* UNUSED(window), gpointer data)
{
  gtk_widget_destroy((GtkWidget*)data);
}

void make_case_defaults_list(GList *case_options, gchar *case_default) {

  if (g_strcasecmp(case_default, "Normal") != 0) {
    case_options = g_list_append(case_options, (void *)"Normal");
  }

  if (g_strcasecmp(case_default, "Capitalize") != 0) {
    case_options = g_list_append(case_options, (void *)"Capitalize");
  }

  if (g_strcasecmp(case_default, "Uppercase") != 0) {
    case_options = g_list_append(case_options, (void *)"Uppercase");
  }

  if (g_strcasecmp(case_default, "Lowercase") != 0) {
    case_options = g_list_append(case_options, (void *)"Lowercase");
  }

  if (g_strcasecmp(case_default, "TitleCase") != 0) {
    case_options = g_list_append(case_options, (void *)"TitleCase");
  }

}

void prefs_apply_cb(GtkWidget* dialog, gpointer UNUSED(data))
{
  // Dictation prefs
  gnome_config_set_int("xvoice/Dictation/DefaultSpacing",
      gtk_spin_button_get_value_as_int
      (GTK_SPIN_BUTTON(gtk_object_get_data
                       (GTK_OBJECT(dialog),
                        "DefaultSpacing"))));

  gnome_config_set_int("xvoice/Dictation/AfterSentenceSpacing",
      gtk_spin_button_get_value_as_int
      (GTK_SPIN_BUTTON(gtk_object_get_data
                       (GTK_OBJECT(dialog),
                        "AfterSentenceSpacing"))));
  gnome_config_set_int("xvoice/Dictation/AfterSentenceSpacing",
      gtk_spin_button_get_value_as_int
      (GTK_SPIN_BUTTON(gtk_object_get_data
                       (GTK_OBJECT(dialog),
                        "AfterSentenceSpacing"))));
  gnome_config_set_int("xvoice/Dictation/AfterParaSpacing",
      gtk_spin_button_get_value_as_int
      (GTK_SPIN_BUTTON(gtk_object_get_data
                       (GTK_OBJECT(dialog),
                        "AfterParaSpacing"))));
  gnome_config_set_int("xvoice/Dictation/AfterNewLineSpacing",
      gtk_spin_button_get_value_as_int
      (GTK_SPIN_BUTTON(gtk_object_get_data
                       (GTK_OBJECT(dialog),
                        "AfterNewLineSpacing"))));
  gnome_config_set_int("xvoice/Dictation/InitialSpacing",
      gtk_spin_button_get_value_as_int
      (GTK_SPIN_BUTTON(gtk_object_get_data
                       (GTK_OBJECT(dialog),
                        "InitialSpacing"))));

  gnome_config_set_bool("xvoice/Dictation/AfterNewLineCap",
      gtk_toggle_button_get_active
      (GTK_TOGGLE_BUTTON
       (gtk_object_get_data
        (GTK_OBJECT(dialog),
         "AfterNewLineCap"))));

  gnome_config_set_bool("xvoice/Dictation/SlowKeys",
      gtk_toggle_button_get_active
      (GTK_TOGGLE_BUTTON
       (gtk_object_get_data
        (GTK_OBJECT(dialog),
         "SlowKeys"))));

  gnome_config_set_string("xvoice/Dictation/DefaultCase",
      gtk_entry_get_text
      (GTK_ENTRY(GTK_COMBO(
                           (gtk_object_get_data
                            (GTK_OBJECT(dialog),
                             "DefaultCase"))) -> entry)));

  gnome_config_set_string("xvoice/Dictation/InitialCase",
      gtk_entry_get_text
      (GTK_ENTRY(GTK_COMBO(
                           (gtk_object_get_data
                            (GTK_OBJECT(dialog),
                             "InitialCase"))) -> entry)));

  // Sync to Gnome config file
  gnome_config_sync();

  gboolean def;
  char *systemCase = gnome_config_get_string_with_default
        ("xvoice/Dictation/DefaultCase=Normal", &def);
  int systemSpace =  gnome_config_get_int_with_default
        ("xvoice/Dictation/DefaultSpacing=1", &def);
  appSetSystemState(systemCase, systemSpace);
  g_free(systemCase);

  // Engine parameter prefs
  VEngineParam ep;
  ep.unambiguous_to = (unsigned long) (1000.0 * ((GtkAdjustment *)
        gtk_object_get_data(GTK_OBJECT(dialog), "unambiguous"))->value);
  ep.command_to = (unsigned long) (1000.0 * ((GtkAdjustment *)
        gtk_object_get_data(GTK_OBJECT(dialog), "command"))->value);
  ep.text_to = (unsigned long) (1000.0 * ((GtkAdjustment *)
        gtk_object_get_data(GTK_OBJECT(dialog), "dictate"))->value);
  ep.threshold = (unsigned long) ((GtkAdjustment *)
      gtk_object_get_data(GTK_OBJECT(dialog), "threshold"))->value;
  /* XXX -- this is gross. maybe we shouldn't be passing int in void*'s */
  ep.performance = (VEnginePerformance)(int)gtk_object_get_data(GTK_OBJECT(dialog), "opt");

  dbgprintf(("performance %d\n", ep.performance));
  VSetEngineParam(vhdl, &ep);

}

void make_dictation_pref_page(GtkWidget *vbox, GtkWidget *dialog)
{
  GtkWidget *label;
  GtkWidget *hbox;
  GtkWidget *default_spacing_spin;
  GtkWidget *after_sentence_spacing_spin;
  GtkWidget *after_para_spacing_spin;
  GtkWidget *after_newline_spacing_spin;
  GtkWidget *after_newline_cap_checkbutton;
  GtkWidget *slow_keys_checkbutton;
  GtkWidget *initial_spacing_spin;
  GtkWidget *default_case_list;
  GtkWidget *initial_case_list;
  GtkObject *adj;
  GList     *case_options;
  gboolean  def;

  // hbox for the next two elements:
  hbox = gtk_hbox_new(FALSE, 5);

  /* DefaultSpacing (1) */
  label = gtk_label_new("Spacing between words:");
  adj = gtk_adjustment_new
    (gnome_config_get_float_with_default
     ("xvoice/Dictation/DefaultSpacing=1", &def), 0.0, 99.0, 1.0, 1.0, 1.0);
  default_spacing_spin = gtk_spin_button_new(GTK_ADJUSTMENT(adj), 1.0, 0);

  gtk_box_pack_start(GTK_BOX(hbox), label, TRUE, FALSE, 0);
  gtk_box_pack_start(GTK_BOX(hbox), default_spacing_spin, TRUE, FALSE, 0);
  gtk_box_pack_start(GTK_BOX(vbox), hbox, TRUE, FALSE, 0);

  gtk_label_set_justify(GTK_LABEL(label), GTK_JUSTIFY_RIGHT);
  gtk_signal_connect(GTK_OBJECT(default_spacing_spin), "changed",
      GTK_SIGNAL_FUNC(set_prop_changed), dialog);

  /* AfterSentenceSpacing (2) */
  label = gtk_label_new("Spacing after a sentence:");
  adj = gtk_adjustment_new
    (gnome_config_get_float_with_default
     ("xvoice/Dictation/AfterSentenceSpacing=2", &def),
     0.0, 99.0, 1.0, 1.0, 1.0);
  after_sentence_spacing_spin =
    gtk_spin_button_new(GTK_ADJUSTMENT(adj), 1.0, 0);

  gtk_box_pack_start(GTK_BOX(hbox), label, TRUE, FALSE, 0);
  gtk_box_pack_start(GTK_BOX(hbox), after_sentence_spacing_spin,
      TRUE, FALSE, 0);
  gtk_label_set_justify(GTK_LABEL(label), GTK_JUSTIFY_RIGHT);

  gtk_signal_connect(GTK_OBJECT(after_sentence_spacing_spin), "changed",
      GTK_SIGNAL_FUNC(set_prop_changed), dialog);


  // hbox for the next two elements:
  hbox = gtk_hbox_new(FALSE, 5);

  /* AfterNewLineSpacing (0) */
  label = gtk_label_new("Spacing after a newline:");
  adj = gtk_adjustment_new
    (gnome_config_get_float_with_default
     ("xvoice/Dictation/AfterNewLineSpacing=0", &def),
     0.0, 99.0, 1.0, 1.0, 1.0);
  after_newline_spacing_spin =
    gtk_spin_button_new(GTK_ADJUSTMENT(adj), 1.0, 0);

  gtk_box_pack_start(GTK_BOX(hbox), label, TRUE, FALSE, 0);
  gtk_box_pack_start(GTK_BOX(hbox), after_newline_spacing_spin, TRUE, FALSE, 0);
  gtk_box_pack_start(GTK_BOX(vbox), hbox, TRUE, FALSE, 0);
  gtk_label_set_justify(GTK_LABEL(label), GTK_JUSTIFY_RIGHT);

  gtk_signal_connect(GTK_OBJECT(after_newline_spacing_spin), "changed",
      GTK_SIGNAL_FUNC(set_prop_changed), dialog);

  /* AfterNewLineCap (true) */
  label = gtk_label_new("Capitalize after a newline:");
  after_newline_cap_checkbutton = gtk_check_button_new();
  // Set the active state of the checkbutton to true if AfterNewLineCap
  // is true, false if false.
  gtk_toggle_button_set_active
    (GTK_TOGGLE_BUTTON(after_newline_cap_checkbutton),
     gnome_config_get_bool_with_default
     ("xvoice/Dictation/AfterNewLineCap=true", &def));
  gtk_box_pack_start(GTK_BOX(hbox), label, TRUE, FALSE, 0);
  gtk_box_pack_start(GTK_BOX(hbox), after_newline_cap_checkbutton,
      TRUE, FALSE, 0);
  gtk_label_set_justify(GTK_LABEL(label), GTK_JUSTIFY_RIGHT);
  gtk_signal_connect(GTK_OBJECT(after_newline_cap_checkbutton), "toggled",
      GTK_SIGNAL_FUNC(set_prop_changed), dialog);


  // hbox for the next two elements:
  hbox = gtk_hbox_new(FALSE, 5);

  /* AfterParaSpacing (0) */
  label = gtk_label_new("Spacing after a paragraph:");
  adj = gtk_adjustment_new
    (gnome_config_get_float_with_default
     ("xvoice/Dictation/AfterParaSpacing=0", &def),
     0.0, 99.0, 1.0, 1.0, 1.0);
  after_para_spacing_spin =
    gtk_spin_button_new(GTK_ADJUSTMENT(adj), 1.0, 0);

  gtk_box_pack_start(GTK_BOX(hbox), label, TRUE, FALSE, 0);
  gtk_box_pack_start(GTK_BOX(hbox), after_para_spacing_spin, TRUE, FALSE, 0);
  gtk_box_pack_start(GTK_BOX(vbox), hbox, TRUE, FALSE, 0);
  gtk_label_set_justify(GTK_LABEL(label), GTK_JUSTIFY_RIGHT);

  gtk_signal_connect(GTK_OBJECT(after_para_spacing_spin), "changed",
      GTK_SIGNAL_FUNC(set_prop_changed), dialog);

  /* InitialSpacing (0) */
  label = gtk_label_new("Initial spacing in new app:");
  adj = gtk_adjustment_new
    (gnome_config_get_float_with_default
     ("xvoice/Dictation/InitialSpacing=0", &def),
     0.0, 99.0, 1.0, 1.0, 1.0);
  initial_spacing_spin = gtk_spin_button_new(GTK_ADJUSTMENT(adj), 1.0, 0);

  gtk_box_pack_start(GTK_BOX(hbox), label, TRUE, FALSE, 0);
  gtk_box_pack_start(GTK_BOX(hbox), initial_spacing_spin, TRUE, FALSE, 0);
  gtk_label_set_justify(GTK_LABEL(label), GTK_JUSTIFY_RIGHT);

  gtk_signal_connect(GTK_OBJECT(initial_spacing_spin), "changed",
      GTK_SIGNAL_FUNC(set_prop_changed), dialog);


  // hbox for the next two elements:
  hbox = gtk_hbox_new(FALSE, 5);

  /* SlowKeys */
  label = gtk_label_new("Send keystrokes slowly:");
  slow_keys_checkbutton = gtk_check_button_new();
  // Set the active state of the checkbutton to true if SlowKeys
  // is true, false if false.
  gtk_toggle_button_set_active
    (GTK_TOGGLE_BUTTON(slow_keys_checkbutton),
     gnome_config_get_bool_with_default
     ("xvoice/Dictation/SlowKeys=false", &def));
  gtk_box_pack_start(GTK_BOX(hbox), label, TRUE, FALSE, 0);
  gtk_box_pack_start(GTK_BOX(hbox), slow_keys_checkbutton,
      TRUE, FALSE, 0);
  gtk_box_pack_start(GTK_BOX(vbox), hbox, TRUE, FALSE, 0);
  gtk_label_set_justify(GTK_LABEL(label), GTK_JUSTIFY_RIGHT);
  gtk_signal_connect(GTK_OBJECT(slow_keys_checkbutton), "toggled",
      GTK_SIGNAL_FUNC(set_prop_changed), dialog);

  /* Put next new item here. */

  // hbox for the next two elements:
  hbox = gtk_hbox_new(FALSE, 5);

  /* DefaultCase (normal) */
  label = gtk_label_new("Default case:");
  default_case_list = gtk_combo_new();
  case_options = NULL;
  gchar *case_default = gnome_config_get_string_with_default
    ("xvoice/Dictation/DefaultCase=Normal", &def);
  case_options = g_list_append(case_options, case_default);
  make_case_defaults_list(case_options, case_default);
  gtk_combo_set_popdown_strings(GTK_COMBO(default_case_list), case_options);
  gtk_editable_set_editable
    (GTK_EDITABLE(GTK_COMBO(default_case_list)->entry), FALSE);

  gtk_signal_connect(GTK_OBJECT(GTK_COMBO(default_case_list)->entry),
      "changed", GTK_SIGNAL_FUNC(set_prop_changed), dialog);


  gtk_box_pack_start(GTK_BOX(hbox), label, TRUE, FALSE, 0);
  gtk_box_pack_start(GTK_BOX(hbox), default_case_list, TRUE, FALSE, 0);
  gtk_box_pack_start(GTK_BOX(vbox), hbox, TRUE, FALSE, 0);
  gtk_label_set_justify(GTK_LABEL(label), GTK_JUSTIFY_RIGHT);

  /* InitialCase (normal) */
  label = gtk_label_new("Initial case:");
  initial_case_list = gtk_combo_new();
  case_options = NULL;
  case_default = gnome_config_get_string_with_default
    ("xvoice/Dictation/InitialCase=Normal", &def);
  case_options = g_list_append(case_options, case_default);
  make_case_defaults_list(case_options, case_default);
  gtk_combo_set_popdown_strings(GTK_COMBO(initial_case_list), case_options);
  gtk_editable_set_editable
    (GTK_EDITABLE(GTK_COMBO(initial_case_list)->entry), FALSE);

  gtk_signal_connect(GTK_OBJECT(GTK_COMBO(initial_case_list)->entry),
      "changed", GTK_SIGNAL_FUNC(set_prop_changed), dialog);

  gtk_box_pack_start(GTK_BOX(hbox), label, TRUE, FALSE, 0);
  gtk_box_pack_start(GTK_BOX(hbox), initial_case_list, TRUE, FALSE, 0);

  // Set data on objects for future reference by apply callback.
  gtk_object_set_data(GTK_OBJECT(dialog), "DefaultSpacing",
      default_spacing_spin);
  gtk_object_set_data(GTK_OBJECT(dialog), "AfterSentenceSpacing",
      after_sentence_spacing_spin);
  gtk_object_set_data(GTK_OBJECT(dialog), "AfterParaSpacing",
      after_para_spacing_spin);
  gtk_object_set_data(GTK_OBJECT(dialog), "AfterNewLineSpacing",
      after_newline_spacing_spin);
  gtk_object_set_data(GTK_OBJECT(dialog), "AfterNewLineCap",
      after_newline_cap_checkbutton);
  gtk_object_set_data(GTK_OBJECT(dialog), "InitialSpacing",
      initial_spacing_spin);
  gtk_object_set_data(GTK_OBJECT(dialog), "SlowKeys",
      slow_keys_checkbutton);
  gtk_object_set_data(GTK_OBJECT(dialog), "DefaultCase",
      default_case_list);
  gtk_object_set_data(GTK_OBJECT(dialog), "InitialCase",
      initial_case_list);
}

static void engine_set_cb(GtkAdjustment *adj, gpointer dialog)
{
  GtkAdjustment *next, *prev;

  gnome_property_box_changed(GNOME_PROPERTY_BOX(dialog));

  next = (GtkAdjustment *)gtk_object_get_data(GTK_OBJECT(adj), "next");
  if (next != NULL) {
    if (next->value < adj->value)
      gtk_adjustment_set_value(next, adj->value);
  }

  prev = (GtkAdjustment *)gtk_object_get_data(GTK_OBJECT(adj), "prev");
  if (prev != NULL) {
    if (prev->value > adj->value)
      gtk_adjustment_set_value(prev, adj->value);
  }
}

void engine_opt_cb(GtkWidget *button, gpointer dialog)
{
  gnome_property_box_changed(GNOME_PROPERTY_BOX(dialog));
  gtk_object_set_data(GTK_OBJECT(dialog), "opt",
      gtk_object_get_data(GTK_OBJECT(button), "opt"));
}

void make_engine_param_page(GtkWidget *vbox, GtkWidget *dialog)
{
  GtkWidget *w;                           /* general tmp */
  GtkObject *adj1, *adj2, *adj3, *adj4;   /* level parameters */
  VEngineParam ep;

  VGetEngineParam(vhdl, &ep);

  /* unambiguous command */
  w = gtk_label_new("Unambiguous command timeout:");
  gtk_box_pack_start(GTK_BOX(vbox), w, FALSE, FALSE, 0);
  gtk_label_set_justify(GTK_LABEL(w), GTK_JUSTIFY_RIGHT);

  adj1 = gtk_adjustment_new((gfloat)ep.unambiguous_to/1000.0, 0.1, 1.0, 0.01,
      0.10, 0.0);
  gtk_signal_connect(GTK_OBJECT(adj1), "value-changed",
      GTK_SIGNAL_FUNC(engine_set_cb), dialog);
  w = gtk_hscale_new(GTK_ADJUSTMENT(adj1));
  gtk_scale_set_digits(GTK_SCALE(w), 2);
  gtk_box_pack_start (GTK_BOX(vbox), w, FALSE, FALSE, 0);

  /* command */
  w = gtk_label_new("Complete command timeout:");
  gtk_box_pack_start(GTK_BOX(vbox), w, FALSE, FALSE, 0);
  gtk_label_set_justify(GTK_LABEL(w), GTK_JUSTIFY_RIGHT);

  adj2 = gtk_adjustment_new((gfloat)ep.command_to/1000.0, 0.1, 1.0, 0.01, 
      0.10, 0.0);
  gtk_signal_connect(GTK_OBJECT(adj2), "value-changed",
      GTK_SIGNAL_FUNC(engine_set_cb), dialog);
  w = gtk_hscale_new(GTK_ADJUSTMENT(adj2));
  gtk_scale_set_digits(GTK_SCALE(w), 2);
  gtk_box_pack_start (GTK_BOX(vbox), w, FALSE, FALSE, 0);

  /* dictate */
  w = gtk_label_new("Dictation timeout:");
  gtk_box_pack_start(GTK_BOX(vbox), w, FALSE, FALSE, 0);
  gtk_label_set_justify(GTK_LABEL(w), GTK_JUSTIFY_RIGHT);

  adj3 = gtk_adjustment_new((gfloat)ep.text_to/1000.0, 0.1, 1.0, 0.01, 
      0.10, 0.0);
  gtk_signal_connect(GTK_OBJECT(adj3), "value-changed",
      GTK_SIGNAL_FUNC(engine_set_cb), dialog);
  w = gtk_hscale_new(GTK_ADJUSTMENT(adj3));
  gtk_scale_set_digits(GTK_SCALE(w), 2);
  gtk_box_pack_start (GTK_BOX(vbox), w, FALSE, FALSE, 0);

  /* recognition threshold */
  w = gtk_label_new("Recognition threshold:");
  gtk_box_pack_start(GTK_BOX(vbox), w, FALSE, FALSE, 0);
  gtk_label_set_justify(GTK_LABEL(w), GTK_JUSTIFY_RIGHT);

  adj4 = gtk_adjustment_new((gfloat)ep.threshold, (gfloat)ep.threshold_min,
      (gfloat)ep.threshold_max, 1, 2, 0.0);
  gtk_signal_connect(GTK_OBJECT(adj4), "value-changed",
      GTK_SIGNAL_FUNC(engine_set_cb), dialog);
  w = gtk_hscale_new(GTK_ADJUSTMENT(adj4));
  gtk_scale_set_digits(GTK_SCALE(w), 0);
  gtk_box_pack_start (GTK_BOX(vbox), w, FALSE, FALSE, 0);


  /* set up some pointers so we can enforce the a1>=a2>=a3 thing */
  gtk_object_set_data(GTK_OBJECT(adj1), "next", adj2);
  gtk_object_set_data(GTK_OBJECT(adj2), "next", adj3);
  gtk_object_set_data(GTK_OBJECT(adj2), "prev", adj1);
  gtk_object_set_data(GTK_OBJECT(adj3), "prev", adj2);

  /* voice engine performance optimization */
  w = gtk_label_new("Voice engine optimization:");
  gtk_box_pack_start(GTK_BOX(vbox), w, FALSE, FALSE, 0);
  gtk_label_set_justify(GTK_LABEL(w), GTK_JUSTIFY_RIGHT);

  GtkWidget *buttonbox = gtk_hbox_new(TRUE, 0);
  GtkWidget *btns[3];
  gtk_box_pack_start(GTK_BOX(vbox), buttonbox, FALSE, FALSE, 0);
  const char *labels[] = {"Fast", "Balanced", "Accurate"};
  gpointer vals[] = {(gpointer)V_PERF_FAST, (gpointer)V_PERF_BALANCED,
    (gpointer)V_PERF_ACCURATE};

    radio_buttons(vals, labels, 3, (gpointer)ep.performance, dialog, "opt", btns,
        GTK_SIGNAL_FUNC(set_prop_changed));
    gtk_box_pack_start(GTK_BOX(buttonbox), btns[0], FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(buttonbox), btns[1], FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(buttonbox), btns[2], FALSE, FALSE, 0);


    gtk_object_set_data(GTK_OBJECT(dialog), "unambiguous", adj1);
    gtk_object_set_data(GTK_OBJECT(dialog), "command", adj2);
    gtk_object_set_data(GTK_OBJECT(dialog), "dictate", adj3);
    gtk_object_set_data(GTK_OBJECT(dialog), "threshold", adj4);

}

void preferences_cb(GtkWidget* UNUSED(wp), gpointer UNUSED(data))
{
  static GtkWidget *dialog = NULL;
  if (dialog != NULL) {
    gdk_window_show(dialog->window);
    gdk_window_raise(dialog->window);
    return;
  }

  GtkWidget *label;
  GtkWidget *vbox;                        /* the main vbox */

  // Setup for main propertybox
  dialog = gnome_property_box_new();
  gtk_signal_connect(GTK_OBJECT(dialog), "destroy",
      GTK_SIGNAL_FUNC(gtk_widget_destroyed), &dialog);

  // Setup for dictation tab page
  vbox = gtk_vbox_new(FALSE, 0);
  label = gtk_label_new("Dictation");
  make_dictation_pref_page(vbox, dialog);
  gnome_property_box_append_page(GNOME_PROPERTY_BOX(dialog), vbox, label);

  // Setup for engine parameters
  vbox = gtk_vbox_new(FALSE, 0);
  label = gtk_label_new("Engine parameters");
  make_engine_param_page(vbox, dialog);
  gnome_property_box_append_page(GNOME_PROPERTY_BOX(dialog), vbox, label);

  // signal connect for when it is submitted
  gtk_signal_connect(GTK_OBJECT(dialog), "apply",
      GTK_SIGNAL_FUNC(prefs_apply_cb), NULL);

  gtk_widget_show_all(dialog);

}


/*
 * Mouse event builder dialog
 */

static void update_one_page(fstring *str, GtkWidget *page)
{
  EventRecord *er = (EventRecord *)gtk_object_get_data(GTK_OBJECT(page),
      "context");
  int origin = (int)gtk_object_get_data(GTK_OBJECT(page), "origin");
  int button = (int)gtk_object_get_data(GTK_OBJECT(page), "button");
  char *action = (char*)gtk_object_get_data(GTK_OBJECT(page), "action");
  GtkWidget *mtog = (GtkWidget*)gtk_object_get_data(GTK_OBJECT(page), "mtog");
  GtkWidget *atog = (GtkWidget*)gtk_object_get_data(GTK_OBJECT(page), "atog");

  str->appendf("%s", "<mouse ");
  if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(mtog))) {
    switch (origin) {
      case ORG_WIN:
        str->appendf("x='%d' y='%d' origin='%s'", er->x, er->y, "window");
        break;
      case ORG_ROOT:
        str->appendf("x='%d' y='%d' origin='%s'", er->x_root, er->y_root, "root");
        break;
      case ORG_WIDGET:
        str->appendf("origin='widget' wid='%s'", er->widget->c_str());
        break;
    }
  }

  if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(atog))) {
    str->appendf(" button='%d' action='%s' ", button, action);
  }
  str->appendf(" />\n");
  dbgprintf((str->c_str()));
  dbgprintf(("\n"));
}

static void update_page(GtkWidget* UNUSED(w), gpointer page)
{
  GtkWidget *notebook = (GtkWidget*)gtk_object_get_data(GTK_OBJECT(page), 
      "notebook");
  int count = (int)gtk_object_get_data(GTK_OBJECT(notebook), "count");
  int i;
  fstring str;
  GtkWidget *text;


  text = (GtkWidget*)gtk_object_get_data(GTK_OBJECT(notebook), "text");
  for (i = 0; i < count; ++i) {
    update_one_page(&str, 
        gtk_notebook_get_nth_page(GTK_NOTEBOOK(notebook), i+1));
  }
  gtk_text_backward_delete(GTK_TEXT(text), gtk_text_get_length(GTK_TEXT(text)));
  dbgprintf(("inserting %s\n", str.c_str()));
  gtk_text_insert(GTK_TEXT(text), NULL, 
      &text->style->black, NULL, str.c_str(), -1);
}

static void set_sensitive_cb(GtkWidget *w, gpointer win)
{
  if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(w))) {
    gtk_widget_set_sensitive(GTK_WIDGET(win), TRUE);
  } else {
    gtk_widget_set_sensitive(GTK_WIDGET(win), FALSE);
  }
}

static void clean_event_cb(GtkWidget* UNUSED(w), gpointer data)
{
  EventRecord *er = (EventRecord *)data;
  if (er->widget != NULL) delete er->widget;
  delete er;
}

static void get_coord_cb(GtkWidget *dialog, int button, gpointer data)
{
  GtkWidget *notebook = GTK_WIDGET(data);
  GtkWidget *page, *vbox, *w, *mtog, *atog;
  GtkWidget *btns[5];
  fstring str;
  EventRecord *er;
  int i, count;


  count = (int)gtk_object_get_data(GTK_OBJECT(notebook), "count") + 1;
  gtk_object_set_data(GTK_OBJECT(notebook), "count", (gpointer)count);

  if (button == 1) {
    gtk_widget_destroy(dialog);
    return;
  }

  er = appRecordMouse();

  /* the notebook page */
  page = gtk_vbox_new(FALSE, 0);

  /* movement option */
  mtog = gtk_check_button_new_with_label("Move to");
  gtk_box_pack_start(GTK_BOX(page), mtog, TRUE, TRUE, 0);

  w = gtk_frame_new(NULL);
  gtk_box_pack_start(GTK_BOX(page), w, TRUE, TRUE, 0);
  vbox = gtk_vbox_new(FALSE, 0);
  gtk_container_add(GTK_CONTAINER(w), vbox);

  gtk_signal_connect(GTK_OBJECT(mtog), "toggled",
      GTK_SIGNAL_FUNC(set_sensitive_cb), vbox);
  gtk_signal_connect(GTK_OBJECT(mtog), "toggled",
      GTK_SIGNAL_FUNC(update_page), page);

  fstring opt1, opt2, opt3;
  opt1.appendf("Window x: %d y: %d", er->x, er->y);
  opt2.appendf("Root x: %d y: %d", er->x_root, er->y_root);
  opt3.appendf("Widget ");
  if (er->widget != NULL && er->owner)
    opt3.appendf("%s", er->widget->c_str());

  const char *options[] = { opt1.c_str(), opt2.c_str(), opt3.c_str() };
  int vals[] = { ORG_WIN, ORG_ROOT, ORG_WIDGET };
  radio_buttons((gpointer*)vals, options, 3, 
      (gpointer)(er->owner?ORG_WIN:ORG_ROOT),
      page, "origin", btns, GTK_SIGNAL_FUNC(update_page));
  for (i = 0; i < 3; ++i)
    gtk_box_pack_start(GTK_BOX(vbox), btns[i], FALSE, FALSE, 0);

  if (er->window == false) {
    gtk_widget_set_sensitive(btns[0], FALSE);
    gtk_widget_set_sensitive(btns[2], FALSE);
  }
  if (er->widget == NULL || er->owner == false)
    gtk_widget_set_sensitive(btns[2], FALSE);

  /* actions */
  atog = gtk_check_button_new_with_label("Action");
  gtk_box_pack_start(GTK_BOX(page), atog, TRUE, TRUE, 0);

  w = gtk_frame_new(NULL);
  gtk_box_pack_start(GTK_BOX(page), w, TRUE, TRUE, 0);
  vbox = gtk_vbox_new(FALSE, 0);
  gtk_container_add(GTK_CONTAINER(w), vbox);

  gtk_signal_connect(GTK_OBJECT(atog), "toggled", 
      GTK_SIGNAL_FUNC(set_sensitive_cb), vbox);
  gtk_signal_connect(GTK_OBJECT(atog), "toggled", 
      GTK_SIGNAL_FUNC(update_page), page);

  const char *action[] = { 
    "click", "double click", "triple click", "up", "down" 
  };

  radio_buttons((gpointer*)action, action, 5, (gpointer)action[0], page, 
      "action", btns, GTK_SIGNAL_FUNC(update_page));
  GtkWidget *hbox = gtk_hbox_new(FALSE, 0);
  gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);
  gtk_box_pack_start(GTK_BOX(hbox), btns[0], FALSE, FALSE, 0);
  gtk_box_pack_start(GTK_BOX(hbox), btns[1], FALSE, FALSE, 0);
  gtk_box_pack_start(GTK_BOX(hbox), btns[2], FALSE, FALSE, 0);
  hbox = gtk_hbox_new(FALSE, 0);
  gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);
  gtk_box_pack_start(GTK_BOX(hbox), btns[3], FALSE, FALSE, 0);
  gtk_box_pack_start(GTK_BOX(hbox), btns[4], FALSE, FALSE, 0);

  hbox = gtk_hbox_new(FALSE, 0);
  gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);
  w = gtk_label_new("button");
  gtk_box_pack_start(GTK_BOX(hbox), w, FALSE, FALSE, 0);
  const char *buttons[] = { " 1", " 2", " 3", " 4", " 5" };
  int bvals[] = { 1, 2, 3, 4, 5 };
  radio_buttons((gpointer*)bvals, buttons, 5, (gpointer)er->button, page,
      "button", btns, GTK_SIGNAL_FUNC(update_page));
  for (i = 0; i < 5; ++i)
    gtk_box_pack_start(GTK_BOX(hbox), btns[i], FALSE, FALSE, 0);

  gtk_object_set_data(GTK_OBJECT(page), "atog", (gpointer)atog);
  gtk_object_set_data(GTK_OBJECT(page), "mtog", (gpointer)mtog);
  gtk_object_set_data(GTK_OBJECT(page), "notebook", (gpointer)notebook);
  gtk_object_set_data(GTK_OBJECT(page), "context", (gpointer)er);
  gtk_signal_connect(GTK_OBJECT(page), "destroy",
      GTK_SIGNAL_FUNC(clean_event_cb), er);
  str.resize(0);
  str.appendf("%d", count);
  w = gtk_label_new(str.c_str());
  gtk_notebook_append_page(GTK_NOTEBOOK(notebook), page, w);
  gtk_widget_show_all(page);
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(mtog), TRUE);
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(atog), TRUE);
}

static void mouse_events_cb()
{
  static GtkWidget *dialog = NULL;
  GtkWidget *notebook;
  GtkWidget *text, *w;

  if (dialog != NULL) {
    gdk_window_show(dialog->window);
    gdk_window_raise(dialog->window);
    return;
  }

  dialog = gnome_dialog_new(_("Build mouse events"),
      _("Capture"),
      GNOME_STOCK_BUTTON_CLOSE,
      NULL);
  notebook = gtk_notebook_new();
  text = gtk_text_new(FALSE, FALSE);
  w = gtk_label_new("XML");
  gtk_notebook_append_page(GTK_NOTEBOOK(notebook), text, w);
  gtk_text_set_editable (GTK_TEXT(text), FALSE);
  gtk_object_set_data(GTK_OBJECT(notebook), "text", (gpointer)text);
  gtk_object_set_data(GTK_OBJECT(notebook), "count", (gpointer)0);
  gtk_box_pack_start(GTK_BOX(GNOME_DIALOG(dialog)->vbox), notebook, 
      TRUE, TRUE, 0);
  gtk_signal_connect(GTK_OBJECT(dialog), "clicked",
      GTK_SIGNAL_FUNC(get_coord_cb), (gpointer)notebook);
  gtk_signal_connect(GTK_OBJECT(dialog), "destroy",
      GTK_SIGNAL_FUNC(gtk_widget_destroyed), &dialog);
  gtk_widget_show_all(dialog);
}

/*
 * about dialog
 */

void showabout( GtkWidget* UNUSED(w), gpointer UNUSED(d) )
{
  static GtkWidget *dialog = NULL;
  if (dialog != NULL) {
    gdk_window_show(dialog->window);
    gdk_window_raise(dialog->window);
    return;
  }

  const gchar *authors[] = {
    "David Z. Creemer",
    "Tom Doris",
    "Brian Craft",
    "Deborah Kaplan",
    "Jessica Hekman",
    NULL
  };
  dialog = gnome_about_new(_(PACKAGE), VERSION,
      "(c) 1999, 2000, 2001", authors,
      _("Voice enabled X"), NULL);

  gtk_signal_connect(GTK_OBJECT(dialog), "destroy",
      GTK_SIGNAL_FUNC(gtk_widget_destroyed), &dialog);

  gtk_widget_show(dialog);
}

static GnomeUIInfo file_menu[] = {
  GNOMEUIINFO_MENU_EXIT_ITEM(CloseUpShop, NULL),
  GNOMEUIINFO_END
};

static GnomeUIInfo settings_menu[] = {
  {
    GNOME_APP_UI_ITEM,
    const_cast<char*>(N_("_Preferences")),
    const_cast<char*>(N_("Set user preferences")),
    (void *)preferences_cb, NULL, NULL,
    GNOME_APP_PIXMAP_NONE, NULL,
    'p', GDK_CONTROL_MASK, NULL
  },
  GNOMEUIINFO_END
};

static GnomeUIInfo grammar_menu[] = {
  {
    GNOME_APP_UI_ITEM,
    const_cast<char*>(N_("_Rebuild")),
    const_cast<char*>(N_("Rebuild grammar files")),
    (void *)rebuild_cb, NULL, NULL,
    GNOME_APP_PIXMAP_NONE, NULL,
    'r', GDK_CONTROL_MASK, NULL
  },
  {
    GNOME_APP_UI_ITEM,
    const_cast<char*>(N_("_Mouse events")),
    const_cast<char*>(N_("Construct mouse events")),
    (void *)mouse_events_cb, NULL, NULL,
    GNOME_APP_PIXMAP_NONE, NULL,
    'm', GDK_CONTROL_MASK, NULL
  },
  GNOMEUIINFO_END
};

static GnomeUIInfo help_menu[] = {
  {
    GNOME_APP_UI_ITEM,
    const_cast<char*>(N_("_About")),
    const_cast<char*>(N_("Get information about " PACKAGE)),
    (void *)showabout, NULL, NULL,
    GNOME_APP_PIXMAP_NONE, NULL,
    'a', GDK_CONTROL_MASK, NULL
  },
  /*
   * can't get this to work. it installs a Alt_L accellerator,
   * which is bogus.
   * GNOMEUIINFO_MENU_ABOUT_ITEM((void *)showabout, NULL),
   */
  GNOMEUIINFO_END
};

// Because libgnomeui is not const correct, we need to add
// a few dirty casts here.  Redefine a few macros here in
// order to be able to have that cast:
#define CC_GNOMEUIINFO_MENU_FILE_TREE(tree) \
        { GNOME_APP_UI_SUBTREE_STOCK, const_cast<char*>(N_("_File")), NULL, tree, NULL, NULL, \
                (GnomeUIPixmapType) 0, NULL, 0, (GdkModifierType) 0, NULL }
#define CC_GNOMEUIINFO_MENU_EDIT_TREE(tree) \
        { GNOME_APP_UI_SUBTREE_STOCK, const_cast<char*>(N_("_Edit")), NULL, tree, NULL, NULL, \
                (GnomeUIPixmapType) 0, NULL, 0, (GdkModifierType) 0, NULL }
#define CC_GNOMEUIINFO_MENU_VIEW_TREE(tree) \
        { GNOME_APP_UI_SUBTREE_STOCK, const_cast<char*>(N_("_View")), NULL, tree, NULL, NULL, \
                (GnomeUIPixmapType) 0, NULL, 0, (GdkModifierType) 0, NULL }
#define CC_GNOMEUIINFO_MENU_SETTINGS_TREE(tree) \
        { GNOME_APP_UI_SUBTREE_STOCK, const_cast<char*>(N_("_Settings")), NULL, tree, NULL, NULL, \
                (GnomeUIPixmapType) 0, NULL, 0, (GdkModifierType) 0, NULL }
#define CC_GNOMEUIINFO_MENU_FILES_TREE(tree) \
        { GNOME_APP_UI_SUBTREE_STOCK, const_cast<char*>(N_("Fi_les")), NULL, tree, NULL, NULL, \
                (GnomeUIPixmapType) 0, NULL, 0, (GdkModifierType) 0, NULL }
#define CC_GNOMEUIINFO_MENU_HELP_TREE(tree) \
        { GNOME_APP_UI_SUBTREE_STOCK, const_cast<char*>(N_("_Help")), NULL, tree, NULL, NULL, \
                (GnomeUIPixmapType) 0, NULL, 0, (GdkModifierType) 0, NULL }

static GnomeUIInfo main_menu[] = {
  CC_GNOMEUIINFO_MENU_FILE_TREE(file_menu),
  GNOMEUIINFO_SUBTREE(const_cast<char*>(N_("_Grammars")), grammar_menu),
  CC_GNOMEUIINFO_MENU_SETTINGS_TREE(settings_menu),
  CC_GNOMEUIINFO_MENU_HELP_TREE(help_menu),
  GNOMEUIINFO_END
};

void gnomeMainWindow::set_status(const char *status_str)
{
    gnome_appbar_set_status(GNOME_APPBAR(_statusBar), status_str);
}

void gnomeMainWindow::createMainWindow()
{
  GtkWidget *mainWindow;
  GtkWidget *vbox3;
  GtkWidget *scrolledwindow1;
  GtkWidget *label;
  GtkWidget *hbox5;
  GtkWidget *hpaned;
  GtkWidget *vpaned;

  mainWindow = gnome_app_new(PACKAGE, PACKAGE);
  gtk_window_set_default_size(GTK_WINDOW(mainWindow), 150, 150);
  gtk_window_set_policy (GTK_WINDOW(mainWindow), FALSE, TRUE, FALSE);

  vbox3 = gtk_vbox_new (FALSE, 0);
  gnome_app_set_contents (GNOME_APP(mainWindow), vbox3);
  gtk_widget_show (vbox3);

  gnome_app_create_menus(GNOME_APP(mainWindow), main_menu);

  /* Put scrolledwindow1 and hpaned in a VPaned so there's
     a resize bar between 'em */

  vpaned = gtk_vpaned_new();
  gtk_widget_show(vpaned);
  gtk_box_pack_start (GTK_BOX (vbox3), vpaned, TRUE, TRUE, 0);

  scrolledwindow1 = gtk_scrolled_window_new (NULL, NULL);
  gtk_object_set_data (GTK_OBJECT (mainWindow), "scrolledwindow1", scrolledwindow1);
  gtk_widget_show (scrolledwindow1);
  gtk_paned_pack1 (GTK_PANED (vpaned), scrolledwindow1, TRUE, TRUE);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolledwindow1), GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);

  _targetWindow = gtk_clist_new (3);
  gtk_widget_set_usize(_targetWindow, 1, 1);
  gtk_widget_show (_targetWindow);
  gtk_container_add (GTK_CONTAINER (scrolledwindow1), _targetWindow);
  gtk_container_border_width (GTK_CONTAINER (_targetWindow), 1);
  gtk_clist_set_column_width (GTK_CLIST (_targetWindow), 0, 23);
  gtk_clist_set_column_width (GTK_CLIST (_targetWindow), 1, 73);
  gtk_clist_set_column_width (GTK_CLIST (_targetWindow), 2, 80);
  gtk_clist_column_titles_show (GTK_CLIST (_targetWindow));

  label = gtk_label_new("#");
  gtk_widget_show (label);
  gtk_clist_set_column_widget (GTK_CLIST (_targetWindow), 0, label);

  label = gtk_label_new("Status");
  gtk_widget_show (label);
  gtk_clist_set_column_widget(GTK_CLIST (_targetWindow), 1, label);

  label = gtk_label_new("Application");
  gtk_widget_show(label);
  gtk_clist_set_column_widget(GTK_CLIST (_targetWindow), 2, label);

  hpaned = gtk_hpaned_new();
  gtk_paned_set_position(GTK_PANED(hpaned), 90);
  gtk_widget_show(hpaned);
  gtk_paned_pack2 (GTK_PANED(vpaned), hpaned, TRUE, TRUE);

  /* Create the vocab list */
  _vocabWindow = gtk_clist_new (1);
  gtk_widget_set_usize(_vocabWindow, 1, 1);
  GTK_CLIST_SET_FLAG(_vocabWindow, CLIST_SHOW_TITLES);
  gtk_widget_show (_vocabWindow);
  label = gtk_label_new("Vocabularies");
  gtk_paned_pack1 (GTK_PANED (hpaned), _vocabWindow, TRUE, TRUE);
  gtk_widget_show (label);
  gtk_clist_set_column_widget (GTK_CLIST (_vocabWindow), 0, label);

  /* Create the GtkText widget */
  _textWindow = gtk_text_new (NULL, NULL);
  gtk_widget_set_usize(_textWindow, 1, 1);
  gtk_text_set_editable (GTK_TEXT (_textWindow), FALSE);
  gtk_text_set_word_wrap(GTK_TEXT(_textWindow), TRUE);
  gtk_paned_pack2 (GTK_PANED (hpaned), _textWindow, TRUE, TRUE);
  gtk_widget_show (_textWindow);
  gtk_widget_set_usize(_textWindow, -1, 50);

  /* 
   * Realizing a widget creates a window for it,
   * ready for us to insert some text 
   */
  gtk_widget_realize (_textWindow);


  hbox5 = gtk_hbox_new(FALSE, 0);
  gtk_widget_show (hbox5);
  gtk_box_pack_start (GTK_BOX (vbox3), hbox5, FALSE, FALSE, 0);

  label = gtk_label_new("     Microphone:   ");
  gtk_widget_show(label);
  gtk_box_pack_start(GTK_BOX (hbox5), label, FALSE, FALSE, 3);
  gtk_label_set_justify(GTK_LABEL (label), GTK_JUSTIFY_RIGHT);

  _micButton = gtk_toggle_button_new_with_label ("Push To Talk");
  gtk_widget_show (_micButton);
  gtk_box_pack_start (GTK_BOX (hbox5), _micButton, FALSE, FALSE, 0);

  _mic_signal = gtk_signal_connect ( GTK_OBJECT ( _micButton ), "toggled",
      GTK_SIGNAL_FUNC ( ToggleMic ), NULL );

  _statusBar = gnome_appbar_new (FALSE, TRUE, GNOME_PREFERENCES_USER);
  gnome_app_set_statusbar (GNOME_APP(mainWindow), _statusBar);
  gtk_widget_show (_statusBar);
  gnome_app_install_menu_hints(GNOME_APP(mainWindow), main_menu);

  char const* initial_clist[] = { "0", "Idle", "-" };
  gtk_clist_append( GTK_CLIST( _targetWindow ), const_cast<char**>(initial_clist) );

  gtk_signal_connect ( GTK_OBJECT ( mainWindow ), "destroy",
      GTK_SIGNAL_FUNC ( CloseUpShop ), NULL );
  _mainwin = mainWindow;
  gtk_widget_show( _mainwin );
}

MainWindow *getMainWindow(int argc, char **argv,
                          const struct poptOption* opt)
{
  gnome_init_with_popt_table(PACKAGE, VERSION, argc, argv, opt, 0, NULL);
  display = GDK_DISPLAY();
  gnomeMainWindow *mw = new gnomeMainWindow();
  mw->createMainWindow();

  XWindowAttributes attr;
  XGetWindowAttributes(GDK_DISPLAY(), GDK_ROOT_WINDOW(), &attr);
  /* 
   * request that the X server report the events associated with the
   * specified event mask
   */
  XSelectInput(GDK_DISPLAY(), GDK_ROOT_WINDOW(),
      attr.your_event_mask | PropertyChangeMask);
  return mw;
}

static int turnOnMic(gpointer)
{
  VMicOn(vhdl, 1);
  return 0;
}

static void mic(int state, void *data)
{
  gnomeMainWindow *gmw = (gnomeMainWindow*)data;
  gmw->mic(state);
}

static void state(int s, void *data)
{
  gnomeMainWindow *gmw = (gnomeMainWindow*)data;
  gmw->state(s);
}

static void reco(const char *text, int firm, void *data)
{
  gnomeMainWindow *gmw = (gnomeMainWindow*)data;
  gmw->reco(text, firm);
}

static void error(int err, const char *msg, void *data)
{
  gnomeMainWindow *gmw = (gnomeMainWindow*)data;
  gmw->error(err, msg);
}

static void vocab(const char *name, int active, void *data)
{
  gnomeMainWindow *gmw = (gnomeMainWindow*)data;
  gmw->vocab(name, active);
}

static VClient gmwClient = {
  "gmw",
  mic,
  state,
  reco,
  error,
  vocab,
  notify,
  initialState,
};

static gint runInit(gpointer p)
{
  bool save = (bool)p;
  xvoice_init(&gmwClient, save);
  return 0;
}

void gnomeMainWindow::main(bool save, bool mic_on)
{
  gtk_idle_add(runInit, (void*)save);
  if (mic_on) {
    gtk_timeout_add(500, turnOnMic, this);
  }
  gtk_main();
}
