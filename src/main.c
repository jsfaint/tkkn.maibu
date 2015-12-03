#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "maibu_sdk.h"
#include "../build/maibu_res.h"

/************ Macro *************/
#define TITLE   "特训 for 麦步"
#define AUTHOR  "Jia Sui"
#define EMAIL   "jsfaint@gmail.com"
#define VERSION "0.1"

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
enum GameStatus {
    Game_Init,
    Game_Play,
    Game_Pause,
};

enum PlaneStyle {
    PLANE_NORMAL,
    PLANE_LEFT,
    PLANE_RIGHT,
    PLANE_EXPLODE
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
static uint8_t gameState = Game_Init;
static uint32_t g_count;
static uint32_t g_old_count;

static Plane g_plane;
static Bullet g_bullet[BULLET_NUM];

//Window/Layer ID
static int8_t g_plane_layer_id = -1;
static int8_t g_message_layer_id = -1;
static int8_t g_bullet_layer_id[BULLET_NUM];

/* Function */
void gameInit(P_Window pwindow);
void messageInit(P_Window pwindow);
void timeDisplay(P_Window pwindow, uint32_t millis);
inline P_Layer planeCreateLayer(enum PlaneStyle style, uint8_t x, uint8_t y);
void planeInit(P_Window pwindow);
void planeMove(P_Window pwindow);
P_Layer bulletCreateLayer(uint8_t x, uint8_t y);
void bulletInit(P_Window pwindow, uint8_t i);
void bulletInitAll(P_Window pwindow);
void bulletMove(P_Window pwindow);
bool checkCollision(void);
void gamePlay(date_time_t dt, uint32_t millis, void* context);
void messageUpdate(P_Window pwindow, char *str);
void gamePauseToggle(void *context);
void gameQuit(void *context);
uint8_t math_random(uint8_t seed);
uint16_t math_distance(int8_t x1, int8_t y1, int8_t x2, int8_t y2);
int16_t bullet_vx(int8_t x1, int8_t y1, int8_t x2, int8_t y2);
int16_t bullet_vy(int8_t x1, int8_t y1, int8_t x2, int8_t y2);
void gameCounterReset(void);
void gameCounterGet(uint32_t *sec, uint32_t *ms);
void gameCounterUpdate(uint32_t millis);

//Initial variables
void gameInit(P_Window pwindow)
{
    if (NULL == pwindow) {
        return;
    }

    gameCounterReset();

    messageInit(pwindow);

    planeInit(pwindow);

    bulletInitAll(pwindow);
}

void messageInit(P_Window pwindow)
{
    char str[40];

    sprintf(str, "%s %s", TITLE, VERSION);

    GRect frame = {{0, 0}, {12, 128}};
    LayerText lt = {str, frame, GAlignTopLeft, U_ASCII_ARIAL_12, 0};
    P_Layer layer = app_layer_create_text(&lt);

    if (layer != NULL) {
        g_message_layer_id = app_window_add_layer(pwindow, layer);
    }
}

void timeDisplay(P_Window pwindow, uint32_t millis)
{
    char str[40];
    uint32_t sec, ms;

    gameCounterGet(&sec, &ms);

    sprintf(str, "%d.%02ds", sec, ms);

    messageUpdate(pwindow, str);
}

P_Layer planeCreateLayer(enum PlaneStyle style, uint8_t x, uint8_t y)
{
    GRect frame = {{x, y}, {PLANE_H, PLANE_W}};

    GBitmap bitmap_plane;

    switch (style) {
        case PLANE_LEFT:
            res_get_user_bitmap(RES_BITMAP_PLANE_LEFT, &bitmap_plane);
            break;
        case PLANE_RIGHT:
            res_get_user_bitmap(RES_BITMAP_PLANE_RIGHT, &bitmap_plane);
            break;
        case PLANE_EXPLODE:
            res_get_user_bitmap(RES_BITMAP_PLANE_EXPLODE, &bitmap_plane);
        case PLANE_NORMAL:
        default:
            res_get_user_bitmap(RES_BITMAP_PLANE, &bitmap_plane);
            break;
    }

    LayerBitmap layer_bitmap = {bitmap_plane, frame, GAlignCenter};

    P_Layer layer = app_layer_create_bitmap(&layer_bitmap);

    return layer;
}

void planeInit(P_Window pwindow)
{
    PLANEX = PLANE_ORIGIN_X;
    PLANEY = PLANE_ORIGIN_Y;

    P_Layer layer = planeCreateLayer(PLANE_NORMAL, PLANEX, PLANEY);
    if (layer != NULL) {
        g_plane_layer_id = app_window_add_layer(pwindow, layer);
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

    //Move the plane to new position
    P_Layer old_layer = app_window_get_layer_by_id(pwindow, g_plane_layer_id);

    P_Layer layer = planeCreateLayer(style, PLANEX, PLANEY);
    if (layer != NULL) {
        app_window_replace_layer(pwindow, old_layer, layer);
    }
}

P_Layer bulletCreateLayer(uint8_t x, uint8_t y)
{
    GRect frame = {{x, y}, {BULLET_H, BULLET_W}};

    GBitmap bitmap;
    res_get_user_bitmap(RES_BITMAP_BULLET, &bitmap);

    LayerBitmap layer_bitmap = {bitmap, frame, GAlignCenter};

    P_Layer layer = app_layer_create_bitmap(&layer_bitmap);

    return layer;
}

void bulletInit(P_Window pwindow, uint8_t i)
{
    uint8_t direct;
    int16_t radius;

    Bullet *pb = &g_bullet[i];

    direct = math_random(i) % 4;

    switch(direct)
    {
        case 0: // y = 0
            pb->y = 0;
            pb->x = (math_random(i) % SCREEN_WIDTH)<<8;
            break;
        case 1: // x = 0
            pb->x = 0;
            pb->y = (math_random(i) % SCREEN_HEIGHT)<<8;
            break;
        case 2: // y = max
            pb->x = (math_random(i) % SCREEN_WIDTH)<<8;
            pb->y = (SCREEN_HEIGHT - BULLET_H)<<8;
            break;
        case 3: // x = max
            pb->x = (SCREEN_WIDTH - BULLET_W)<<8;
            pb->y = (math_random(i) % SCREEN_HEIGHT)<<8;
            break;
        default:
            break;
    }

    pb->vx = bullet_vx(PLANEX, PLANEY, BULLETX(pb), BULLETY(pb));
    pb->vy = bullet_vy(PLANEX, PLANEY, BULLETX(pb), BULLETY(pb));

    if (g_bullet_layer_id[i] == -1) {
        P_Layer layer = bulletCreateLayer(BULLETX(pb), BULLETY(pb));
        if (layer != NULL) {
            g_bullet_layer_id[i] = app_window_add_layer(pwindow, layer);
        }
    }
}

void bulletInitAll(P_Window pwindow)
{
    uint8_t i;

    memset(g_bullet_layer_id, -1, sizeof(g_bullet_layer_id));

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

        //Move the plane to new position
        P_Layer old_layer = app_window_get_layer_by_id(pwindow, g_bullet_layer_id[i]);

        P_Layer layer = bulletCreateLayer(BULLETX(pb), BULLETY(pb));
        if (layer != NULL) {
            app_window_replace_layer(pwindow, old_layer, layer);
        }
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

void gamePlay(date_time_t dt, uint32_t millis, void* context)
{
    P_Window pwindow = (P_Window)context;

    if (NULL != pwindow) {
        gameCounterUpdate(millis);

        if (gameState == Game_Play) {
            timeDisplay(pwindow, millis);

            planeMove(pwindow);
            bulletMove(pwindow);

            //Check collision
            if (checkCollision()) {
                gameCounterReset();
                //TODO: Add game result handle.
                maibu_service_vibes_pulse(VibesPulseTypeShort, 0);
            }
        }

        app_window_update(pwindow);
    }
}

void messageUpdate(P_Window pwindow, char *str)
{
    P_Layer layer = app_window_get_layer_by_id(pwindow, g_message_layer_id);
    if (NULL == layer) {
        maibu_service_vibes_pulse(VibesPulseTypeLong, 0);
        return;
    }

    app_layer_set_text_text(layer, str);
}

void gamePauseToggle(void *context)
{
    P_Window pwindow = (P_Window)context;

    char str[20] = "";

    switch (gameState) {
        case Game_Play:
            gameState = Game_Pause;

            sprintf(str, "pause");
            messageUpdate(pwindow, str);
            break;
        case Game_Init:
        case Game_Pause:
            gameState = Game_Play;
            break;
        default:
            break;
    }
}

void gameQuit(void *context)
{
    P_Window pwindow = (P_Window)context;
    app_window_stack_pop(pwindow);

    gameCounterReset();
    gameState = Game_Init;
    planeInit(pwindow);
    bulletInitAll(pwindow);
}

// Function: main()
int main(int argc, char ** argv)
{
    P_Window pwindow = NULL;

    //Create main window
    pwindow = app_window_create();
    if (NULL == pwindow) {
        return 0;
    }

    app_window_click_subscribe(pwindow, ButtonIdUp, gamePauseToggle);
    app_window_click_subscribe(pwindow, ButtonIdBack, gameQuit);

    gameInit(pwindow);

    app_window_timer_subscribe(pwindow, TIMER_INTERVAL, gamePlay, pwindow);

    app_window_stack_push(pwindow);

    return 0;
} // End of main()

uint8_t math_random(uint8_t seed)
{
    uint8_t num;
    struct date_time t;

    app_service_get_datetime(&t);

    num = (uint8_t)((t.wday * t.hour * t.sec + t.min + seed * t.sec) & 0xff);

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

void gameCounterGet(uint32_t *sec, uint32_t *ms)
{
    *sec = g_count/1000;
    *ms = g_count%1000/10;
}

void gameCounterUpdate(uint32_t millis)
{
    if (g_old_count == 0) {
        g_old_count = millis;
        return;
    }

    if (gameState == Game_Play) {
        g_count += (millis - g_old_count);
    }

    g_old_count = millis;
}
