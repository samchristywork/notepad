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
