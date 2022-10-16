#include <gtk/gtk.h>

GtkWidget *text_view;
GtkWidget *statusbar;

gboolean typingCallback(GtkWidget *widget, GdkEventKey *event,
                        gpointer data) {

  GtkTextIter start;
  GtkTextIter end;

  GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text_view));
  gtk_text_buffer_get_start_iter(buffer, &start);
  gtk_text_buffer_get_end_iter(buffer, &end);

  char *str = gtk_text_buffer_get_text(buffer, &start, &end, 0);

  char buf[256];
  sprintf(buf, "%d", strlen(str));
  gtk_statusbar_remove_all(GTK_STATUSBAR(statusbar), 0);
  gtk_statusbar_push(GTK_STATUSBAR(statusbar), 0, buf);

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

void open_file() {
  GtkWidget *dialog = gtk_file_chooser_dialog_new("Open File",
                                       NULL,
                                       GTK_FILE_CHOOSER_ACTION_OPEN,
                                       "_Cancel",
                                       GTK_RESPONSE_CANCEL,
                                       "_Open",
                                       GTK_RESPONSE_ACCEPT,
                                       NULL);

  gint res = gtk_dialog_run(GTK_DIALOG(dialog));
  if (res == GTK_RESPONSE_ACCEPT) {
    char *filename;
    GtkFileChooser *chooser = GTK_FILE_CHOOSER(dialog);
    filename = gtk_file_chooser_get_filename(chooser);

    FILE *f = fopen(filename, "rb");
    if (f) {
      fseek(f, 0L, SEEK_END);
      size_t len = ftell(f);
      rewind(f);

      char str[len + 1];
      fread(str, 1, len, f);
      str[len] = 0;
      fclose(f);

      //GtkTextIter iter;
      GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text_view));
      //gtk_text_buffer_get_iter_at_offset(buffer, &iter, 0);
      //gtk_text_buffer_insert(buffer, &iter, str, -1);

      gtk_text_buffer_set_text(buffer, str, strlen(str));
    }

    g_free(filename);
  }

  gtk_widget_destroy(dialog);
}

int main(int argc, char *argv[]) {
  gtk_init(&argc, &argv);

  GtkWidget *window = gtk_window_new(GTK_WINDOW_TOPLEVEL);

  GtkWidget *all = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
  gtk_container_add(GTK_CONTAINER(window), all);

  GtkWidget *menubar = gtk_menu_bar_new();
  GtkWidget *fileMenu = gtk_menu_new();

  GtkWidget *file = gtk_menu_item_new_with_label("File");
  GtkWidget *open = gtk_menu_item_new_with_label("Open");
  GtkWidget *quit = gtk_menu_item_new_with_label("Quit");
  GtkWidget *example = gtk_menu_item_new_with_label("Example");

  gtk_menu_item_set_submenu(GTK_MENU_ITEM(file), fileMenu);
  gtk_menu_shell_append(GTK_MENU_SHELL(menubar), file);
  gtk_menu_shell_append(GTK_MENU_SHELL(fileMenu), open);
  gtk_menu_shell_append(GTK_MENU_SHELL(fileMenu), quit);

  gtk_menu_shell_append(GTK_MENU_SHELL(menubar), example);
  gtk_box_pack_start(GTK_BOX(all), menubar, FALSE, FALSE, 0);

  GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
  gtk_container_add(GTK_CONTAINER(all), box);

  GtkWidget *scrolled_window = gtk_scrolled_window_new(0, 0);
  gtk_container_add(GTK_CONTAINER(window), scrolled_window);

  text_view = gtk_text_view_new();
  gtk_container_add(GTK_CONTAINER(scrolled_window), text_view);

  statusbar = gtk_statusbar_new();
  gtk_container_add(GTK_CONTAINER(box), statusbar);
  {
    GtkStyleContext *context = gtk_widget_get_style_context(statusbar);
    gtk_style_context_add_class(context, "status-bar");
  }

  GtkCssProvider *css = gtk_css_provider_new();
  gtk_css_provider_load_from_path(css, "style.css", NULL);
  gtk_style_context_add_provider_for_screen(gdk_screen_get_default(), GTK_STYLE_PROVIDER(css), GTK_STYLE_PROVIDER_PRIORITY_USER);

  gtk_widget_add_events(window, GDK_KEY_PRESS_MASK);
  g_signal_connect(G_OBJECT(window), "key-press-event", G_CALLBACK(keyPressCallback), NULL);
  g_signal_connect(G_OBJECT(text_view), "draw", G_CALLBACK(typingCallback), NULL);
  g_signal_connect(G_OBJECT(window), "destroy", G_CALLBACK(gtk_main_quit), NULL);
  g_signal_connect(G_OBJECT(quit), "activate", G_CALLBACK(gtk_main_quit), NULL);
  g_signal_connect(G_OBJECT(open), "activate", G_CALLBACK(open_file), NULL);

  gtk_widget_show_all(window);
  gtk_main();
}
