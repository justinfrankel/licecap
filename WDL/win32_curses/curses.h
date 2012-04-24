#ifndef _CURSES_WIN32SIM_H_
#define _CURSES_WIN32SIM_H_

#if !defined(_WIN32) && !defined(MAC_NATIVE) && !defined(FORCE_WIN32_CURSES)
  #ifdef MAC
  #include <ncurses.h>
  #else
  #include <curses.h>
  #endif
#else

  #ifdef _WIN32
  #include <windows.h>
  #else
  #include "../swell/swell.h"
  #endif
  #include "../queue.h"

/*
** this implements a tiny subset of curses on win32.
** It creates a window (Resizeable by user), and gives you a callback to run 
** your UI. 
*/


// if you need multiple contexts, define this in your sourcefiles BEFORE including curses.h
// if you don't need multiple contexts, declare win32CursesCtx g_curses_context; in one of your source files.
#ifndef CURSES_INSTANCE
#define CURSES_INSTANCE (&g_curses_context)
#endif

#define LINES ((CURSES_INSTANCE)->lines)
#define COLS ((CURSES_INSTANCE)->cols)

//ncurses WIN32 wrapper functions


#define addnstr(str,n) __addnstr(CURSES_INSTANCE,str,n)
#define addstr(str) __addnstr(CURSES_INSTANCE,str,-1)
#define addch(c) __addch(CURSES_INSTANCE,c)

#define mvaddstr(x,y,str) __mvaddnstr(CURSES_INSTANCE,x,y,str,-1)
#define mvaddnstr(x,y,str,n) __mvaddnstr(CURSES_INSTANCE,x,y,str,n)
#define clrtoeol() __clrtoeol(CURSES_INSTANCE)
#define move(x,y) __move(CURSES_INSTANCE, x,y, 0)
#define attrset(a) (CURSES_INSTANCE)->m_cur_attr=(a)
#define bkgdset(a) (CURSES_INSTANCE)->m_cur_erase_attr=(a)
#define initscr() __initscr(CURSES_INSTANCE)
#define endwin() __endwin(CURSES_INSTANCE)
#define curses_erase(x) __curses_erase(x)
#define start_color()
#define init_pair(x,y,z) __init_pair((CURSES_INSTANCE),x,y,z)
#define has_colors() 1

#define A_NORMAL 0
#define A_BOLD 1
#define COLOR_PAIR(x) ((x)<<NUM_ATTRBITS)
#define COLOR_PAIRS 8
#define NUM_ATTRBITS 1

typedef struct win32CursesCtx
{
  HWND m_hwnd;
  int lines, cols;

  int m_cursor_x;
  int m_cursor_y;
  char m_cur_attr, m_cur_erase_attr;
  unsigned char *m_framebuffer;
  HFONT mOurFont;
  
  bool m_need_fontcalc;
  int m_font_w, m_font_h;
  int m_need_redraw;

  WDL_Queue *m_kbq;
  int m_intimer;

  int colortab[COLOR_PAIRS << NUM_ATTRBITS][2];

  int m_cursor_state; // blinky
  int m_cursor_state_lx,m_cursor_state_ly; // used to detect changes and reset m_cursor_state

  // callbacks/config available for user
  int want_getch_runmsgpump; // set to 1 to cause getch() to run the message pump, 2 to cause it to be blocking (waiting for keychar)

  void (*ui_run)(struct win32CursesCtx *ctx);
  void *user_data;

  LRESULT (*onMouseMessage)(void *user_data, HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
} win32CursesCtx;

extern win32CursesCtx g_curses_context; // declare this if you need it

void curses_setWindowContext(HWND hwnd, win32CursesCtx *ctx);
void curses_unregisterChildClass(HINSTANCE hInstance);
void curses_registerChildClass(HINSTANCE hInstance);
HWND curses_CreateWindow(HINSTANCE hInstance, win32CursesCtx *ctx, const char *title);



void __addnstr(win32CursesCtx *inst, const char *str,int n);
void __move(win32CursesCtx *inst, int x, int y, int noupdest);
static inline void __addch(win32CursesCtx *inst, char c) { __addnstr(inst,&c,1); }
static inline void __mvaddnstr(win32CursesCtx *inst, int x, int y, const char *str, int n) { __move(inst,x,y,1); __addnstr(inst,str,n); }


void __clrtoeol(win32CursesCtx *inst);
void __initscr(win32CursesCtx *inst);
void __endwin(win32CursesCtx *inst);
void __curses_erase(win32CursesCtx *inst);

int curses_getch(win32CursesCtx *inst);

#if defined(_WIN32) || defined(MAC_NATIVE) || defined(FORCE_WIN32_CURSES)
#define getch() curses_getch(CURSES_INSTANCE)
#define erase() curses_erase(CURSES_INSTANCE)
#endif


#define wrefresh(x)
#define cbreak()
#define noecho()
#define nonl()
#define intrflush(x,y)
#define keypad(x,y)
#define nodelay(x,y)
#define raw()
#define refresh()
#define sync()



#define COLOR_WHITE RGB(192,192,192)
#define COLOR_BLACK RGB(0,0,0)
#define COLOR_BLUE  RGB(0,0,192)
#define COLOR_RED   RGB(192,0,0)
#define COLOR_CYAN  RGB(0,192,192)
#define COLOR_BLUE_DIM  RGB(0,0,56)
#define COLOR_RED_DIM    RGB(56,0,0)
#define COLOR_CYAN_DIM  RGB(0,56,56)

#define ERR -1

enum
{
  KEY_DOWN=4096,
  KEY_UP,
  KEY_PPAGE,
  KEY_NPAGE,
  KEY_RIGHT,
  KEY_LEFT,
  KEY_HOME,
  KEY_END,
  KEY_IC,
  KEY_DC,
  KEY_F1,
  KEY_F2,
  KEY_F3,
  KEY_F4,
  KEY_F5,
  KEY_F6,
  KEY_F7,
  KEY_F8,
  KEY_F9,
  KEY_F10,
  KEY_F11,
  KEY_F12,
};

#define KEY_BACKSPACE '\b'

#define KEY_F(x) (KEY_F1 + (x) - 1)


void __init_pair(win32CursesCtx *ctx, int p, int b, int f);

#endif

#endif
