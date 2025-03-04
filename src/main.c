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
