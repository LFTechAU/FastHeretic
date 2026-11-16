// i_text.h : minimal replacements for the Watcom graph.h text calls
#ifndef __I_TEXT_H__
#define __I_TEXT_H__
void  _settextposition(short row, short col);
long  _setbkcolor(long color);
short _settextcolor(short color);
void  _outtext(char const *text);
short _settextcursor(short shape);
#endif
