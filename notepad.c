#include <getopt.h>
#include <gtk/gtk.h>
#include <gtksourceview/gtksource.h>

GtkWidget *notebook;
GtkWidget *statusbar;
GtkWidget *text_view;
GtkWidget *window;

typedef struct tab {
  GtkSourceBuffer *sourceBuffer;
  char *filename;
  int modified;
} tab;

struct tab tabs[10];

int modified = 0;

gboolean typingCallback(GtkWidget *widget, GdkEventKey *event, gpointer data) {
  GtkTextIter start;
  GtkTextIter end;

  gint current_page = gtk_notebook_get_current_page(GTK_NOTEBOOK(notebook));
  gtk_text_buffer_get_start_iter(GTK_TEXT_BUFFER(tabs[current_page].sourceBuffer), &start);
  gtk_text_buffer_get_end_iter(GTK_TEXT_BUFFER(tabs[current_page].sourceBuffer), &end);

  char *str = gtk_text_buffer_get_text(GTK_TEXT_BUFFER(tabs[current_page].sourceBuffer), &start, &end, 0);

  char buf[256];
  sprintf(buf, "%lu", strlen(str));
  gtk_statusbar_remove_all(GTK_STATUSBAR(statusbar), 0);
  gtk_statusbar_push(GTK_STATUSBAR(statusbar), 0, buf);

  return FALSE;
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
    gint current_page = gtk_notebook_get_current_page(GTK_NOTEBOOK(notebook));
    tabs[current_page].filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));

    FILE *f = fopen(tabs[current_page].filename, "wb");
    if (f) {
      GtkTextIter start;
      GtkTextIter end;
      gtk_text_buffer_get_start_iter(GTK_TEXT_BUFFER(tabs[current_page].sourceBuffer), &start);
      gtk_text_buffer_get_end_iter(GTK_TEXT_BUFFER(tabs[current_page].sourceBuffer), &end);

      char *str = gtk_text_buffer_get_text(GTK_TEXT_BUFFER(tabs[current_page].sourceBuffer), &start, &end, 0);
      fwrite(str, 1, strlen(str), f);
      tabs[current_page].modified = 0;
    }
    fclose(f);
  }

  gtk_widget_destroy(dialog);
}

void save_file() {
  gint current_page = gtk_notebook_get_current_page(GTK_NOTEBOOK(notebook));
  if (!tabs[current_page].filename) {
    saveas_file();
    return;
  }
  FILE *f = fopen(tabs[current_page].filename, "wb");
  if (f) {
    GtkTextIter start;
    GtkTextIter end;
    gtk_text_buffer_get_start_iter(GTK_TEXT_BUFFER(tabs[current_page].sourceBuffer), &start);
    gtk_text_buffer_get_end_iter(GTK_TEXT_BUFFER(tabs[current_page].sourceBuffer), &end);

    char *str = gtk_text_buffer_get_text(GTK_TEXT_BUFFER(tabs[current_page].sourceBuffer), &start, &end, 0);
    fwrite(str, 1, strlen(str), f);
    tabs[current_page].modified = 0;
  }
  fclose(f);
}

void populate_buffer_from_file(char *filename, int idx) {
  FILE *f = fopen(filename, "rb");
  if (f) {
    fseek(f, 0L, SEEK_END);
    size_t len = ftell(f);
    rewind(f);

    char str[len + 1];
    fread(str, 1, len, f);
    str[len] = 0;
    fclose(f);

    if (g_utf8_validate(str, len, NULL) == FALSE) {
      fprintf(stderr, "WARNING: Invalid UTF-8 detected.\n");
      gchar *valid_text = g_utf8_make_valid(str, strlen(str));
      gtk_text_buffer_set_text(GTK_TEXT_BUFFER(tabs[idx].sourceBuffer), valid_text, strlen(valid_text));
      free(valid_text);
    } else {
      gtk_text_buffer_set_text(GTK_TEXT_BUFFER(tabs[idx].sourceBuffer), str, strlen(str));
    }

    tabs[idx].modified = 0;
  }
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

  gint current_page = gtk_notebook_get_current_page(GTK_NOTEBOOK(notebook));
  gint res = gtk_dialog_run(GTK_DIALOG(dialog));
  if (res == GTK_RESPONSE_ACCEPT) {
    tabs[current_page].filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));

    populate_buffer_from_file(tabs[current_page].filename, 0);
  }

  gtk_widget_destroy(dialog);
}

void new_file() {
  tabs[0].filename = NULL;
  gtk_text_buffer_set_text(GTK_TEXT_BUFFER(tabs[0].sourceBuffer), "", 0);
  tabs[0].modified = 0;
  num_tabs = 1;
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

int no_unsaved_changes() {
  for (int i = 0; i < num_tabs; i++) {
    if (tabs[i].modified) {
      return 0;
    }
  }
  return 1;
}

int ask_quit() {
  fflush(stdout);
  if (no_unsaved_changes()) {
    gtk_main_quit();
  } else {
    GtkWidget *dialog = gtk_message_dialog_new(GTK_WINDOW(window),
                                               GTK_DIALOG_DESTROY_WITH_PARENT,
                                               GTK_MESSAGE_QUESTION,
                                               GTK_BUTTONS_YES_NO,
                                               "Are you sure you want to quit without saving changes?");
    gtk_window_set_title(GTK_WINDOW(dialog), "Confirmation");

    gint res = gtk_dialog_run(GTK_DIALOG(dialog));
    if (res == GTK_RESPONSE_YES) {
      gtk_main_quit();
    }

    gtk_widget_destroy(dialog);
  }
  return TRUE;
}

gboolean keyPressCallback(GtkWidget *widget, GdkEventKey *event, gpointer data) {

  gint current_page = gtk_notebook_get_current_page(GTK_NOTEBOOK(notebook));

  if (event->keyval == 's' && event->state & GDK_CONTROL_MASK) {
    if (!tabs[current_page].filename) {
      saveas_file();
    } else {
      save_file();
    }
    return TRUE;
  }

  if (event->keyval == GDK_KEY_Escape) {
    ask_quit();
    return TRUE;
  }

  tabs[current_page].modified = 1;

  return FALSE;
}

void usage(char *argv[]) {
  fprintf(stderr,
          "Usage: %s [file]\n"
          " -h,--help     Print this usage message.\n"
          " -v,--verbose  Display additional logging information.\n"
          "",
          argv[0]);
  exit(EXIT_FAILURE);
}

int main(int argc, char *argv[]) {
  gtk_init(NULL, NULL);

  int verbose = 0;

  int opt;
  int option_index = 0;
  char *optstring = "hps:v";
  static struct option long_options[] = {
      {"help", no_argument, 0, 'h'},
      {"verbose", no_argument, 0, 'v'},
      {0, 0, 0, 0},
  };
  while ((opt = getopt_long(argc, argv, optstring, long_options, &option_index)) != -1) {
    if (opt == 'h') {
      usage(argv);
    } else if (opt == 'v') {
      verbose = 1;
    } else if (opt == '?') {
      usage(argv);
    } else {
      puts(optarg);
    }
  }

  GtkBuilder *builder = gtk_builder_new();
  gtk_builder_add_from_file(builder, "notepad.glade", NULL);

  GtkWidget *window = GTK_WIDGET(gtk_builder_get_object(builder, "window"));
  notebook = GTK_WIDGET(gtk_builder_get_object(builder, "notebook"));

  statusbar = GTK_WIDGET(gtk_builder_get_object(builder, "statusbar"));

  {
    int i = optind;
    int idx = 0;
    while (i < argc) {
      tabs[idx].filename = malloc(strlen(argv[i]) + 1);
      strcpy(tabs[idx].filename, argv[i]);

      GtkWidget *scrolled_view = gtk_scrolled_window_new(0, 0);
      gtk_container_add(GTK_CONTAINER(notebook), scrolled_view);
      gtk_widget_show(scrolled_view);

      tabs[idx].sourceBuffer = gtk_source_buffer_new(NULL);
      text_view = gtk_source_view_new_with_buffer(tabs[idx].sourceBuffer);

      gtk_container_add(GTK_CONTAINER(scrolled_view), text_view);
      gtk_widget_show(text_view);

      populate_buffer_from_file(tabs[idx].filename, idx);

      GtkWidget *page = gtk_notebook_get_nth_page(GTK_NOTEBOOK(notebook), idx);
      GtkWidget *label = gtk_notebook_get_tab_label(GTK_NOTEBOOK(notebook), page);
      gtk_notebook_set_tab_label(GTK_NOTEBOOK(notebook), page, gtk_label_new(tabs[idx].filename));

      GtkSourceLanguage *lang;
      GtkSourceLanguageManager *lm = gtk_source_language_manager_get_default();
      lang = gtk_source_language_manager_guess_language(lm, tabs[idx].filename, NULL);
      gtk_source_buffer_set_language(tabs[idx].sourceBuffer, lang);

      i++;
      idx++;
    }
    if(idx==0){
      tabs[0].filename = malloc(strlen("New") + 1);
      strcpy(tabs[0].filename, "New");

      GtkWidget *scrolled_view = gtk_scrolled_window_new(0, 0);
      gtk_container_add(GTK_CONTAINER(notebook), scrolled_view);
      gtk_widget_show(scrolled_view);

      tabs[0].sourceBuffer = gtk_source_buffer_new(NULL);
      text_view = gtk_source_view_new_with_buffer(tabs[0].sourceBuffer);

      gtk_container_add(GTK_CONTAINER(scrolled_view), text_view);
      gtk_widget_show(text_view);

      populate_buffer_from_file(tabs[0].filename, 0);

      GtkWidget *page = gtk_notebook_get_nth_page(GTK_NOTEBOOK(notebook), 0);
      GtkWidget *label = gtk_notebook_get_tab_label(GTK_NOTEBOOK(notebook), page);
      gtk_notebook_set_tab_label(GTK_NOTEBOOK(notebook), page, gtk_label_new(tabs[0].filename));
    }
  }


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
  g_signal_connect(G_OBJECT(window), "delete-event", G_CALLBACK(ask_quit), NULL);
  g_signal_connect(G_OBJECT(quit), "activate", G_CALLBACK(ask_quit), NULL);
  g_signal_connect(G_OBJECT(new), "activate", G_CALLBACK(new_file), NULL);
  g_signal_connect(G_OBJECT(open), "activate", G_CALLBACK(open_file), NULL);
  g_signal_connect(G_OBJECT(saveas), "activate", G_CALLBACK(saveas_file), NULL);
  g_signal_connect(G_OBJECT(save), "activate", G_CALLBACK(save_file), NULL);
  g_signal_connect(G_OBJECT(about), "activate", G_CALLBACK(show_about), NULL);

  gtk_main();
}
