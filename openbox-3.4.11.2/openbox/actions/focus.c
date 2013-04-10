#include "openbox/actions.h"
#include "openbox/event.h"
#include "openbox/client.h"
#include "openbox/focus.h"
#include "openbox/screen.h"

typedef struct {
    gboolean here;
    gboolean activate;
    gboolean stop_int;
} Options;

static gpointer setup_func(ObParseInst *i, xmlDocPtr doc, xmlNodePtr node);
static gpointer setup_activate_func(ObParseInst *i,
                                    xmlDocPtr doc, xmlNodePtr node);
static void     free_func(gpointer options);
static gboolean run_func(ObActionsData *data, gpointer options);

void action_focus_startup(void)
{
    actions_register("Focus",
                     setup_func,
                     free_func,
                     run_func,
                     NULL, NULL);
    actions_register("Activate",
                     setup_activate_func,
                     free_func,
                     run_func,
                     NULL, NULL);
}

static gpointer setup_func(ObParseInst *i, xmlDocPtr doc, xmlNodePtr node)
{
    xmlNodePtr n;
    Options *o;

    o = g_new0(Options, 1);
    o->stop_int = TRUE;

    if ((n = parse_find_node("here", node)))
        o->here = parse_bool(doc, n);
    if ((n = parse_find_node("stopInteractive", node)))
        o->stop_int = parse_bool(doc, n);
    return o;
}

static gpointer setup_activate_func(ObParseInst *i,
                                    xmlDocPtr doc, xmlNodePtr node)
{
    Options *o = setup_func(i, doc, node);
    o->activate = TRUE;
    return o;
}

static void free_func(gpointer options)
{
    Options *o = options;

    g_free(o);
}

/* Always return FALSE because its not interactive */
static gboolean run_func(ObActionsData *data, gpointer options)
{
    Options *o = options;

    if (data->client) {
/*
        ob_debug("button %d focusable %d context %d %d %d\n",
                 data->button, client_mouse_focusable(data->client),
                 data->context,
                 OB_FRAME_CONTEXT_CLIENT, OB_FRAME_CONTEXT_FRAME);
*/
        if (data->button == 0 || client_mouse_focusable(data->client) ||
            (data->context != OB_FRAME_CONTEXT_CLIENT &&
             data->context != OB_FRAME_CONTEXT_FRAME))
        {
            if (o->stop_int)
                actions_interactive_cancel_act();

            actions_client_move(data, TRUE);
            client_activate(data->client, TRUE, o->here,
                            o->activate, o->activate, TRUE);
            actions_client_move(data, FALSE);
        }
    } else if (data->context == OB_FRAME_CONTEXT_DESKTOP) {
        if (o->stop_int)
            actions_interactive_cancel_act();

        /* focus action on the root window. make keybindings work for this
           openbox instance, but don't focus any specific client */
        focus_nothing();
    }

    return FALSE;
}
