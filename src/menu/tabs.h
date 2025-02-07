#ifndef __MENU_TABS_H__
#define __MENU_TABS_H__

#include <ultra64.h>
#include "../font/font.h"

struct Tab {
    char* message;
};

struct TabRenderData {
    Gfx* text;
    short width;
    short x;
};

struct Tabs {
    struct Tab* tabs;
    struct Font* font;
    short tabCount;
    short width;
    short height;
    short selectedTab;
    short x;
    short y;
    Gfx* tabOutline;
    struct TabRenderData* tabRenderData;
};

void tabsInit(struct Tabs* tabs, struct Tab* tabList, int tabCount, struct Font* font, int x, int y, int width, int height);
void tabsSetSelectedTab(struct Tabs* tabs, int index);
Gfx* tabsRenderText(struct Tabs* tabs, Gfx* dl);

#endif