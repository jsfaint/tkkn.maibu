// Includes
#include "maibu_sdk.h"
#include "maibu_res.h"
#include <stdlib.h>
#include <time.h>

/************ Macro *************/
#define VERSION "v0.01"
#define SCREEN_WIDTH    128u
#define SCREEN_HEIGHT   128u
//plane size
#define PLANE_W         16
#define PLANE_H         16
#define PLANE_ORIGIN_X   SCREEN_WIDTH/2 - PLANE_W/2
#define PLANE_ORIGIN_Y   SCREEN_HEIGHT/2 - PLANE_H/2

#define BULLET_W        4
#define BULLET_H        4
//20 layer per window
#define BULLET_INIT     10
#define BULLET_NUM      19

#define PLANEX  g_plane.x
#define PLANEY  g_plane.y

#define BULLETX(idx)  g_bullet[idx].x
#define BULLETY(idx)  g_bullet[idx].y

/* Structure */
enum GameStatus{
    Game_Init,
    Game_Play,
    Game_Pause,
};

typedef struct{
    float x;
    float y;
} __attribute__ ((packed)) Plane;

typedef struct{
    float x;
    float y;
    float vx;
    float vy;
} __attribute__ ((packed)) Bullet;

/* Global Variables */
static int32_t g_count = 0;
static uint8_t g_bulletNum = BULLET_INIT;
static Plane g_plane;
static Bullet g_bullet[BULLET_NUM];

static int16_t old_x, old_y;

//Window/Layer ID
static int8_t g_app_window_id = -1;
static int8_t g_plane_layer_id = -1;
static int8_t g_bullet_layer_id[BULLET_NUM] = {-1};

static uint8_t gameState = Game_Init;

/* Function */
void gameInit(P_Window pwindow);
void gamePlay(date_time_t dt, uint32_t millis, void* context);
void planeInit(P_Window pwindow);
void movePlane(P_Window pwindow);
bool checkCollision();
void bulletInit(uint8_t uIndex, GBitmap *bitmap, P_Window pwindow);
void moveBullet(uint8_t *pGameState);
void bulletInitAll(P_Window pwindow);
void destructSprites(void);
void gamePauseToggle(void *context);

//Initial variables
void gameInit(P_Window pwindow)
{
    int16_t z;
    g_count = 0;
    g_bulletNum = BULLET_INIT;

    maibu_get_accel_data(&old_x, &old_y, &z);

    planeInit(pwindow);

    //FIXME: get bullet function works
    //memset(g_bullet, 0, sizeof(Bullet)*BULLET_NUM);
    //bulletInitAll(pwindow);
}

void gamePlay(date_time_t dt, uint32_t millis, void* context)
{
    P_Window pwindow = (P_Window)context;
    if (gameState == Game_Play) {
        g_count += millis;

        //FIXME
#if 0
        PA_OutputText(1, 1, 0, "TKKN DS %s", VERSION);
        PA_OutputText(1, 1, 2, "%d.%02ds", g_count/PA_RTC.FPS, g_count%PA_RTC.FPS);
        PA_OutputText(1, 1, 3, "FPS: %d", PA_RTC.FPS);
        PA_OutputText(1, 1, 5, "Bullet Num: %d", g_bulletNum);

        //FIXME
        //In the beginning of the game, bullet should be increasing in 1/10 seconds.
        if (g_count%6==0 && g_bulletNum < BULLET_NUM) {
            bulletInit(g_bulletNum);
            g_bulletNum++;
        }
#endif

        movePlane(pwindow);

        //FIXME
#if 0
        moveBullet(pGameState);

        if(checkCollision())
        {
            *pGameState = Game_Statistic;
            AS_MP3Stop();

            return;
        }
#endif
    }
}

void planeInit(P_Window pwindow)
{
    // fix position with sprites size
    g_plane.x = SCREEN_WIDTH/2;
    g_plane.y = SCREEN_HEIGHT/2;

    GRect frame = {{PLANE_ORIGIN_X, PLANE_ORIGIN_Y}, {PLANE_H, PLANE_W}};
    GBitmap bitmap_plane;
    res_get_user_bitmap(RES_BITMAP_PLANE, &bitmap_plane);

    LayerBitmap layer_bitmap = {bitmap_plane, frame, GAlignCenter};

    P_Layer layer = app_layer_create_bitmap(&layer_bitmap);
    if (layer != NULL) {
        g_plane_layer_id = app_window_add_layer(pwindow, layer);
    }
}

/*********************************************************
  movePlane(void)
  Routine Description:
  handle plane moving, control the plane by Pad or Stylus
Arguments:
void
Return Value:
None
 ***********************************************************/
void movePlane(P_Window window)
{
    int16_t x, y, z;

    maibu_get_accel_data(&x, &y, &z);

    if (x < old_x) {
        g_plane.x--;
    } else if (x > old_x){
        g_plane.x++;
    }

    if (y < old_y) {
        g_plane.y--;
    } else if (y > old_y) {
        g_plane.y++;
    }

    if (PLANEX <= 0)
        g_plane.x = 0;
    else if (PLANEX  >= (SCREEN_WIDTH-PLANE_W))
        g_plane.x = (SCREEN_WIDTH - PLANE_W);

    if (PLANEY <= 0)
        g_plane.y = 0;
    else if (PLANEY >= (SCREEN_HEIGHT-PLANE_H))
        g_plane.y = (SCREEN_HEIGHT - PLANE_H);

    //TODO: set new plane position
    P_Layer layer = app_window_get_layer_by_id(window, g_plane_layer_id);
    GRect frame;
    maibu_layer_get_bitmap_frame(layer, &frame);
    frame.origin.x = g_plane.x;
    frame.origin.y = g_plane.y;
}

//FIXME
#if 0
/********************************************
  bool checkCollision()
  Routine Description:
  check if the given bullent is collision with plane.
  Consider these sprites as circle.
Arguments:
void
Return Value:
bool, collision return true, no collision return false.
 ********************************************/
bool checkCollision()
{
    int16_t ii;
    for (ii=0; ii<g_bulletNum; ii++)
    {
        if (PA_Distance(PLANEX+8, PLANEY+8, BULLETX(ii)+4, BULLETY(ii)+4) < 8*4)
            return 1;
    }

    return 0;
}

void bulletInit(uint8_t uIndex, GBitmap *bitmap, P_Window window)
{
    uint8_t direct;
    int16_t radius;
    Bullet *pBullet = &g_bullet[uIndex];

    srand(time(0));
    direct = rand() % 4;

    switch(direct)
    {
        case 0: // y = 0
            pBullet->y = 0;
            pBullet->x = rand() % SCREEN_WIDTH;
            break;
        case 1: // x = 0
            pBullet->x = 0;
            pBullet->y = rand() % SCREEN_HEIGHT;
            break;
        case 2: // y = max
            pBullet->x = rand() % SCREEN_WIDTH;
            pBullet->y = (SCREEN_HEIGHT - BULLET_H);
            break;
        case 3: // x = max
            pBullet->x = (SCREEN_WIDTH - BULLET_W);
            pBullet->y = rand() % SCREEN_HEIGHT;
            break;
        default:
            break;
    }

    //TODO: calculate radius to get bullet vx,vy
    /*radius = PA_GetAngle(BULLETX(uIndex), BULLETY(uIndex), PLANEX, PLANEY);*/
    pBullet->vx = cos(radius);
    pBullet->vy = -sin(radius);


    if (g_bullet_layer_id[uIndex] == -1) {
        //Create bullet layer
        GRect frame_bullet = {{BULLETX(uIndex), BULLETY(uIndex)}, {BULLET_H, BULLET_W}};
        P_Layer layer = app_layer_create_bitmap(bitmap);
        if (layer != NULL) {
            g_bullet_layer_id[uIndex] = app_window_add_layer(window, layer);
        }
    } else {
        P_Layer layer = app_window_get_layer_by_id(window, g_bullet_layer_id[uIndex]);
    }
}

void bulletInitAll(P_Window pwindow)
{
    uint8_t uIndex;

    GBitmap bitmap_bullet;
    res_get_user_bitmap(RES_BITMAP_BULLET, &bitmap_bullet);

    for (uIndex=0; uIndex<g_bulletNum; uIndex++)
    {
        bulletInit(uIndex, &bitmap_bullet, pwindow);
    }
}

void moveBullet(uint8_t *pGameState)
{
    uint8_t uIndex;

    for (uIndex=0; uIndex<g_bulletNum; uIndex++)
    {
        if(BULLETX(uIndex) > SCREEN_WIDTH || BULLETX(uIndex) < 0
                || BULLETY(uIndex) > SCREEN_HEIGHT || BULLETY(uIndex) < 0) {
            //FIXME
            //bulletInit(uIndex);
        }

        g_bullet[uIndex].x += g_bullet[uIndex].vx;
        g_bullet[uIndex].y += g_bullet[uIndex].vy;
        }

        //TODO:Move bullet sprites
    }
}

void destructSprites(void)
{
    u16 uIndex;

    //TODO: delete all sprites
    for (uIndex = 0; uIndex <= g_bulletNum; uIndex++)
    {
    }

    g_count = 0;
    g_bulletNum = BULLET_INIT;
}
#endif

void gamePauseToggle(void *context)
{
    switch (gameState) {
        case Game_Play:
            gameState = Game_Pause;
            break;
        case Game_Init:
        case Game_Pause:
        default:
            gameState = Game_Play;
            break;
    }
}

P_Window init_window()
{
    P_Window pwindow = NULL;

    //Create main window
    pwindow = app_window_create();
    if (NULL == pwindow) {
        return NULL;
    }

    app_window_click_subscribe(pwindow, ButtonIdUp, gamePauseToggle);

    gameInit(pwindow);

    //1000ms, 60fps
    app_window_timer_subscribe(pwindow, 1000/60, gamePlay, pwindow);

    return pwindow;
}

// Function: main()
int main(int argc, char ** argv)
{

    P_Window pwindow = init_window();

    if (pwindow != NULL) {
        g_app_window_id = app_window_stack_push(pwindow);
    }

    return 0;
} // End of main()
