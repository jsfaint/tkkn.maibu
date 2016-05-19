#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "maibu_sdk.h"
#include "../build/maibu_res.h"

/************ Macro *************/
#define TITLE   "特训 for 麦步"
#define AUTHOR  "Jia Sui"
#define EMAIL   "jsfaint@gmail.com"
#define VERSION "0.3.2"

#define SCREEN_WIDTH    128
#define SCREEN_HEIGHT   128

#define TIMER_INTERVAL  30

//Accelerator
#define ACCER_BASE              2048
#define ACCER_THRESHOLD_HIGH    25
#define ACCER_THRESHOLD_LOW     50

//Plane
#define PLANE_W     16
#define PLANE_H     16
#define PLANE_ORIGIN_X   SCREEN_WIDTH/2 - PLANE_W/2
#define PLANE_ORIGIN_Y   SCREEN_HEIGHT/2 - PLANE_H/2

#define PLANEX  g_plane.x
#define PLANEY  g_plane.y

//Bullet
#define BULLET_W        4
#define BULLET_H        4

#define BULLET_NUM      10

#define BULLETX(pb)      (pb->x >> 8)
#define BULLETY(pb)      (pb->y >> 8)

/* Enumeration */
enum GameState {
    Game_Init,
    Game_Menu,
    Game_Play,
    Game_Pause,
    Game_Result,
    Game_About,
    Game_Quit,
};

enum PlaneStyle {
    PLANE_NORMAL,
    PLANE_LEFT,
    PLANE_RIGHT,
    PLANE_EXPLODE,
    PLANE_EXPLODE2,
};

typedef struct {
    uint8_t x;
    uint8_t y;
} Plane;

/* Structure */
typedef struct{
    int16_t x;
    int16_t y;
    int16_t vx;
    int16_t vy;
} Bullet;

/* Global Variables */
static uint8_t g_gamestate = Game_Init;
static uint32_t g_count;
static uint32_t g_old_count;

static Plane g_plane;
static Bullet g_bullet[BULLET_NUM];

//Window/Layer ID
static int32_t g_window_id;

//Game menu
static int8_t g_banner_layer_id;
static int8_t g_arrow_layer_id;
static int8_t g_menu_start_layer_id;
static int8_t g_version_layer_id;

//Game Play
static int8_t g_plane_layer_id;
static int8_t g_bullet_layer_id[BULLET_NUM];
static int8_t g_message_layer_id;

//Game restart
static int8_t g_menu_lose_layer_id;
static int8_t g_menu_score_layer_id;
static int8_t g_menu_restart_layer_id;

/* Function */
void initVariables(void);
void gameInit(P_Window pwindow, bool init);
void upPressed(void *context);
void backPressed(void *context);
void downPressed(void *context);
void selectPressed(void *context);
void timeDisplay(P_Window pwindow, uint32_t millis);
P_Layer planeCreateLayer(enum PlaneStyle style, uint8_t x, uint8_t y);
void planeInit(P_Window pwindow, uint8_t init);
void planeMove(P_Window pwindow);
P_Layer bulletCreateLayer(uint8_t x, uint8_t y);
void bulletInit(P_Window pwindow, uint8_t i);
void bulletInitAll(P_Window pwindow);
void bulletMove(P_Window pwindow);
bool checkCollision(void);
void run(date_time_t dt, uint32_t millis, void* context);
void gameMessageUpdate(P_Window pwindow, int8_t id, char *str);
uint8_t math_random(uint8_t seed, uint8_t min, uint8_t max);
uint16_t math_distance(int8_t x1, int8_t y1, int8_t x2, int8_t y2);
int16_t bullet_vx(int8_t x1, int8_t y1, int8_t x2, int8_t y2);
int16_t bullet_vy(int8_t x1, int8_t y1, int8_t x2, int8_t y2);
void gameCounterReset(void);
char* gameCounterGet(char *str);
void gameCounterUpdate(uint32_t millis);
void planeExplode(P_Window pwindow);
P_Layer textOut(char *str, uint8_t x, uint8_t y, uint8_t height, uint8_t width, enum GAlign alignment, uint8_t type);
P_Layer bmpOut(uint8_t x, uint8_t y, uint8_t height, uint8_t wdith, uint16_t res);
void gameLayer(P_Window pwindow, int8_t *layer_id, P_Layer layer);
void gameResult(P_Window pwindow);
void layerVisible(P_Window pwindow, int8_t id, bool status);
void gameLayerVisible(P_Window pwindow, enum GameState stat);
void gameStateSet(enum GameState stat);
enum GameState gameStateGet(void);

//Init global variables
void initVariables(void)
{
    //window
    g_window_id = -1;

    //Menu
    g_banner_layer_id = -1;
    g_arrow_layer_id = -1;
    g_menu_start_layer_id = -1;
    g_version_layer_id = -1;

    //Play
    g_plane_layer_id = -1;
    memset(g_bullet_layer_id, -1, sizeof(g_bullet_layer_id));
    g_message_layer_id = -1;

    //Result
    g_menu_lose_layer_id = -1;
    g_menu_score_layer_id = -1;
    g_menu_restart_layer_id = -1;
}

//Initial variables
void gameInit(P_Window pwindow, bool init)
{
    if (NULL == pwindow) {
        return;
    }

    gameCounterReset();

    planeInit(pwindow, init);

    bulletInitAll(pwindow);
}


void backPressed(void *context)
{
    P_Window pwindow = (P_Window)context;

    enum GameState stat = gameStateGet();

    if (stat == Game_About) {
        gameStateSet(Game_Menu);
        gameLayerVisible(pwindow, Game_Menu);
    } else if (stat == Game_Play) {
        gameStateSet(Game_Menu);
        gameLayerVisible(pwindow, Game_Menu);
    } else if (stat == Game_Result) {
        gameStateSet(Game_Menu);
        gameLayerVisible(pwindow, Game_Menu);
    } else if (stat == Game_Pause) {
        //NOTE: do nothing.
    } else {
        //Quit game
        app_window_stack_pop(pwindow);
        gameInit(pwindow, false);
    }
}

void upPressed(void *context)
{
    P_Window pwindow = (P_Window)context;

    enum GameState stat = gameStateGet();
    int i;

    if (stat == Game_Play) {
        gameStateSet(Game_Pause);
        gameMessageUpdate(pwindow, g_message_layer_id, "pause");
    } else if (stat == Game_Pause) {
        gameStateSet(Game_Play);
    } else if (stat == Game_Menu) {
        //TODO: menu select up
    }
}


void downPressed(void *context)
{
    P_Window pwindow = (P_Window)context;

    //TODO: menu select down
}

void selectPressed(void *context)
{
    P_Window pwindow = (P_Window)context;

    enum GameState stat = gameStateGet();

    if (stat == Game_Menu) {
        gameLayerVisible(pwindow, Game_Play);
        gameInit(pwindow, false);
        gameStateSet(Game_Play);
    } else if (stat == Game_Result) {
        gameLayerVisible(pwindow, Game_Play);
        gameInit(pwindow, false);
        gameStateSet(Game_Play);
    }
}

void timeDisplay(P_Window pwindow, uint32_t millis)
{
    char str[40];

    gameCounterGet(str);
    sprintf(str, "%ss", str);
    gameMessageUpdate(pwindow, g_message_layer_id, str);
}

P_Layer planeCreateLayer(enum PlaneStyle style, uint8_t x, uint8_t y)
{
    uint16_t res;

    if (style == PLANE_NORMAL) {
        res = RES_BITMAP_PLANE;
    } else if (style == PLANE_LEFT) {
        res = RES_BITMAP_PLANE_LEFT;
    } else if (style == PLANE_RIGHT) {
        res = RES_BITMAP_PLANE_RIGHT;
    } else if (style == PLANE_EXPLODE) {
        res = RES_BITMAP_PLANE_EXPLODE;
    } else if (style == PLANE_EXPLODE2) {
        res = RES_BITMAP_PLANE_EXPLODE2;
    }

    return bmpOut(x, y, PLANE_H, PLANE_W, res);
}

void planeInit(P_Window pwindow, uint8_t init)
{
    PLANEX = PLANE_ORIGIN_X;
    PLANEY = PLANE_ORIGIN_Y;

    if (init) {
        P_Layer layer = planeCreateLayer(PLANE_NORMAL, PLANEX, PLANEY);
        if (layer != NULL) {
            g_plane_layer_id = app_window_add_layer(pwindow, layer);
        }
    }
}

void planeMove(P_Window pwindow)
{
    int16_t x, y, z;
    const uint8_t step = 2;
    enum PlaneStyle style;

    //Calculate plane position
    maibu_get_accel_data(&x, &y, &z);

    if (y >= (ACCER_BASE + ACCER_THRESHOLD_HIGH)) {
        PLANEX -= step;
        style = PLANE_LEFT;
    } else if (y <= (ACCER_BASE - ACCER_THRESHOLD_LOW)) {
        PLANEX += step;
        style = PLANE_RIGHT;
    } else {
        style = PLANE_NORMAL;
    }

    if (x >= (ACCER_BASE + ACCER_THRESHOLD_HIGH)) {
        PLANEY -= step;
    } else if (x <= (ACCER_BASE - ACCER_THRESHOLD_LOW)) {
        PLANEY += step;
    }

    if (PLANEX <= step)
        PLANEX = step;
    else if (PLANEX  >= (SCREEN_WIDTH-PLANE_W-step))
        PLANEX = (SCREEN_WIDTH - PLANE_W - step);

    if (PLANEY <= step)
        PLANEY = step;
    else if (PLANEY >= (SCREEN_HEIGHT-PLANE_H-step))
        PLANEY = (SCREEN_HEIGHT - PLANE_H - step);

    P_Layer layer = planeCreateLayer(style, PLANEX, PLANEY);
    gameLayer(pwindow, &g_plane_layer_id, layer);
}

P_Layer bulletCreateLayer(uint8_t x, uint8_t y)
{
    return bmpOut(x, y, BULLET_H, BULLET_W, RES_BITMAP_BULLET);
}

void bulletInit(P_Window pwindow, uint8_t i)
{
    uint8_t direct;

    Bullet *pb = &g_bullet[i];

    direct = math_random(i, 0, 4);

    if (direct == 0) {
        //y = 0
        pb->y = 0;
        pb->x = (math_random(i, 0, SCREEN_WIDTH))<<8;
    } else if (direct == 1) {
        //x = 0
        pb->x = 0;
        pb->y = (math_random(i, 0, SCREEN_HEIGHT))<<8;
    } else if (direct == 2) {
        //y = max
        pb->x = (math_random(i, 0, SCREEN_WIDTH))<<8;
        pb->y = (SCREEN_HEIGHT - BULLET_H)<<8;
    } else if (direct ==3) {
        // x = max
        pb->x = (SCREEN_WIDTH - BULLET_W)<<8;
        pb->y = (math_random(i, 0, SCREEN_HEIGHT))<<8;
    }

    pb->vx = bullet_vx(PLANEX, PLANEY, BULLETX(pb), BULLETY(pb));
    pb->vy = bullet_vy(PLANEX, PLANEY, BULLETX(pb), BULLETY(pb));

    if (g_bullet_layer_id[i] == -1) {
        P_Layer layer = bulletCreateLayer(BULLETX(pb), BULLETY(pb));
        gameLayer(pwindow, &g_bullet_layer_id[i], layer);
    }
}

void bulletInitAll(P_Window pwindow)
{
    uint8_t i;

    for (i=0; i<BULLET_NUM; i++) {
        bulletInit(pwindow, i);
    }
}

void bulletMove(P_Window pwindow)
{
    uint8_t i;

    for (i=0; i<BULLET_NUM; i++) {
        Bullet *pb = &g_bullet[i];

        if(BULLETX(pb) > SCREEN_WIDTH || BULLETX(pb) < 0 || BULLETY(pb) > SCREEN_HEIGHT || BULLETY(pb) < 0) {
            bulletInit(pwindow, i);
        }

        pb->x += pb->vx;
        pb->y += pb->vy;

        P_Layer layer = bulletCreateLayer(BULLETX(pb), BULLETY(pb));
        gameLayer(pwindow, &g_bullet_layer_id[i], layer);
    }
}

bool checkCollision(void)
{
    int16_t i;
    for (i=0; i<BULLET_NUM; i++) {
        Bullet *pb = &g_bullet[i];
        if (math_distance(PLANEX+PLANE_W/2, PLANEY+PLANE_H/2, BULLETX(pb)+BULLET_W/2, BULLETY(pb)+BULLET_H/2) < (PLANE_W*BULLET_W))
            return 1;
    }

    return 0;
}

void run(date_time_t dt, uint32_t millis, void* context)
{
    P_Window pwindow = app_window_stack_get_window_by_id(g_window_id);
    if (pwindow == NULL) {
        return;
    }

    enum GameState stat = gameStateGet();

    gameCounterUpdate(millis);

    if (stat == Game_Play) {
        timeDisplay(pwindow, millis);

        planeMove(pwindow);
        bulletMove(pwindow);

        //Check collision
        if (checkCollision()) {
            maibu_service_vibes_pulse(VibesPulseTypeShort, 0);
            gameResult(pwindow);
        }
    }

    app_window_update(pwindow);
}

void gameMessageUpdate(P_Window pwindow, int8_t id, char *str)
{
    if (pwindow == NULL) {
        return;
    }

    if (id == -1) {
        return;
    }

    P_Layer layer = app_window_get_layer_by_id(pwindow, id);
    if (NULL == layer) {
        maibu_service_vibes_pulse(VibesPulseTypeLong, 0);
        return;
    }

    app_layer_set_text_text(layer, str);
}

uint8_t math_random(uint8_t seed, uint8_t min, uint8_t max)
{
    uint8_t num;
    struct date_time t;

    app_service_get_datetime(&t);

    num = (uint8_t)((t.wday * t.hour * t.sec + t.min + seed * t.sec) & 0xff);
    num = (max - min) * num / 256 + min;

    return num;
}

uint16_t math_distance(int8_t x1, int8_t y1, int8_t x2, int8_t y2)
{
    int16_t h = x1 - x2;
    int16_t v = y1 - y2;
    return (h*h + v*v);
}

int16_t bullet_vx(int8_t x1, int8_t y1, int8_t x2, int8_t y2)
{
    int16_t r = math_distance(x1, y1, x2, y2);
    int16_t a = x1 - x2;
    uint16_t vx;

    if (r == 0) {
        vx = 256;
    } else {
        vx = (a*a)*256/r;
    }

    vx *= 2;

    if (a < 0) {
        return -vx;
    } else {
        return vx;
    }
}

int16_t bullet_vy(int8_t x1, int8_t y1, int8_t x2, int8_t y2)
{
    int16_t r = math_distance(x1, y1, x2, y2);
    int16_t a = y1 - y2;
    uint16_t vy;

    if (r == 0) {
        vy = 256;
    } else {
        vy = (a*a)*256/r;
    }

    vy *= 2;

    if (a < 0) {
        return -vy;
    } else {
        return vy;
    }
}

void gameCounterReset(void)
{
    g_count = 0;
    g_old_count = 0;
}

char* gameCounterGet(char *str)
{
    if (str == NULL) {
        return NULL;
    }

    sprintf(str, "%d.%02d", g_count/1000, g_count%1000/10);

    return str;
}

void gameCounterUpdate(uint32_t millis)
{
    if (g_old_count == 0) {
        g_old_count = millis;
        return;
    }

    if (gameStateGet() == Game_Play) {
        g_count += (millis - g_old_count);
    }

    g_old_count = millis;
}

void planeExplode(P_Window pwindow)
{
    P_Layer layer;
    layer = planeCreateLayer(PLANE_EXPLODE, PLANEX, PLANEY);
    gameLayer(pwindow, &g_plane_layer_id, layer);

    maibu_service_vibes_pulse(VibesPulseTypeShort, 0);
}

P_Layer textOut(char *str, uint8_t x, uint8_t y, uint8_t height, uint8_t width, enum GAlign alignment, uint8_t type)
{
    GRect frame = {{x, y}, {height, width}};
    LayerText lt = {str, frame, alignment, type, 0};
    P_Layer layer = app_layer_create_text(&lt);

    return layer;
}

P_Layer bmpOut(uint8_t x, uint8_t y, uint8_t height, uint8_t wdith, uint16_t res)
{
    GRect frame = {{x, y}, {height, wdith}};
    GBitmap bitmap;

    res_get_user_bitmap(res, &bitmap);

    LayerBitmap layer_bitmap = {bitmap, frame, GAlignCenter};

    P_Layer layer = app_layer_create_bitmap(&layer_bitmap);

    return layer;
}

void gameLayer(P_Window pwindow, int8_t *layer_id, P_Layer layer)
{
    if (pwindow == NULL) {
        return;
    }

    if (layer == NULL) {
        return;
    }

    if (*layer_id == -1) {
        *layer_id =  app_window_add_layer(pwindow, layer);
    } else {
        P_Layer old_layer = app_window_get_layer_by_id(pwindow, *layer_id);
        app_window_replace_layer(pwindow, old_layer, layer);
    }
}

void gameLayerInit(P_Window pwindow)
{
    P_Layer layer;

    /* Menu */
    //banner
    layer = bmpOut(14, 19, 40, 100, RES_BITMAP_BANNER);
    gameLayer(pwindow, &g_banner_layer_id, layer);
    //arrow
    layer = bmpOut(40, 95, 9, 4, RES_BITMAP_ARROW);
    gameLayer(pwindow, &g_arrow_layer_id, layer);
    //start
    layer = bmpOut(49, 95, 9, 29, RES_BITMAP_START);
    gameLayer(pwindow, &g_menu_start_layer_id, layer);
    //version
    layer = textOut(VERSION, 99, 115, 12, 28, GAlignBottomRight, U_ASCII_ARIAL_12);
    gameLayer(pwindow, &g_version_layer_id, layer);

    /* Play */
    //message
    char str[40];
    sprintf(str, "%s %s", TITLE, VERSION);
    layer = textOut(str, 0, 0, 12, 128, GAlignTopLeft, U_ASCII_ARIAL_12);
    gameLayer(pwindow, &g_message_layer_id, layer);

    /* Result */
    //lose
    layer = bmpOut(32, 20, 30, 64, RES_BITMAP_LOSE);
    gameLayer(pwindow, &g_menu_lose_layer_id, layer);
    //score
    layer = textOut("", 0, 58, 14, 128, GAlignCenter, U_GBK_SIMSUN_12);
    gameLayer(pwindow, &g_menu_score_layer_id, layer);
    //restart
    layer = bmpOut(49, 95, 9, 41, RES_BITMAP_RESTART);
    gameLayer(pwindow, &g_menu_restart_layer_id, layer);
}

void gameResult(P_Window pwindow)
{
    char str[10];
    unsigned char buf[50];

    planeExplode(pwindow);

    sprintf(buf, "生存时间%s秒", gameCounterGet(str));
    P_Layer layer = app_window_get_layer_by_id(pwindow, g_menu_score_layer_id);
    if (NULL == layer) {
        return;
    }
    app_layer_set_text_text(layer, buf);

    gameLayerVisible(pwindow, Game_Result);
    gameStateSet(Game_Result);
}

void layerVisible(P_Window pwindow, int8_t id, bool status)
{
    if (pwindow == NULL) {
        return;
    }

    if (id == -1) {
        return;
    }

    P_Layer layer = app_window_get_layer_by_id(pwindow, id);
    maibu_layer_set_visible_status(layer, status);
}

void gameLayerVisible(P_Window pwindow, enum GameState stat)
{
    //Game menu
    bool banner;
    bool arrow;
    bool start;
    bool version;

    //Game play
    bool plane;
    bool bullet;
    bool message;

    //Game result
    bool lose;
    bool score;
    bool restart;

    if (stat == Game_Menu) {
        banner = true;
        arrow = true;
        start = true;
        version = true;
        plane = false;
        bullet = false;
        message = false;
        lose = false;
        score = false;
        restart = false;
    } else if (stat == Game_Play) {
        banner = false;
        arrow = false;
        start = false;
        version = false;
        plane = true;
        bullet = true;
        message = true;
        lose = false;
        score = false;
        restart = false;
    } else if (stat == Game_Pause) {
        //TODO
    } else if (stat == Game_Result) {
        banner = false;
        arrow = true;
        start = false;
        version = true;
        plane = false;
        bullet = false;
        message = false;
        lose = true;
        score = true;
        restart = true;
    } else if (stat == Game_About) {
        //TODO
    }

    //Menu
    layerVisible(pwindow, g_banner_layer_id, banner);
    layerVisible(pwindow, g_arrow_layer_id, arrow);
    layerVisible(pwindow, g_menu_start_layer_id, start);
    layerVisible(pwindow, g_version_layer_id, version);

    //Play
    layerVisible(pwindow, g_plane_layer_id, plane);
    int i;
    for (i=0; i<BULLET_NUM; i++) {
        layerVisible(pwindow, g_bullet_layer_id[i], bullet);
    }
    layerVisible(pwindow, g_message_layer_id, message);

    //Result
    layerVisible(pwindow, g_menu_lose_layer_id, lose);
    layerVisible(pwindow, g_menu_score_layer_id, score);
    layerVisible(pwindow, g_menu_restart_layer_id, restart);
}

void gameStateSet(enum GameState stat)
{
    g_gamestate = stat;
}

enum GameState gameStateGet(void)
{
    return g_gamestate;
}

// Function: main()
int main(int argc, char ** argv)
{
    P_Window pwindow = NULL;

    initVariables();

    //Create main window
    pwindow = app_window_create();
    if (NULL == pwindow) {
        return 0;
    }

    gameLayerInit(pwindow);
    gameInit(pwindow, true);

    gameLayerVisible(pwindow, Game_Menu);

    app_window_timer_subscribe(pwindow, TIMER_INTERVAL, run, NULL);

    app_window_click_subscribe(pwindow, ButtonIdUp, upPressed);
    app_window_click_subscribe(pwindow, ButtonIdBack, backPressed);
    app_window_click_subscribe(pwindow, ButtonIdDown, downPressed);
    app_window_click_subscribe(pwindow, ButtonIdSelect, selectPressed);

    g_window_id = app_window_stack_push(pwindow);

    gameStateSet(Game_Menu);

    return 0;
} // End of main()
