#pragma once
#include <X11/keysym.h>

#define NONE	0
#define SHIFT	1
#define CTRL	4
#define ALT	8

void ToggleCommandMode();
void ExitCommandMode();
void SWWorkspace();
void toggleMode();
void fullscreen();
void SWWindow();
void Tile(int args[2]);
void spawn(int args[2]);
void killW(int args[2]);
void MRWindow(int args[2]);
void SWMonitor(int args[2]);

/* config */
#define WINDOWLENGTH	40		/* amount of windows   */	/* that can be open in each workspace */
#define WORKSPACELENGTH	5		 /* amount of workspace */
#define MONITORLENGTH	2		  /* amount of monitors  */

/* notif window */
#define notifWW		50		   /* notif window width */
#define notifWH		20		  /* notif window height*/
#define notifWC		"#f5f5dc"	 /* notif window color */
#define notifWP		4		/* notif window pos   */	/* top left(0), top right(1), bottom left(2), bottom right(3), middle(4) */

/* window configs */
#define RS		20		/* resize step    */		/* How much a window resizes per call */
#define MS		20		 /* move step      */		/* How much a window moves per call */
#define MW		40		  /* minimum width  */		/* minimum height of a window */
#define MH		40		   /* minimum height */		/* minimum width of a window */

/* keybinds */
/* To use more than one modifier, do MOD|MOD;
 * 	for example: SHIFT|CTRL.
 */
#define MODKEY XK_Alt_L
static Key keys[] = {
	/* Modifier,	Key,		Function,		Args */
	{ NONE,		MODKEY, 	ToggleCommandMode, 	{ } },

	{ NONE,		XK_Escape,	ExitCommandMode,	{ } },
	{ NONE,		XK_a,		spawn,			{ 1 } },
	{ NONE,		XK_space,	Tile,			{ 0 } },
	{ CTRL,		XK_h,		Tile,			{ 1 } },
	{ CTRL,		XK_v,		Tile,			{ 2 } },

	{ NONE,		XK_h,		MRWindow,		{ 1, 0 } },
	{ NONE,		XK_l,		MRWindow,		{ 1, 1 } },
	{ NONE,		XK_j,		MRWindow,		{ 1, 3 } },
	{ NONE,		XK_k,		MRWindow,		{ 1, 2 } },

	{ SHIFT,	XK_h,		MRWindow,		{ 0, 0 } },
	{ SHIFT,	XK_l,		MRWindow,		{ 0, 1 } },
	{ SHIFT,	XK_j,		MRWindow,		{ 0, 3 } },
	{ SHIFT,	XK_k,		MRWindow,		{ 0, 2 } },

	{ NONE,		XK_Return,	spawn,			{ 0 } },
	{ NONE,		XK_f,		fullscreen,		{ } },

	{ NONE,		XK_m,		toggleMode,		{ } },
	{ NONE,		XK_q,		SWWorkspace,		{ 1 } },
	{ NONE,		XK_w,		SWWorkspace,		{ 0 } },
	{ NONE,		XK_Tab,		SWWindow,		{ } },
	{ NONE,		XK_Left,	SWMonitor,		{ 0 } },
	{ NONE,		XK_Right,	SWMonitor,		{ 1 } },

	/* workspaces */
	{ NONE,		XK_1,		SWWorkspace,		{ 2, 0 } },
	{ NONE,		XK_2,		SWWorkspace,		{ 2, 1 } },
	{ NONE,		XK_3,		SWWorkspace,		{ 2, 2 } },
	{ NONE,		XK_4,		SWWorkspace,		{ 2, 3 } },
	{ NONE,		XK_5,		SWWorkspace,		{ 2, 4 } },

	{ NONE,		XK_z,		killW,			{ 0 } }, /* kill the focused window */
	{ CTRL,		XK_z,		killW,			{ 1 } }, /* kill all windows in the current Workspace*/
	{ SHIFT|CTRL,	XK_z,		killW,			{ 2 } }, /* kill all windows in all workspaces*/
	{ NONE,		XK_c,		killW,			{ 3 } }, /* exit window manager */
};
