#ifndef LIBXML2_PARSE
#define LIBXML2_PARSE
struct RAISE{
	char src[50];
	int winid;

};
typedef enum {
	OB_START_APP,
	OB_KILL_APP,
	OB_EXIT_APP,
	OB_SET_FULLSCREEN,
	OB_SET_HIDE,//
	OB_SET_MIN_RUN,
	OB_SET_BOTTOM,
	OB_SET_TOP,
	OB_SET_MAXIMIZE,
	OB_GET_APPS_LIST,
	OB_GET_APP_STATE,
	OB_EXIT,
	OB_RESTART,
	OB_REFRESH
}WM_METHOD;
typedef struct{
	int winid;
	int pid;
	char title[40];
	char cmd[40];
	char name[40];
}APP_INFO;
typedef struct {
	int x;
	int y;
	int width;
	int height;
	int extend;
	int desktop;
}APP_DATA;
typedef struct {
	//here may add sender socket msg
	WM_METHOD method;
	char src[40];
	int id;
	APP_INFO appInfo;
	APP_DATA appData;
}OB_SOCKET;
#endif
