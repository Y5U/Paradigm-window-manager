#include <X11/Xlib.h>
#include <X11/extensions/Xinerama.h>
#include <X11/extensions/Xrandr.h>
#include <X11/keysym.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "pwm.h"
#include "config.h"

/* macros */
#define SECTION(x, div, sel) (x / div * sel)
#define LENGTH(x) (sizeof x / sizeof *x)
#define FITSECTION(dpy, w, sec) XMoveResizeWindow(dpy, w, sec[0], sec[1], sec[2], sec[3])
#define HORIZ_HALVE_SEC(sec) (sec[2] /= 2)
#define VERT_HALVE_SEC(sec) (sec[3] /= 2)

/* variables */
static int 		sel[MONITORLENGTH][WORKSPACELENGTH];/* Selected window */
static int		CM = 0;					/* current monitor */
static int		CW = 0;					/* current workpace */
static int		mode = 0;				/* tiling(0) or floating(1) */
static bool 		cmdMD = false; 				/* Command mode */
static Window 		workspace[MONITORLENGTH][WORKSPACELENGTH][WINDOWLENGTH]; /* monitor, workspace, windows */
static Window		notifW; 				/* notifications window */
static int		screen;
static int		nMonitors;
static Display		*dpy;
XineramaScreenInfo 	*screens;

#define SH	XDisplayHeight(dpy, screen)	/* screen height */
#define SW	XDisplayWidth(dpy, screen)	/* screen height */
#define CMH	screens[CM].height		/* Current Monitor height */
#define CMW	screens[CM].width 		/* Current Monitor height */
#define CMX	screens[CM].x_org 		/* Current Monitor top left corner X coordinate */
#define CMY	screens[CM].y_org 		/* Current Monitor top left corner Y coordinate */

int
arrSize(long unsigned int arr[])
{
        int i = 0;
        while(arr[i] != 0) { i++; }
        return i;
}

bool
checkArr(long unsigned int arr[], long unsigned int key, int length)
{
        for(int i = 0; i < length; i++){
                if(arr[i] == key) { return true; }
        }
        return false;
}

int
whatMonitor(Window w)
{
	XWindowAttributes attrs;
	
	XGetWindowAttributes(dpy, w, &attrs);

	for(int m = 0; m < nMonitors; m++){
		if(attrs.x >= screens[m].x_org && attrs.x <= 
				(screens[m].x_org + screens[m].width)){
			return m;
		}
	}
	return -1;
}

unsigned int oldNChild;

void
checkWindows()
{
	Window parent, root, *children, focused;
	unsigned int nChild;
	XWindowAttributes attrs;
	int revert;

	XGetInputFocus(dpy, &focused, &revert);

	XQueryTree(dpy, DefaultRootWindow(dpy), &root, &parent, &children, &nChild);

	/* automatically resize new windows */
	if(mode == 0 && nChild > oldNChild){
		if(children[nChild - 1] != notifW)
			XMoveResizeWindow(dpy, children[nChild - 1], CMX, CMY, CMW, CMH);
	}

	/* remove closed windows from workspace */
	for(int i = 0; i < arrSize(workspace[CM][CW]); i++){
		if(!checkArr(children, workspace[CM][CW][i], nChild)){
			for(int n = 0; n < WINDOWLENGTH - 1; n++){
				workspace[CM][CW][n] = workspace[CM][CW][n + 1];
			}
		}
	}

	/* add open windows to workspace */
	for(int i = 0; i < (int)nChild; i++){
		if(children[i] != notifW && !checkArr(workspace[CM][CW ^ 1], children[i], WINDOWLENGTH) && 
				!checkArr(workspace[CM][CW], children[i], WINDOWLENGTH)){
			XGetWindowAttributes(dpy, children[i], &attrs);
			if(attrs.map_state == 2 && whatMonitor(children[i]) == CM){
				workspace[CM][CW][arrSize(workspace[CM][CW])] = children[i];
			}
		}
	}

	/* to protect from freezing when all windows are killed */
	if(arrSize(workspace[CM][CW]) <= 1)
		XSetInputFocus(dpy, DefaultRootWindow(dpy), RevertToParent, CurrentTime);

	oldNChild = nChild;
}

void
SWMonitor(int arrs[10])
{
	/* arrs[0] = left(0), right(1) */

	/* exit function if there is only one monitor */
	if(nMonitors == 1)
		return;

	if(arrs[0] == 0){
		if((CM -= 1) > 0){
			if(workspace[CM][CW][0] != 0 && workspace[CM][CW][0] != 1){
				XSetInputFocus(dpy, workspace[CM][CW][sel[CM][CW]], RevertToNone, CurrentTime); 
			} else {
				XSetInputFocus(dpy, None, RevertToNone, CurrentTime);
			}
		}
		else CM = 0;
	}
	else if(arrs[0] == 1) {
		if((CM += 1) < nMonitors){
			if(workspace[CM][CW][0] != 0 && workspace[CM][CW][0] != 1){
				XSetInputFocus(dpy, workspace[CM][CW][sel[CM][CW]], RevertToNone, CurrentTime); 
			} else {
				XSetInputFocus(dpy, None, RevertToNone, CurrentTime);
			}
		}
		else CM = nMonitors;
	}
}

void
SWWorkspace()
{ /* Switch Workspaces */
	Window parent, root, *children;
	unsigned int nChild;

	XQueryTree(dpy, DefaultRootWindow(dpy), &root, &parent, &children, &nChild);

	/* hide all windows */
	for(int i = 0; i < (int)nChild; i++){
		XUnmapWindow(dpy, children[i]); }
	/* switch workspace */
	if((CW += 1) > WORKSPACELENGTH)
		CW = 0;
	/* show all the windows on the current workspace */
	for(int i = 0; i < arrSize(workspace[CM][CW]); i++){
		XMapWindow(dpy, workspace[CM][CW][i]); }
	/* set focus to the selected window in the current workspace */
	if(workspace[CM][CW][0] != 0 && workspace[CM][CW][0] != 1)
		XSetInputFocus(dpy, workspace[CM][CW][sel[CM][CW]], RevertToNone, CurrentTime);
}

void
Tile(int args[10]) 
{
	Window parent, root, *children;
	unsigned int nChild;
	XWindowAttributes attrs;
	Window windows[WINDOWLENGTH];
	memcpy(windows, workspace[CM][CW], WINDOWLENGTH);

	XQueryTree(dpy, DefaultRootWindow(dpy), &root, &parent, &children, &nChild);

	for(int i = 0; i < arrSize(windows) - 1; i++){
		if(checkArr(children, windows[i], nChild)){
			XGetWindowAttributes(dpy, windows[i], &attrs);
			if(attrs.map_state != 2){
				for(int n = 0; n < WINDOWLENGTH - 1; n++){
					workspace[CM][CW][n] = workspace[CM][CW][n + 1];
				}
			}
		}
	}

	/* auto */
	if(args[0] == 0){
		switch(arrSize(workspace[CM][CW])){
			case 1 :
				XMoveResizeWindow(dpy, workspace[CM][CW][0], 0, 0, CMW, CMH);
				break;
			case 2 :
				XMoveResizeWindow(dpy, workspace[CM][CW][0], 0, 0, CMW / 2, CMH);
				XMoveResizeWindow(dpy, workspace[CM][CW][1], CMW / 2, 0, CMW / 2, CMH);
				break;
			case 3 :
				XMoveResizeWindow(dpy, workspace[CM][CW][0], 0, 0, CMW / 2, CMH);
				XMoveResizeWindow(dpy, workspace[CM][CW][1], CMW / 2, 0, CMW / 2, CMH / 2);
				XMoveResizeWindow(dpy, workspace[CM][CW][2], CMW / 2, CMH / 2, CMW / 2, CMH / 2);
				break;
			case 4 :
				XMoveResizeWindow(dpy, workspace[CM][CW][0], 0, 0, CMW / 2, CMH / 2);
				XMoveResizeWindow(dpy, workspace[CM][CW][1], CMW / 2, 0, CMW / 2, CMH / 2);
				XMoveResizeWindow(dpy, workspace[CM][CW][2], 0, CMH / 2, CMW / 2, CMH / 2);
				XMoveResizeWindow(dpy, workspace[CM][CW][3], CMW / 2, CMH / 2, CMW / 2, CMH / 2);
				break;
			default :
				return;
		}
	}
	/* horizontal */
	else if(args[0] == 1){
		for(int i = 0; i < arrSize(workspace[CM][CW]); i++) {
			XGetWindowAttributes(dpy, workspace[CM][CW][i], &attrs);
			if(attrs.map_state == 2){
				XMoveResizeWindow(dpy, workspace[CM][CW][i], SECTION(CMW, 
					arrSize(workspace[CM][CW]), i), 0, CMW / arrSize(workspace[CM][CW]), CMH);
			}
		}
	}
	/* vertical */
	else if(args[0] == 2){
		for(int i = 0; i < arrSize(workspace[CM][CW]); i++) {
			XGetWindowAttributes(dpy, workspace[CM][CW][i], &attrs);
			if(attrs.map_state == 2){
				XMoveResizeWindow(dpy, workspace[CM][CW][i], 0, SECTION(CMH,
					arrSize(workspace[CM][CW]), i), CMW, 
						CMH / arrSize(workspace[CM][CW]));
			}
		}
	} 
}

/* Switch window */
void
SWWindow(void)
{
	if(arrSize(workspace[CM][CW]) < 1)
			return;
	/* window variables */
	unsigned int nChild;
	XWindowAttributes attrs;
	Window parent, root, *children;
	
	XQueryTree(dpy, DefaultRootWindow(dpy), &root, &parent, &children, &nChild);

	/* inc sel and get info */
	if((sel[CM][CW] += 1) >= (int)arrSize(workspace[CM][CW])) { sel[CM][CW] = 0; }
	XGetWindowAttributes(dpy, workspace[CM][CW][sel[CM][CW]], &attrs);

	/* make sure window is viewable and on the correct monitor */
	if(attrs.map_state == 2 && whatMonitor(workspace[CM][CW][sel[CM][CW]]) == CM)
		XSetInputFocus(dpy, workspace[CM][CW][sel[CM][CW]], RevertToNone, CurrentTime);
	/* try again */
	else {
		/* inc sel and get info */
		if((sel[CM][CW] += 1) >= (int)arrSize(workspace[CM][CW])) { sel[CM][CW] = 0; }
		XGetWindowAttributes(dpy, workspace[CM][CW][sel[CM][CW]], &attrs);

		/* make sure window is viewable and on the correct monitor */
		if(attrs.map_state == 2 && whatMonitor(workspace[CM][CW][sel[CM][CW]]) == CM)
			XSetInputFocus(dpy, workspace[CM][CW][sel[CM][CW]], RevertToNone, CurrentTime);
	}

	/* display notification ontop of focused window */
	#if notifWP  ==  0
	XMoveWindow(dpy, notifW, attrs.x, attrs.y);
	#elif notifWP == 1
	XMoveWindow(dpy, notifW, (attrs.x + attrs.width) - notifWW, attrs.y);
	#elif notifWP == 2
	XMoveWindow(dpy, notifW, attrs.x, (attrs.y + attrs.height) - notifWH);
	#elif notifWP == 3
	XMoveWindow(dpy, notifW, (attrs.x + attrs.width) - notifWW, (attrs.y + attrs.height) - notifWH);
	#else
	XMoveWindow(dpy, notifW, (attrs.x + (attrs.width / 2)) - notifWW, (attrs.y + (attrs.height / 2)) - notifWH);
	#endif

	XMapWindow(dpy, notifW);
	XRaiseWindow(dpy, notifW);
}

void
MRWindow(int args[10]) /* move resize window */
{
	/* args[0] = resize(0), or move(1), args[1] = left(0), right(1), up(2), down(3) */
	int rev;
	Window focused;
	XWindowAttributes attrs;

	XGetInputFocus(dpy, &focused, &rev);
	/* check if an actual window is focused */
	if(focused == 0 || focused == 1)
		return;
	if(args[0] == 0){
		switch(args[1]){
			case 0:
				XGetWindowAttributes(dpy, focused, &attrs);
				if(attrs.width <= MW) return;
				XResizeWindow(dpy, focused, attrs.width - RS, attrs.height);
				break;
			case 1:
				XGetWindowAttributes(dpy, focused, &attrs);
				if((attrs.x + attrs.width) >= CMW) return;
				XResizeWindow(dpy, focused, attrs.width + RS, attrs.height);
				break;
			case 2:
				XGetWindowAttributes(dpy, focused, &attrs);
				if(attrs.height <= MH) return;
				XResizeWindow(dpy, focused, attrs.width, attrs.height - RS);
				break;
			case 3:
				XGetWindowAttributes(dpy, focused, &attrs);
				if((attrs.y + attrs.height) >= CMH) return;
				XResizeWindow(dpy, focused, attrs.width, attrs.height + RS);
				break;
		}
	}
	else if(args[0] == 1){
		switch(args[1]){
			case 0:
				XGetWindowAttributes(dpy, focused, &attrs);
				if(attrs.x <= 0) return;
				XMoveWindow(dpy, focused, attrs.x - MS, attrs.y);
				break;
			case 1:
				XGetWindowAttributes(dpy, focused, &attrs);
				if((attrs.x + attrs.width) >= CMW) return;
				XMoveWindow(dpy, focused, attrs.x + MS, attrs.y);
				break;
			case 2:
				XGetWindowAttributes(dpy, focused, &attrs);
				if(attrs.y <= 0) return;
				XMoveWindow(dpy, focused, attrs.x, attrs.y - MS);
				break;
			case 3:
				XGetWindowAttributes(dpy, focused, &attrs);
				if((attrs.y + attrs.height) >= CMH) return;
				XMoveWindow(dpy, focused, attrs.x, attrs.y + MS);
				break;
		}
	}
}

void
ToggleCommandMode(void)
{
	if(!cmdMD) {
		cmdMD = true;
		XGrabKeyboard(dpy, DefaultRootWindow(dpy), True, GrabModeAsync, GrabModeAsync, CurrentTime);
	}
	else if(cmdMD){
		cmdMD = false;
		XUngrabKeyboard(dpy, CurrentTime);
		XGrabKey(dpy, XKeysymToKeycode(dpy, MODKEY), 0, DefaultRootWindow(dpy), 
			True, GrabModeAsync, GrabModeAsync);
		//XSetInputFocus(dpy, None, RevertToNone, CurrentTime);
	}
}

void
ExitCommandMode()
{
	cmdMD = false;
	XUngrabKeyboard(dpy, CurrentTime);
	XGrabKey(dpy, XKeysymToKeycode(dpy, MODKEY), 0, DefaultRootWindow(dpy), 
		True, GrabModeAsync, GrabModeAsync);
}

void
spawn(int args[10])
{
	switch(args[0]){
		case 0:
			system("st&");
			break;
		case 1:
			cmdMD = false;
			XUngrabKeyboard(dpy, CurrentTime);
			XGrabKey(dpy, XKeysymToKeycode(dpy, MODKEY), 0, DefaultRootWindow(dpy), 
				True, GrabModeAsync, GrabModeAsync);
			system("dmenu_run");
			break;
	}
}

void
fullscreen(void)
{
	XWindowAttributes attrs;
	Window focused;
	int revert;

	if(arrSize(workspace[CM][CW]) > 1){
		XGetInputFocus(dpy, &focused, &revert);
		XGetWindowAttributes(dpy, focused, &attrs);

		XRaiseWindow(dpy, focused);
		/* XMoveResizeWindow(dpy, focused, screens[XScreenNumberOfScreen(attrs.screen)].x_org, 
			screens[XScreenNumberOfScreen(attrs.screen)].y_org + BARH, screens[XScreenNumberOfScreen(attrs.screen)].width, 
			screens[XScreenNumberOfScreen(attrs.screen)].height - BARH); */
		XMoveResizeWindow(dpy, focused, CMX,  CMY, CMW, CMH);
	}
	else if(arrSize(workspace[CM][CW]) == 1){
		XMoveResizeWindow(dpy, workspace[CM][CW][0], CMX,  CMY, CMW, CMH);
	}
}

void
killW(int args[10])
{
	Window parent, root, focused, *children;
	int revert;
	unsigned int tmp2;


	/* kill focused window */
	if(args[0] == 0){ 
		if(arrSize(workspace[CM][CW]) > 1){
			XGetInputFocus(dpy, &focused, &revert);

			if(checkArr(workspace[CM][CW], focused, arrSize(workspace[CM][CW]))){
				XKillClient(dpy, focused);
				
				XQueryTree(dpy, DefaultRootWindow(dpy), &root, &parent, &children, &tmp2);
			}

			if(checkArr(children, workspace[CM][CW][0 ^ 1], tmp2))
				XSetInputFocus(dpy, workspace[CM][CW][0 ^ 1], RevertToNone, CurrentTime);
		}
		else if(arrSize(workspace[CM][CW]) == 1)
			XKillClient(dpy, workspace[CM][CW][0]);

		/* to protect from freezing when all windows are killed */
		if(arrSize(workspace[CM][CW]) <= 1)
			XSetInputFocus(dpy, DefaultRootWindow(dpy), RevertToParent, CurrentTime);
	}
	/* kill all windows in the current workspace[CM] */
	else if(args[0] == 1){
		int size = arrSize(workspace[CM][CW]);

		for(int i = 0; i < size; i++){
			XKillClient(dpy, workspace[CM][CW][i]);
		}
		/* If this line isn't here it will freeze when you kill all windows */
		XSetInputFocus(dpy, DefaultRootWindow(dpy), RevertToParent, CurrentTime);
	}
	/* kill all windows in all workspaces */
	else if(args[0] == 2){
		int size;
		for(int w = 0; w < WORKSPACELENGTH; w++){
			size = arrSize(workspace[CM][w]);

			for(int i = 0; i < size; i++){
				XKillClient(dpy, workspace[CM][w][i]);
				workspace[CM][w][i] = 0;
			}
		}
		/* If this line isn't here it will freeze when you kill all windows */
		XSetInputFocus(dpy, DefaultRootWindow(dpy), RevertToParent, CurrentTime);
	}
	/* exit wm */
	else if(args[0] == 3){
		XCloseDisplay(dpy);
		exit(0);
	}
}

void
toggleMode()
{
	mode ^= 1;
}

void
getKeys(XEvent ev)
{
	for(int i = 0; i < (int)LENGTH(keys); i++){
		if(ev.xkey.keycode == XKeysymToKeycode(dpy, keys[i].keysym) && ev.xkey.state == keys[i].state){
			keys[i].func(keys[i].args);
		}
	}
}

unsigned long 
color(void)
{
	unsigned int color;
	sscanf(notifWC, "#%x", &color);
	return color;
}

int
main(void)
{
	int num;
	XEvent ev;

	if(!(dpy = XOpenDisplay(NULL))) return 1;

	screen = DefaultScreen(dpy);
	screens = XineramaQueryScreens(dpy, &num);
	XRRGetMonitors(dpy, RootWindow(dpy, screen), true, &nMonitors);

	XGrabKey(dpy, XKeysymToKeycode(dpy, MODKEY), 0, DefaultRootWindow(dpy), 
		True, GrabModeAsync, GrabModeAsync);
	/* notif window */
	notifW = XCreateSimpleWindow(dpy, RootWindow(dpy, screen), 0, 0, notifWW, notifWH, 0, 
		BlackPixel(dpy, screen), color());

	while(true) {
		XNextEvent(dpy, &ev);
		XUnmapWindow(dpy, notifW);
		
		if(ev.type == KeyPress){ 
			getKeys(ev);
		}

		checkWindows();
		
		#ifdef DEBUG
		printf("workspace[0] = ");
		for(int i = 0; i < WINDOWLENGTH; i++) { printf("%ld,", workspace[CM][0][i]); }
		printf("workspace[1] = ");
		for(int i = 0; i < WINDOWLENGTH; i++) { printf("%ld,", workspace[CM][1][i]); }
		
		if(cmdMD){printf("in command mode\n"); }
		else { printf("not command mode\n"); }
		#endif
	}
	return 0;
}
