/* cc -Wall `pkg-config --cflags --libs glib-2.0` loop.c -o loop */
#include <stdio.h>
#include <glib.h>
gboolean callback(gpointer data)
{
    printf("%s\n", (char*)data);
    return TRUE;
}
int main()
{
    GMainLoop* main_loop = NULL;
    main_loop = g_main_loop_new (NULL, FALSE);
    /* interval, function callback, userdata */
    g_timeout_add_seconds(10, callback, "callback_function");
    g_main_loop_run(main_loop);
    return 0;
}
