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
