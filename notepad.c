#include <gtk/gtk.h>

GtkWidget *text_view;

gboolean typingCallback(GtkWidget *widget, GdkEventKey *event,
             gpointer data) {

  GtkTextIter start;
  GtkTextIter end;

  GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text_view));
  gtk_text_buffer_get_start_iter(buffer, &start);
  gtk_text_buffer_get_end_iter(buffer, &end);

  char *str=gtk_text_buffer_get_text(buffer, &start, &end, 0);
  printf("%s\n", str);

  return FALSE;
}

gboolean keyPressCallback(GtkWidget *widget, GdkEventKey *event,
                          gpointer data) {

  if (event->keyval == GDK_KEY_Escape) {
    exit(EXIT_SUCCESS);
    return TRUE;
  }
  return FALSE;
}

int main(int argc, char *argv[]) {
  gtk_init(&argc, &argv);

  GtkWidget *window = gtk_window_new(GTK_WINDOW_TOPLEVEL);

  GtkWidget *scrolled_window = gtk_scrolled_window_new(0, 0);
  gtk_container_add(GTK_CONTAINER(window), scrolled_window);

  text_view = gtk_text_view_new();
  gtk_container_add(GTK_CONTAINER(scrolled_window), text_view);

  gtk_widget_add_events(window, GDK_KEY_PRESS_MASK);
  g_signal_connect(G_OBJECT(window), "key_press_event",
                   G_CALLBACK(keyPressCallback), NULL);

  g_signal_connect(G_OBJECT(text_view), "key_press_event",
                   G_CALLBACK(typingCallback), NULL);

  gtk_widget_show_all(window);
  gtk_main();
}
