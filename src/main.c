#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "maibu_sdk.h"
#include "maibu_res.h"

/************ Macro *************/
#define TITLE "特训 for 麦步"
#define AUTHOR "Jia Sui"
#define EMAIL "jsfaint@gmail.com"

#define SCREEN_WIDTH    128
#define SCREEN_HEIGHT   128

//Accelerator
#define ACCER_BASE              2048
#define ACCER_THRESHOLD_HIGH    25
#define ACCER_THRESHOLD_LOW     50

//Plane
#define PLANE_W     16
#define PLANE_H     16
#define PLANE_ORIGIN_X   SCREEN_WIDTH/2 - PLANE_W/2
#define PLANE_ORIGIN_Y   SCREEN_HEIGHT/2 - PLANE_H/2

#define PLANEX  g_plane.pos.x
#define PLANEY  g_plane.pos.y

//Bullet
#define BULLET_W        4
#define BULLET_H        4

#define BULLET_NUM      5

/* Enumeration */
enum GameStatus{
    Game_Init,
    Game_Play,
    Game_Pause,
};

typedef struct {
    GPoint pos;
} Plane;

/* Structure */
typedef struct{
    GPoint pos;
    uint8_t vx:4;
    uint8_t vy:4;
} Bullet;

/* Global Variables */
static uint8_t gameState = Game_Init;
static int32_t g_count;

static Plane g_plane;
static Bullet g_bullet[BULLET_NUM];

//Window/Layer ID
static int8_t g_plane_layer_id = -1;
static int8_t g_message_layer_id = -1;
static int8_t g_bullet_layer_id[BULLET_NUM] = {-1};

/* Function */
void gameInit(P_Window pwindow);
void messageInit(P_Window pwindow);
P_Layer planeCreateLayer(uint8_t x, uint8_t y);
void planeInit(P_Window pwindow);
void movePlane(P_Window pwindow);
P_Layer bulletCreateLayer(uint8_t x, uint8_t y);
void bulletInit(P_Window pwindow, uint8_t i);
void bulletInitAll(P_Window pwindow);
void moveBullet(P_Window pwindow);
bool checkCollision(void);
void gamePlay(date_time_t dt, uint32_t millis, void* context);
void messageUpdate(P_Window pwindow, char *str);
void gamePauseToggle(void *context);
void gameQuit(void *context);
uint8_t math_random(void);
uint16_t math_distance(int8_t x1, int8_t y1, int8_t x2, int8_t y2);

//Initial variables
void gameInit(P_Window pwindow)
{
    if (NULL == pwindow) {
        return;
    }

    messageInit(pwindow);

    planeInit(pwindow);

    //bulletInitAll(pwindow);
}

void messageInit(P_Window pwindow)
{
    g_count = 0;
    char str[] = TITLE;

    GRect frame = {{0, 0}, {12, 128}};
    LayerText lt = {str, frame, GAlignTopLeft, U_ASCII_ARIAL_12, 0};
    P_Layer layer = app_layer_create_text(&lt);

    if (layer != NULL) {
        g_message_layer_id = app_window_add_layer(pwindow, layer);
    }
}

P_Layer planeCreateLayer(uint8_t x, uint8_t y)
{
    GRect frame = {{x, y}, {PLANE_H, PLANE_W}};

    GBitmap bitmap_plane;
    res_get_user_bitmap(RES_BITMAP_PLANE, &bitmap_plane);

    LayerBitmap layer_bitmap = {bitmap_plane, frame, GAlignCenter};

    P_Layer layer = app_layer_create_bitmap(&layer_bitmap);

    return layer;
}

void planeInit(P_Window pwindow)
{
    PLANEX = (SCREEN_WIDTH - PLANE_W)/2;
    PLANEY = (SCREEN_HEIGHT - PLANE_H)/2;

    P_Layer layer = planeCreateLayer(PLANEX, PLANEY);
    if (layer != NULL) {
        g_plane_layer_id = app_window_add_layer(pwindow, layer);
    }
}

void movePlane(P_Window pwindow)
{
    int16_t x, y, z;

    //Calculate plane position
    maibu_get_accel_data(&x, &y, &z);

    if (y >= (ACCER_BASE + ACCER_THRESHOLD_HIGH)) {
        PLANEX -= 2;
    } else if (y <= (ACCER_BASE - ACCER_THRESHOLD_LOW)) {
        PLANEX += 2;
    }

    if (x >= (ACCER_BASE + ACCER_THRESHOLD_HIGH)) {
        PLANEY -= 2;
    } else if (x <= (ACCER_BASE - ACCER_THRESHOLD_LOW)) {
        PLANEY += 2;
    }

    if (PLANEX <= 0)
        PLANEX = 0;
    else if (PLANEX  >= (SCREEN_WIDTH-PLANE_W))
        PLANEX = (SCREEN_WIDTH - PLANE_W);

    if (PLANEY <= 0)
        PLANEY = 0;
    else if (PLANEY >= (SCREEN_HEIGHT-PLANE_H))
        PLANEY = (SCREEN_HEIGHT - PLANE_H);

    //Move the plane to new position
    P_Layer old_layer = app_window_get_layer_by_id(pwindow, g_plane_layer_id);

    P_Layer layer = planeCreateLayer(PLANEX, PLANEY);
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

    Bullet *pBullet = &g_bullet[i];
    P_GPoint pos = &pBullet->pos;

    direct = math_random() % 4;

    switch(direct)
    {
        case 0: // y = 0
            pos->y = 0;
            pos->x = math_random() % SCREEN_WIDTH;
            break;
        case 1: // x = 0
            pos->x = 0;
            pos->y = math_random() % SCREEN_HEIGHT;
            break;
        case 2: // y = max
            pos->x = math_random() % SCREEN_WIDTH;
            pos->y = (SCREEN_HEIGHT - BULLET_H);
            break;
        case 3: // x = max
            pos->x = (SCREEN_WIDTH - BULLET_W);
            pos->y = math_random() % SCREEN_HEIGHT;
            break;
        default:
            break;
    }

    if (pos->x > PLANEX) {
        pBullet->vx = -1;
    } else if (pos->x < PLANEX) {
        pBullet->vx = 1;
    } else {
        pBullet->vx = 0;
    }

    if (pos->y > PLANEY) {
        pBullet->vy = -1;
    } else if (pos->x < PLANEY) {
        pBullet->vy = 1;
    } else {
        pBullet->vy = 0;
    }

    if (g_bullet_layer_id[i] == -1) {
        P_Layer layer = bulletCreateLayer(pos->x, pos->y);
        if (layer != NULL) {
            g_bullet_layer_id[i] = app_window_add_layer(pwindow, layer);
        }
    }
}

void bulletInitAll(P_Window pwindow)
{
    uint8_t i;

    for (i=0; i<BULLET_NUM; i++) {
        bulletInit(pwindow, i);
    }
}

void moveBullet(P_Window pwindow)
{
    uint8_t i;

    for (i=0; i<BULLET_NUM; i++)
    {
        Bullet *pBullet = &g_bullet[i];
        P_GPoint pos = &pBullet->pos;

        if(pos->x > SCREEN_WIDTH || pos->x < 0 || pos->y > SCREEN_HEIGHT || pos->y < 0) {
            bulletInit(pwindow, i);
        }

        pos->x += pBullet->vx;
        pos->y += pBullet->vy;

        //Move the plane to new position
        P_Layer old_layer = app_window_get_layer_by_id(pwindow, g_bullet_layer_id[i]);

        P_Layer layer = bulletCreateLayer(pos->x, pos->y);
        if (layer != NULL) {
            app_window_replace_layer(pwindow, old_layer, layer);
        }
    }
}

bool checkCollision(void)
{
    int16_t i;
    for (i=0; i<BULLET_NUM; i++)
    {
        Bullet *pBullet = &g_bullet[i];
        P_GPoint pos = &pBullet->pos;
        if (math_distance(PLANEX+PLANE_W/2, PLANEY+PLANE_H/2, pos->x+BULLET_W/2, pos->y+BULLET_H/2) < (PLANE_W*BULLET_W/2))
            return 1;
    }

    return 0;
}

void gamePlay(date_time_t dt, uint32_t millis, void* context)
{
    P_Window pwindow = (P_Window)context;

    if (NULL != pwindow) {
        if (gameState == Game_Play) {
            //Increase counter
            g_count += millis;

            movePlane(pwindow);

            moveBullet(pwindow);

            //Check collision
            if (checkCollision()) {
                gameState = Game_Init;
            }
        }

        app_window_update(pwindow);
    }
}

void messageUpdate(P_Window pwindow, char *str)
{
    P_Layer layer = app_window_get_layer_by_id(pwindow, g_message_layer_id);
    if (NULL == layer) {
        maibu_service_vibes_pulse(VibesPulseTypeLong, 1);
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
            break;
        case Game_Init:
        case Game_Pause:
            gameState = Game_Play;
            sprintf(str, TITLE);
            break;
        default:
            break;
    }

    messageUpdate(pwindow, str);
}

void gameQuit(void *context)
{
    app_window_stack_pop((P_Window)context);
    gameState = Game_Init;
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

    //1000ms, 60fps
    app_window_timer_subscribe(pwindow, 20, gamePlay, pwindow);

    app_window_stack_push(pwindow);

    return 0;
} // End of main()

uint8_t math_random(void)
{
    uint8_t num;
    struct date_time t;

    app_service_get_datetime(&t);

    num = (uint8_t)((t.wday * t.hour * t.sec + t.min) & 0xff);

    return num;
}

uint16_t math_distance(int8_t x1, int8_t y1, int8_t x2, int8_t y2)
{
    int16_t h = x1 - x2;
    int16_t v = y1 - y2;
    return(h*h + v*v);
}
