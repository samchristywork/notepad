#include <gtk/gtk.h>

GtkWidget *text_view;
GtkWidget *statusbar;

char *saveFileName = NULL;

gboolean typingCallback(GtkWidget *widget, GdkEventKey *event, gpointer data) {
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

gboolean keyPressCallback(GtkWidget *widget, GdkEventKey *event, gpointer data) {
  if (event->keyval == GDK_KEY_Escape) {
    exit(EXIT_SUCCESS);
    return TRUE;
  }
  return FALSE;
}

void save_file() {
  if (!saveFileName) {
    return;
  }
  FILE *f = fopen(saveFileName, "wb");
  if (f) {
    GtkTextIter start;
    GtkTextIter end;
    GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text_view));
    gtk_text_buffer_get_start_iter(buffer, &start);
    gtk_text_buffer_get_end_iter(buffer, &end);

    char *str = gtk_text_buffer_get_text(buffer, &start, &end, 0);
    fwrite(str, 1, strlen(str), f);
  }
  fclose(f);
}

void saveas_file() {
  GtkWidget *dialog = gtk_file_chooser_dialog_new("Save File",
                                                  NULL,
                                                  GTK_FILE_CHOOSER_ACTION_SAVE,
                                                  "_Cancel",
                                                  GTK_RESPONSE_CANCEL,
                                                  "_Save",
                                                  GTK_RESPONSE_ACCEPT,
                                                  NULL);

  gint res = gtk_dialog_run(GTK_DIALOG(dialog));
  if (res == GTK_RESPONSE_ACCEPT) {
    saveFileName = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));

    FILE *f = fopen(saveFileName, "wb");
    if (f) {
      GtkTextIter start;
      GtkTextIter end;
      GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text_view));
      gtk_text_buffer_get_start_iter(buffer, &start);
      gtk_text_buffer_get_end_iter(buffer, &end);

      char *str = gtk_text_buffer_get_text(buffer, &start, &end, 0);
      fwrite(str, 1, strlen(str), f);
    }
    fclose(f);
  }

  gtk_widget_destroy(dialog);
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
    saveFileName = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));

    FILE *f = fopen(saveFileName, "rb");
    if (f) {
      fseek(f, 0L, SEEK_END);
      size_t len = ftell(f);
      rewind(f);

      char str[len + 1];
      fread(str, 1, len, f);
      str[len] = 0;
      fclose(f);

      GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text_view));
      gtk_text_buffer_set_text(buffer, str, strlen(str));
    }
  }

  gtk_widget_destroy(dialog);
}

void new_file() {
  saveFileName = NULL;
  GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text_view));
  gtk_text_buffer_set_text(buffer, "", 0);
}

void show_about() {
  GdkPixbuf *example_logo = gdk_pixbuf_new_from_file("logo.png", NULL);
  char *authors[] = {
      "Sam Christy",
      NULL};
  gtk_show_about_dialog(NULL,
                        "authors", authors,
                        "comments", "\"There are two ways of constructing a software design: One way is to make it so simple that there are obviously no deficiencies, and the other way is to make it so complicated that there are no obvious deficiencies. The first method is far more difficult.\" - C. A. R. Hoare",
                        "copyright", "©2022 Sam Christy",
                        "license", "GNU General Public License version 3 (GPLv3)",
                        "license-type", GTK_LICENSE_GPL_3_0,
                        "logo", example_logo,
                        "program-name", "Notepad",
                        "title", "About Notepad",
                        "version", "v1.0.0",
                        "website", "https://github.com/samchristywork",
                        "website-label", "github.com/samchristywork",
                        NULL);
}

int main(int argc, char *argv[]) {
  gtk_init(&argc, &argv);

  GtkBuilder *builder = gtk_builder_new();
  gtk_builder_add_from_file(builder, "notepad.glade", NULL);

  GtkWidget *window = GTK_WIDGET(gtk_builder_get_object(builder, "window"));
  text_view = GTK_WIDGET(gtk_builder_get_object(builder, "text-view"));
  statusbar = GTK_WIDGET(gtk_builder_get_object(builder, "statusbar"));

  GtkWidget *about = GTK_WIDGET(gtk_builder_get_object(builder, "about"));
  GtkWidget *new = GTK_WIDGET(gtk_builder_get_object(builder, "new"));
  GtkWidget *open = GTK_WIDGET(gtk_builder_get_object(builder, "open"));
  GtkWidget *quit = GTK_WIDGET(gtk_builder_get_object(builder, "quit"));
  GtkWidget *save = GTK_WIDGET(gtk_builder_get_object(builder, "save"));
  GtkWidget *saveas = GTK_WIDGET(gtk_builder_get_object(builder, "saveas"));

  gtk_builder_connect_signals(builder, NULL);
  g_signal_connect(G_OBJECT(window), "destroy", G_CALLBACK(gtk_main_quit), NULL);

  g_object_unref(builder);
  gtk_widget_show(window);


  GtkCssProvider *css = gtk_css_provider_new();
  gtk_css_provider_load_from_path(css, "style.css", NULL);
  gtk_style_context_add_provider_for_screen(gdk_screen_get_default(), GTK_STYLE_PROVIDER(css), GTK_STYLE_PROVIDER_PRIORITY_USER);

  gtk_widget_add_events(window, GDK_KEY_PRESS_MASK);
  g_signal_connect(G_OBJECT(window), "key-press-event", G_CALLBACK(keyPressCallback), NULL);
  g_signal_connect(G_OBJECT(text_view), "draw", G_CALLBACK(typingCallback), NULL);
  g_signal_connect(G_OBJECT(window), "destroy", G_CALLBACK(gtk_main_quit), NULL);
  g_signal_connect(G_OBJECT(quit), "activate", G_CALLBACK(gtk_main_quit), NULL);
  g_signal_connect(G_OBJECT(new), "activate", G_CALLBACK(new_file), NULL);
  g_signal_connect(G_OBJECT(open), "activate", G_CALLBACK(open_file), NULL);
  g_signal_connect(G_OBJECT(saveas), "activate", G_CALLBACK(saveas_file), NULL);
  g_signal_connect(G_OBJECT(save), "activate", G_CALLBACK(save_file), NULL);
  g_signal_connect(G_OBJECT(about), "activate", G_CALLBACK(show_about), NULL);

  gtk_main();
}
