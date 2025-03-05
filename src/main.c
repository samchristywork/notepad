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
