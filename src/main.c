#include <gtk/gtk.h>
#include <stdio.h>

typedef struct {
  GtkWidget *window;
  GtkWidget *text_view;
  GtkWidget *find_revealer;
  GtkWidget *replace_revealer;
  GtkWidget *find_entry;
  GtkWidget *replace_entry;
  GtkWidget *status_label;
  char *current_file;
  gboolean modified;
} AppState;

static void update_title(AppState *state) {
  char *base = state->current_file ? g_path_get_basename(state->current_file)
                                   : g_strdup("Untitled");
  char title[512];
  snprintf(title, sizeof(title), "%s%s - Notepad", state->modified ? "*" : "",
           base);
  gtk_window_set_title(GTK_WINDOW(state->window), title);
  g_free(base);
}

static void update_status(AppState *state) {
  GtkTextBuffer *buf =
      gtk_text_view_get_buffer(GTK_TEXT_VIEW(state->text_view));

  GtkTextIter cursor;
  gtk_text_buffer_get_iter_at_mark(buf, &cursor,
                                   gtk_text_buffer_get_insert(buf));
  int line = gtk_text_iter_get_line(&cursor) + 1;
  int col = gtk_text_iter_get_line_offset(&cursor) + 1;
  int n_lines = gtk_text_buffer_get_line_count(buf);

  GtkTextIter start, end;
  gtk_text_buffer_get_bounds(buf, &start, &end);
  char *text = gtk_text_buffer_get_text(buf, &start, &end, FALSE);

  int n_chars = (int)g_utf8_strlen(text, -1);
  int n_words = 0;
  gboolean in_word = FALSE;
  for (const char *p = text; *p; p = g_utf8_next_char(p)) {
    if (g_unichar_isspace(g_utf8_get_char(p))) {
      in_word = FALSE;
    } else if (!in_word) {
      n_words++;
      in_word = TRUE;
    }
  }
  g_free(text);

  char *base = state->current_file ? g_path_get_basename(state->current_file)
                                   : g_strdup("Untitled");
  char status[512];
  snprintf(status, sizeof(status),
           " %s  |  Ln %d, Col %d  |  Lines: %d  |  Words: %d  |  Chars: %d",
           base, line, col, n_lines, n_words, n_chars);
  g_free(base);
  gtk_label_set_text(GTK_LABEL(state->status_label), status);
}

static void on_text_changed(GtkTextBuffer *buf, gpointer user_data) {
  (void)buf;
  AppState *state = user_data;
  if (!state->modified) {
    state->modified = TRUE;
    update_title(state);
  }
  update_status(state);
}

static void on_cursor_moved(GtkTextBuffer *buf, GParamSpec *ps,
                            gpointer user_data) {
  (void)buf;
  (void)ps;
  update_status(user_data);
}

static void show_error(AppState *state, const char *msg) {
  GtkAlertDialog *d = gtk_alert_dialog_new("%s", msg);
  gtk_alert_dialog_show(d, GTK_WINDOW(state->window));
  g_object_unref(d);
}

static void load_file(AppState *state, const char *path) {
  GError *err = NULL;
  char *contents = NULL;
  gsize length;
  if (!g_file_get_contents(path, &contents, &length, &err)) {
    char msg[512];
    snprintf(msg, sizeof(msg), "Could not open: %s", err->message);
    show_error(state, msg);
    g_error_free(err);
    return;
  }
  GtkTextBuffer *buf =
      gtk_text_view_get_buffer(GTK_TEXT_VIEW(state->text_view));
  gtk_text_buffer_set_text(buf, contents, (int)length);
  g_free(contents);
  g_free(state->current_file);
  state->current_file = g_strdup(path);
  state->modified = FALSE;
  update_title(state);
  update_status(state);
}

static void do_save(AppState *state, const char *path) {
  GtkTextBuffer *buf =
      gtk_text_view_get_buffer(GTK_TEXT_VIEW(state->text_view));
  GtkTextIter start, end;
  gtk_text_buffer_get_bounds(buf, &start, &end);
  char *text = gtk_text_buffer_get_text(buf, &start, &end, FALSE);
  GError *err = NULL;
  if (!g_file_set_contents(path, text, -1, &err)) {
    char msg[512];
    snprintf(msg, sizeof(msg), "Could not save: %s", err->message);
    show_error(state, msg);
    g_error_free(err);
  } else {
    g_free(state->current_file);
    state->current_file = g_strdup(path);
    state->modified = FALSE;
    update_title(state);
    update_status(state);
  }
  g_free(text);
}

static void on_save_response(GtkFileDialog *dialog, GAsyncResult *result,
                             gpointer user_data) {
  AppState *state = user_data;
  GFile *file = gtk_file_dialog_save_finish(dialog, result, NULL);
  if (file) {
    char *path = g_file_get_path(file);
    do_save(state, path);
    g_free(path);
    g_object_unref(file);
  }
}

static void on_open_response(GtkFileDialog *dialog, GAsyncResult *result,
                             gpointer user_data) {
  AppState *state = user_data;
  GFile *file = gtk_file_dialog_open_finish(dialog, result, NULL);
  if (!file)
    return;
  char *path = g_file_get_path(file);
  g_object_unref(file);
  load_file(state, path);
  g_free(path);
}

static void action_new(GSimpleAction *a, GVariant *p, gpointer user_data) {
  (void)a;
  (void)p;
  AppState *state = user_data;
  GtkTextBuffer *buf =
      gtk_text_view_get_buffer(GTK_TEXT_VIEW(state->text_view));
  gtk_text_buffer_set_text(buf, "", 0);
  g_free(state->current_file);
  state->current_file = NULL;
  state->modified = FALSE;
  update_title(state);
  update_status(state);
}

static void action_open(GSimpleAction *a, GVariant *p, gpointer user_data) {
  (void)a;
  (void)p;
  AppState *state = user_data;
  GtkFileDialog *dialog = gtk_file_dialog_new();
  gtk_file_dialog_open(dialog, GTK_WINDOW(state->window), NULL,
                       (GAsyncReadyCallback)on_open_response, state);
  g_object_unref(dialog);
}

static void action_save(GSimpleAction *a, GVariant *p, gpointer user_data) {
  (void)a;
  (void)p;
  AppState *state = user_data;
  if (state->current_file)
    do_save(state, state->current_file);
  else {
    GtkFileDialog *dialog = gtk_file_dialog_new();
    gtk_file_dialog_save(dialog, GTK_WINDOW(state->window), NULL,
                         (GAsyncReadyCallback)on_save_response, state);
    g_object_unref(dialog);
  }
}

static void action_save_as(GSimpleAction *a, GVariant *p, gpointer user_data) {
  (void)a;
  (void)p;
  AppState *state = user_data;
  GtkFileDialog *dialog = gtk_file_dialog_new();
  gtk_file_dialog_save(dialog, GTK_WINDOW(state->window), NULL,
                       (GAsyncReadyCallback)on_save_response, state);
  g_object_unref(dialog);
}

static void action_quit(GSimpleAction *a, GVariant *p, gpointer user_data) {
  (void)a;
  (void)p;
  gtk_window_destroy(GTK_WINDOW(((AppState *)user_data)->window));
}

static void close_find_bar(AppState *state) {
  gtk_revealer_set_reveal_child(GTK_REVEALER(state->find_revealer), FALSE);
  gtk_widget_grab_focus(state->text_view);
}

static void find_in_direction(AppState *state, gboolean forward) {
  const char *term = gtk_editable_get_text(GTK_EDITABLE(state->find_entry));
  if (!*term)
    return;

  GtkTextBuffer *buf =
      gtk_text_view_get_buffer(GTK_TEXT_VIEW(state->text_view));
  GtkTextIter ins, bound, match_start, match_end;
  gtk_text_buffer_get_iter_at_mark(buf, &ins, gtk_text_buffer_get_insert(buf));
  gtk_text_buffer_get_iter_at_mark(buf, &bound,
                                   gtk_text_buffer_get_selection_bound(buf));

  GtkTextSearchFlags flags = GTK_TEXT_SEARCH_CASE_INSENSITIVE;

  if (forward) {
    GtkTextIter from = gtk_text_iter_compare(&ins, &bound) > 0 ? ins : bound;
    if (!gtk_text_iter_forward_search(&from, term, flags, &match_start,
                                      &match_end, NULL)) {
      gtk_text_buffer_get_start_iter(buf, &from);
      if (!gtk_text_iter_forward_search(&from, term, flags, &match_start,
                                        &match_end, NULL))
        return;
    }
  } else {
    GtkTextIter from = gtk_text_iter_compare(&ins, &bound) < 0 ? ins : bound;
    if (!gtk_text_iter_backward_search(&from, term, flags, &match_start,
                                       &match_end, NULL)) {
      gtk_text_buffer_get_end_iter(buf, &from);
      if (!gtk_text_iter_backward_search(&from, term, flags, &match_start,
                                         &match_end, NULL))
        return;
    }
  }

  gtk_text_buffer_select_range(buf, &match_start, &match_end);
  gtk_text_view_scroll_to_iter(GTK_TEXT_VIEW(state->text_view), &match_start,
                               0.1, FALSE, 0, 0.5);
}

static void find_next(AppState *state) { find_in_direction(state, TRUE); }
static void find_prev(AppState *state) { find_in_direction(state, FALSE); }

static void do_replace(AppState *state) {
  const char *term = gtk_editable_get_text(GTK_EDITABLE(state->find_entry));
  const char *repl = gtk_editable_get_text(GTK_EDITABLE(state->replace_entry));
  if (!*term)
    return;

  GtkTextBuffer *buf =
      gtk_text_view_get_buffer(GTK_TEXT_VIEW(state->text_view));
  GtkTextIter s, e;
  if (gtk_text_buffer_get_selection_bounds(buf, &s, &e)) {
    char *sel = gtk_text_buffer_get_text(buf, &s, &e, FALSE);
    if (g_ascii_strcasecmp(sel, term) == 0) {
      gtk_text_buffer_begin_user_action(buf);
      gtk_text_buffer_delete(buf, &s, &e);
      gtk_text_buffer_insert(buf, &s, repl, -1);
      gtk_text_buffer_end_user_action(buf);
    }
    g_free(sel);
  }
  find_next(state);
}

static void do_replace_all(AppState *state) {
  const char *term = gtk_editable_get_text(GTK_EDITABLE(state->find_entry));
  const char *repl = gtk_editable_get_text(GTK_EDITABLE(state->replace_entry));
  if (!*term)
    return;

  GtkTextBuffer *buf =
      gtk_text_view_get_buffer(GTK_TEXT_VIEW(state->text_view));
  GtkTextIter pos, ms, me;
  gtk_text_buffer_get_start_iter(buf, &pos);
  GtkTextSearchFlags flags = GTK_TEXT_SEARCH_CASE_INSENSITIVE;

  gtk_text_buffer_begin_user_action(buf);
  while (gtk_text_iter_forward_search(&pos, term, flags, &ms, &me, NULL)) {
    gtk_text_buffer_delete(buf, &ms, &me);
    gtk_text_buffer_insert(buf, &ms, repl, -1);
    pos = ms;
    gtk_text_iter_forward_chars(&pos, (int)g_utf8_strlen(repl, -1));
  }
  gtk_text_buffer_end_user_action(buf);
}

static gboolean on_find_key(GtkEventControllerKey *c, guint keyval,
                            guint keycode, GdkModifierType mods,
                            gpointer user_data) {
  (void)c;
  (void)keycode;
  (void)mods;
  AppState *state = user_data;
  if (keyval == GDK_KEY_Escape) {
    close_find_bar(state);
    return TRUE;
  }
  if (keyval == GDK_KEY_Return || keyval == GDK_KEY_KP_Enter) {
    find_next(state);
    return TRUE;
  }
  return FALSE;
}

static void action_find(GSimpleAction *a, GVariant *p, gpointer user_data) {
  (void)a;
  (void)p;
  AppState *state = user_data;
  gtk_revealer_set_reveal_child(GTK_REVEALER(state->find_revealer), TRUE);
  gtk_revealer_set_reveal_child(GTK_REVEALER(state->replace_revealer), FALSE);
  gtk_widget_grab_focus(state->find_entry);
}

static void action_find_replace(GSimpleAction *a, GVariant *p,
                                gpointer user_data) {
  (void)a;
  (void)p;
  AppState *state = user_data;
  gtk_revealer_set_reveal_child(GTK_REVEALER(state->find_revealer), TRUE);
  gtk_revealer_set_reveal_child(GTK_REVEALER(state->replace_revealer), TRUE);
  gtk_widget_grab_focus(state->find_entry);
}

static GtkWidget *make_find_bar(AppState *state) {
  GtkWidget *outer = gtk_revealer_new();
  gtk_revealer_set_transition_type(GTK_REVEALER(outer),
                                   GTK_REVEALER_TRANSITION_TYPE_SLIDE_UP);
  state->find_revealer = outer;

  GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
  gtk_revealer_set_child(GTK_REVEALER(outer), vbox);

  /* Find row */
  GtkWidget *find_row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
  gtk_widget_set_margin_start(find_row, 6);
  gtk_widget_set_margin_end(find_row, 6);
  gtk_widget_set_margin_top(find_row, 4);
  gtk_widget_set_margin_bottom(find_row, 2);

  state->find_entry = gtk_entry_new();
  gtk_entry_set_placeholder_text(GTK_ENTRY(state->find_entry), "Find…");
  gtk_widget_set_hexpand(state->find_entry, TRUE);
  GtkWidget *btn_prev = gtk_button_new_with_label("◀");
  GtkWidget *btn_next = gtk_button_new_with_label("▶");
  GtkWidget *btn_close = gtk_button_new_with_label("✕");

  gtk_box_append(GTK_BOX(find_row), gtk_label_new("Find:"));
  gtk_box_append(GTK_BOX(find_row), state->find_entry);
  gtk_box_append(GTK_BOX(find_row), btn_prev);
  gtk_box_append(GTK_BOX(find_row), btn_next);
  gtk_box_append(GTK_BOX(find_row), btn_close);

  /* Replace row (inside its own revealer) */
  GtkWidget *repl_rev = gtk_revealer_new();
  gtk_revealer_set_transition_type(GTK_REVEALER(repl_rev),
                                   GTK_REVEALER_TRANSITION_TYPE_SLIDE_UP);
  state->replace_revealer = repl_rev;

  GtkWidget *repl_row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
  gtk_widget_set_margin_start(repl_row, 6);
  gtk_widget_set_margin_end(repl_row, 6);
  gtk_widget_set_margin_top(repl_row, 2);
  gtk_widget_set_margin_bottom(repl_row, 4);
  gtk_revealer_set_child(GTK_REVEALER(repl_rev), repl_row);

  state->replace_entry = gtk_entry_new();
  gtk_entry_set_placeholder_text(GTK_ENTRY(state->replace_entry),
                                 "Replace with…");
  gtk_widget_set_hexpand(state->replace_entry, TRUE);
  GtkWidget *btn_repl = gtk_button_new_with_label("Replace");
  GtkWidget *btn_repl_all = gtk_button_new_with_label("Replace All");

  gtk_box_append(GTK_BOX(repl_row), gtk_label_new("Replace:"));
  gtk_box_append(GTK_BOX(repl_row), state->replace_entry);
  gtk_box_append(GTK_BOX(repl_row), btn_repl);
  gtk_box_append(GTK_BOX(repl_row), btn_repl_all);

  gtk_box_append(GTK_BOX(vbox), find_row);
  gtk_box_append(GTK_BOX(vbox), repl_rev);

  /* Wire up buttons */
  g_signal_connect_swapped(btn_prev, "clicked", G_CALLBACK(find_prev), state);
  g_signal_connect_swapped(btn_next, "clicked", G_CALLBACK(find_next), state);
  g_signal_connect_swapped(btn_close, "clicked", G_CALLBACK(close_find_bar),
                           state);
  g_signal_connect_swapped(btn_repl, "clicked", G_CALLBACK(do_replace), state);
  g_signal_connect_swapped(btn_repl_all, "clicked", G_CALLBACK(do_replace_all),
                           state);

  /* Key controllers for Escape / Enter */
  for (int i = 0; i < 2; i++) {
    GtkWidget *w = i == 0 ? state->find_entry : state->replace_entry;
    GtkEventControllerKey *kc =
        GTK_EVENT_CONTROLLER_KEY(gtk_event_controller_key_new());
    g_signal_connect(kc, "key-pressed", G_CALLBACK(on_find_key), state);
    gtk_widget_add_controller(w, GTK_EVENT_CONTROLLER(kc));
  }

  return outer;
}

static void activate(GtkApplication *app, gpointer user_data);

static void open_files(GtkApplication *app, GFile **files, gint n_files,
                       const gchar *hint, gpointer user_data) {
  (void)hint;
  (void)user_data;
  activate(app, NULL);
  if (n_files < 1)
    return;
  GtkWindow *win = gtk_application_get_active_window(GTK_APPLICATION(app));
  AppState *state = g_object_get_data(G_OBJECT(win), "app-state");
  char *path = g_file_get_path(files[0]);
  load_file(state, path);
  g_free(path);
}

static void activate(GtkApplication *app, gpointer user_data) {
  (void)user_data;
  if (gtk_application_get_active_window(GTK_APPLICATION(app)))
    return;

  AppState *state = g_new0(AppState, 1);
  state->window = gtk_application_window_new(app);
  gtk_window_set_default_size(GTK_WINDOW(state->window), 800, 600);

  /* Text view */
  GtkWidget *scrolled = gtk_scrolled_window_new();
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled),
                                 GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
  state->text_view = gtk_text_view_new();
  gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(state->text_view),
                              GTK_WRAP_WORD_CHAR);
  gtk_text_view_set_left_margin(GTK_TEXT_VIEW(state->text_view), 4);
  gtk_text_view_set_right_margin(GTK_TEXT_VIEW(state->text_view), 4);
  gtk_text_view_set_top_margin(GTK_TEXT_VIEW(state->text_view), 4);
  gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scrolled),
                                state->text_view);

  GtkTextBuffer *buf =
      gtk_text_view_get_buffer(GTK_TEXT_VIEW(state->text_view));
  g_signal_connect(buf, "changed", G_CALLBACK(on_text_changed), state);
  g_signal_connect(buf, "notify::cursor-position", G_CALLBACK(on_cursor_moved),
                   state);

  /* Status bar */
  state->status_label = gtk_label_new("");
  gtk_label_set_xalign(GTK_LABEL(state->status_label), 0.0);
  gtk_widget_set_margin_top(state->status_label, 2);
  gtk_widget_set_margin_bottom(state->status_label, 2);

  /* Find/replace bar */
  GtkWidget *find_bar = make_find_bar(state);

  /* Actions */
  static const GActionEntry entries[] = {
      {"new", action_new, NULL, NULL, NULL},
      {"open", action_open, NULL, NULL, NULL},
      {"save", action_save, NULL, NULL, NULL},
      {"save-as", action_save_as, NULL, NULL, NULL},
      {"quit", action_quit, NULL, NULL, NULL},
      {"find", action_find, NULL, NULL, NULL},
      {"find-replace", action_find_replace, NULL, NULL, NULL},
  };
  g_action_map_add_action_entries(G_ACTION_MAP(state->window), entries,
                                  G_N_ELEMENTS(entries), state);

  const char *new_accels[] = {"<Control>n", NULL};
  const char *open_accels[] = {"<Control>o", NULL};
  const char *save_accels[] = {"<Control>s", NULL};
  const char *saveas_accels[] = {"<Control><Shift>s", NULL};
  const char *quit_accels[] = {"<Control>q", NULL};
  const char *find_accels[] = {"<Control>f", NULL};
  const char *findrepl_accels[] = {"<Control>h", NULL};
  gtk_application_set_accels_for_action(app, "win.new", new_accels);
  gtk_application_set_accels_for_action(app, "win.open", open_accels);
  gtk_application_set_accels_for_action(app, "win.save", save_accels);
  gtk_application_set_accels_for_action(app, "win.save-as", saveas_accels);
  gtk_application_set_accels_for_action(app, "win.quit", quit_accels);
  gtk_application_set_accels_for_action(app, "win.find", find_accels);
  gtk_application_set_accels_for_action(app, "win.find-replace",
                                        findrepl_accels);

  /* Menu */
  GMenu *menu = g_menu_new();

  GMenu *file_menu = g_menu_new();
  g_menu_append(file_menu, "New", "win.new");
  g_menu_append(file_menu, "Open…", "win.open");
  g_menu_append(file_menu, "Save", "win.save");
  g_menu_append(file_menu, "Save As…", "win.save-as");
  g_menu_append(file_menu, "Quit", "win.quit");
  g_menu_append_submenu(menu, "File", G_MENU_MODEL(file_menu));

  GMenu *edit_menu = g_menu_new();
  g_menu_append(edit_menu, "Find…", "win.find");
  g_menu_append(edit_menu, "Find & Replace…", "win.find-replace");
  g_menu_append_submenu(menu, "Edit", G_MENU_MODEL(edit_menu));

  GtkWidget *menu_bar = gtk_popover_menu_bar_new_from_model(G_MENU_MODEL(menu));

  /* Layout */
  GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
  gtk_box_append(GTK_BOX(box), menu_bar);
  gtk_box_append(GTK_BOX(box), scrolled);
  gtk_widget_set_vexpand(scrolled, TRUE);
  gtk_box_append(GTK_BOX(box), find_bar);
  gtk_box_append(GTK_BOX(box), state->status_label);
  gtk_window_set_child(GTK_WINDOW(state->window), box);

  g_object_set_data(G_OBJECT(state->window), "app-state", state);
  update_title(state);
  update_status(state);
  gtk_widget_set_visible(state->window, TRUE);
  gtk_widget_grab_focus(state->text_view);
}
