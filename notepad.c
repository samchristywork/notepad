#include <gtk/gtk.h>

GtkWidget *text_view;

int main(int argc, char *argv[]) {
  gtk_init(&argc, &argv);

  GtkWidget *window = gtk_window_new(GTK_WINDOW_TOPLEVEL);

  text_view = gtk_text_view_new();
  gtk_container_add(GTK_CONTAINER(window), text_view);

  gtk_widget_show_all(window);
  gtk_main();
}
