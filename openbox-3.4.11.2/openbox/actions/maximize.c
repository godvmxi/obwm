#include "openbox/actions.h"
#include "openbox/client.h"

/* These match the values for client_maximize */
typedef enum {
    BOTH = 0,
    HORZ = 1,
    VERT = 2
} MaxDirection;

typedef struct {
    MaxDirection dir;
} Options;

static gpointer setup_both_func(ObParseInst *i, xmlDocPtr doc,
                              xmlNodePtr node);
static gpointer setup_horz_func(ObParseInst *i, xmlDocPtr doc,
                               xmlNodePtr node);
static gpointer setup_vert_func(ObParseInst *i,
                                  xmlDocPtr doc, xmlNodePtr node);
static gboolean run_func_on(ObActionsData *data, gpointer options);
static gboolean run_func_off(ObActionsData *data, gpointer options);
static gboolean run_func_toggle(ObActionsData *data, gpointer options);

void action_maximize_startup(void)
{
    actions_register("MaximizeFull", setup_both_func, g_free,
                     run_func_on, NULL, NULL);
    actions_register("UnmaximizeFull", setup_both_func, g_free,
                     run_func_off, NULL, NULL);
    actions_register("ToggleMaximizeFull", setup_both_func, g_free,
                     run_func_toggle, NULL, NULL);
    actions_register("MaximizeHorz", setup_horz_func, g_free,
                     run_func_on, NULL, NULL);
    actions_register("UnmaximizeHorz", setup_horz_func, g_free,
                     run_func_off, NULL, NULL);
    actions_register("ToggleMaximizeHorz", setup_horz_func, g_free,
                     run_func_toggle, NULL, NULL);
    actions_register("MaximizeVert", setup_vert_func, g_free,
                     run_func_on, NULL, NULL);
    actions_register("UnmaximizeVert", setup_vert_func, g_free,
                     run_func_off, NULL, NULL);
    actions_register("ToggleMaximizeVert", setup_vert_func, g_free,
                     run_func_toggle, NULL, NULL);
}

static gpointer setup_both_func(ObParseInst *i, xmlDocPtr doc, xmlNodePtr node)
{
    Options *o = g_new0(Options, 1);
    o->dir = BOTH;
    return o;
}

static gpointer setup_horz_func(ObParseInst *i, xmlDocPtr doc, xmlNodePtr node)
{
    Options *o = g_new0(Options, 1);
    o->dir = HORZ;
    return o;
}

static gpointer setup_vert_func(ObParseInst *i, xmlDocPtr doc, xmlNodePtr node)
{
    Options *o = g_new0(Options, 1);
    o->dir = VERT;
    return o;
}

/* Always return FALSE because its not interactive */
static gboolean run_func_on(ObActionsData *data, gpointer options)
{
    Options *o = options;
    if (data->client) {
        actions_client_move(data, TRUE);
        client_maximize(data->client, TRUE, o->dir);
        actions_client_move(data, FALSE);
    }
    return FALSE;
}

/* Always return FALSE because its not interactive */
static gboolean run_func_off(ObActionsData *data, gpointer options)
{
    Options *o = options;
    if (data->client) {
        actions_client_move(data, TRUE);
        client_maximize(data->client, FALSE, o->dir);
        actions_client_move(data, FALSE);
    }
    return FALSE;
}

/* Always return FALSE because its not interactive */
static gboolean run_func_toggle(ObActionsData *data, gpointer options)
{
    Options *o = options;
    if (data->client) {
        gboolean toggle;
        actions_client_move(data, TRUE);
        toggle = ((o->dir == HORZ && !data->client->max_horz) ||
                  (o->dir == VERT && !data->client->max_vert) ||
                  (o->dir == BOTH &&
                   !(data->client->max_horz && data->client->max_vert)));
        client_maximize(data->client, toggle, o->dir);
        actions_client_move(data, FALSE);
    }
    return FALSE;
}
