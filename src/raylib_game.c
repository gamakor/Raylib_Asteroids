/*******************************************************************************************
*
*   raylib gamejam template
*
*   Template originally created with raylib 4.5-dev, last time updated with raylib 5.0
*
*   Template licensed under an unmodified zlib/libpng license, which is an OSI-certified,
*   BSD-like license that allows static linking with closed source software
*
*   Copyright (c) 2022-2025 Ramon Santamaria (@raysan5)
*
********************************************************************************************/

#include "raylib.h"

#if defined(PLATFORM_WEB)
    #define CUSTOM_MODAL_DIALOGS            // Force custom modal dialogs usage
    #include <emscripten/emscripten.h>      // Emscripten library - LLVM to JavaScript compiler
#endif

#include <stdio.h>                          // Required for: printf()
#include <stdlib.h>                         // Required for: 
#include <string.h>                         // Required for:

#include "raymath.h"

//----------------------------------------------------------------------------------
// Defines and Macros
//----------------------------------------------------------------------------------
// Simple log system to avoid printf() calls if required
// NOTE: Avoiding those calls, also avoids const strings memory usage
#define SUPPORT_LOG_INFO
#if defined(SUPPORT_LOG_INFO)
    #define LOG(...) printf(__VA_ARGS__)
#else
    #define LOG(...)
#endif

//----------------------------------------------------------------------------------
// Types and Structures Definition
//----------------------------------------------------------------------------------
typedef enum { 
    SCREEN_LOGO = 0, 
    SCREEN_TITLE, 
    SCREEN_GAMEPLAY, 
    SCREEN_ENDING
} GameScreen;

// TODO: Define your custom data types here
typedef struct {
    Vector2 position;
    Vector2 speed;
    float rotation;
    float acceleration;
    int type;
    bool active;

}sEntity;

typedef enum {
    TYPE_ASTEROID_SMALL = 0,
    TYPE_ASTEROID_MED,
    TYPE_ASTEROID_LARGE,
    TYPE_PLAYER,
    TYPE_SHOT
}EntityType;

#define MAX_TEXTURES 4
#define MAX_ASTEROIDS 40
#define MAX_SHOTS 10
#define MAX_SOUNDS 2
#define SPAWN_ASTEROIDS 10
#define MAX_LIVES 3

typedef enum {
    TEXTURE_METEOR_SMALL = 0,
    TEXTURE_METEOR_MED,
    TEXTURE_METEOR_LARGE,
    TEXTURE_PLAYER
};

typedef enum {
    SOUND_SHOOT =0,
    SOUND_EXPLOSION
};

Sound sounds[MAX_SOUNDS];
Texture2D textures[MAX_TEXTURES];

//----------------------------------------------------------------------------------
// Global Variables Definition
//----------------------------------------------------------------------------------
static const int screenWidth = 800;
static const int screenHeight = 450;

static RenderTexture2D target = { 0 };  // Render texture to render our game

// TODO: Define global variables here, recommended to make them static
sEntity sPlayer = { 0 };
sEntity sAsteroids[MAX_ASTEROIDS];
sEntity sShots[MAX_SHOTS];
sEntity sSuperBeam ={0};
sEntity sLives[MAX_LIVES];
int asteroidScore = 0;
int currentAsteroids = MAX_ASTEROIDS;
bool isGameOver = false;
float beamCharge = 0.0f;
float beamDelay = 1.f;
bool preDetonation = true;
int lives = 3;
float spawnInvincibility =2.f;
bool showDebug = false;

//----------------------------------------------------------------------------------
// Module Functions Declaration
//----------------------------------------------------------------------------------
static void UpdateDrawFrame(void);      // Update and Draw one frame
void GameStartup(void);
void GameUpdate(void);
void GameRender(void);
void GameShutdown(void);
void GameReset(void);
bool hasActiveAsteroids();
void CheckAsteroidType(sEntity *entity);
void PlayerDeath(void);


void GameStartUp(void) {

    InitAudioDevice();

    //Using this to set/reset game to default
    Image image1 = LoadImage("resources/player.png");
    textures[TEXTURE_PLAYER] =LoadTextureFromImage(image1);
    UnloadImage(image1);

    Image image2 = LoadImage("resources/small-a.png");
    textures[TEXTURE_METEOR_SMALL] =LoadTextureFromImage(image2);
    UnloadImage(image2);

    Image image3 = LoadImage("resources/med-b.png");
    textures[TEXTURE_METEOR_MED] =LoadTextureFromImage(image3);
    UnloadImage(image3);

    Image image4 = LoadImage("resources/big-a.png");
    textures[TEXTURE_METEOR_LARGE] =LoadTextureFromImage(image4);
    UnloadImage(image4);

    sounds[SOUND_SHOOT] = LoadSound("resources/laser.mp3");
    sounds[SOUND_EXPLOSION] = LoadSound("resources/explode.wav");


    GameReset();
}




void GameUpdate(void) {
if (!isGameOver) {
    if (IsKeyDown(KEY_A)) {
        sPlayer.rotation -= 200.f *GetFrameTime();
    }

    if (IsKeyDown(KEY_D)) {
        sPlayer.rotation +=  200.f *GetFrameTime();
    }

    sPlayer.speed.x = cosf(sPlayer.rotation*DEG2RAD) * 100.f;
    sPlayer.speed.y = sinf(sPlayer.rotation*DEG2RAD) * 100.f;

    if (IsKeyDown(KEY_W)) {
        if (sPlayer.acceleration <1.f) {
            sPlayer.acceleration += 0.04f;
        }

    }
    sPlayer.position.x += (sPlayer.speed.x * sPlayer.acceleration) * GetFrameTime();
    sPlayer.position.y += (sPlayer.speed.y * sPlayer.acceleration) * GetFrameTime();


    // Player Wrapping around the screen.
    if (sPlayer.position.x >screenWidth) {
        sPlayer.position.x = sPlayer.position.x - screenWidth;
    } else if (sPlayer.position.x <0) {
        sPlayer.position.x = screenWidth;
    }
    if (sPlayer.position.y >screenHeight) {
        sPlayer.position.y = sPlayer.position.y - screenHeight;
    }else if (sPlayer.position.y <0) {
        sPlayer.position.y = screenHeight;
    }

    //Spawn Shots
    if (IsKeyPressed(KEY_SPACE)) {
        for (int i = 0; i < MAX_SHOTS; i++) {
            if (!sShots[i].active) {
                sShots[i].active = true;
                sShots[i].position = sPlayer.position;
                sShots[i].rotation = sPlayer.rotation;
                sShots[i].acceleration = 1.f;
                sShots[i].speed.x = cosf(sPlayer.rotation*DEG2RAD) * 250.f;
                sShots[i].speed.y = sinf(sPlayer.rotation*DEG2RAD) * 250.f;

                PlaySound(sounds[SOUND_SHOOT]);

                break;
            }
        }
    }
    //
    if (IsKeyPressed(KEY_B) && !sSuperBeam.active && beamCharge >= 100.f) {
        sSuperBeam.active = true;
        sSuperBeam.position = sPlayer.position;
        sSuperBeam.rotation = sPlayer.rotation;
        sSuperBeam.acceleration = 1.f;
        sSuperBeam.speed.x = cosf(sPlayer.rotation*DEG2RAD) * 200.f;
        sSuperBeam.speed.y = sinf(sPlayer.rotation*DEG2RAD) * 200.f;
        beamCharge = 0.f;
    }

    //Tracks delay to superbeam is active and changes to active
    if (sSuperBeam.active && preDetonation) {
        beamDelay -= GetFrameTime();
    }
    if (sSuperBeam.active && beamDelay < 0) {
        preDetonation = false;
    }


    //Update SuperBeam

    sSuperBeam.position.x += (sSuperBeam.speed.x * sSuperBeam.acceleration) * GetFrameTime();
    sSuperBeam.position.y += (sSuperBeam.speed.y * sSuperBeam.acceleration) * GetFrameTime();




    //Update asteroids
    for (int i = 0; i < MAX_ASTEROIDS; i++) {
        if (sAsteroids[i].active) {
            sAsteroids[i].position.x += sAsteroids[i].speed.x * cosf(sAsteroids[i].rotation * DEG2RAD);
            sAsteroids[i].position.y += sAsteroids[i].speed.y * sinf(sAsteroids[i].rotation * DEG2RAD);

            if (sAsteroids[i].position.x > screenWidth) {
                sAsteroids[i].position.x = sAsteroids[i].position.x -screenWidth;
            } else if (sAsteroids[i].position.x <0) {
                sAsteroids[i].position.x = screenWidth;
            }

            if (sAsteroids[i].position.y >screenHeight) {
                sAsteroids[i].position.y = sAsteroids[i].position.y - screenHeight;
            }else if (sAsteroids[i].position.y <0) {
                sAsteroids[i].position.y = screenHeight;
            }
        }
    }

    //update Shots
    for (int i = 0; i < MAX_SHOTS; i++) {
        if (sShots[i].active) {
            sShots[i].position.x += (sShots[i].speed.x * sShots[i].acceleration) * GetFrameTime();
            sShots[i].position.y += (sShots[i].speed.y * sShots[i].acceleration) * GetFrameTime();

            if (sShots[i].position.x >screenWidth || sShots[i].position.x <0) {
                sShots[i].active = false;
            }

            if (sShots[i].position.y >screenHeight || sShots[i].position.y <0) {
                sShots[i].active = false;
            }

        }
    }

    //Update Live Counter.



    //Collision between shots and asteroids
    for (int i = 0; i < MAX_SHOTS; i++) {
        if (sShots[i].active ) {
            for (int j = 0; j < MAX_ASTEROIDS; j++) {
                if (sAsteroids[j].active) {
                    float texWidth = textures[sAsteroids[j].type].width;
                    if (CheckCollisionCircles(sShots[i].position,2.f,sAsteroids[j].position,texWidth/2)){
                        //Collision
                        sAsteroids[j].active = false;
                        sShots[i].active = false;
                        asteroidScore++;
                        currentAsteroids--;
                        beamCharge += 10.f;
                        CheckAsteroidType(&sAsteroids[j]);
                        PlaySound(sounds[SOUND_EXPLOSION]);
                        break;

                    }
                }
            }
        }
    }
    //Check Collison for Super Beam
    if (sSuperBeam.active && !preDetonation) {
        for (int i = 0; i < MAX_ASTEROIDS; i++) {
            if (sAsteroids[i].active) {
                float roidTexWidth = textures[sAsteroids[i].type].width;

                if (CheckCollisionCircles(sSuperBeam.position,100.f,sAsteroids[i].position,roidTexWidth/2)) {
                    sAsteroids[i].active = false;
                    asteroidScore++;
                    currentAsteroids--;
                    PlaySound(sounds[SOUND_EXPLOSION]);

                    break;
                }
            }
        }
    }

    //Chcek for collision of player with asteroids if not just spawned in
    if (spawnInvincibility <0) {

        // Need to add a switch statement for pulling radius image size are all the same so they have the same hit box
        for (int i = 0; i < MAX_ASTEROIDS; ++i) {

            if (sAsteroids[i].active) {
                float playerTexWidth = textures[sPlayer.type].width / 4; //Player divided by 4 because render texture is already divided by 4
                float roidTexWidth = textures[sAsteroids[i].type].width;

                switch (sAsteroids[i].type) {
                    case TYPE_ASTEROID_SMALL:
                        roidTexWidth = roidTexWidth/5;
                    break;
                    case TYPE_ASTEROID_MED:
                        roidTexWidth = roidTexWidth/4;
                    case TYPE_ASTEROID_LARGE:
                        roidTexWidth = roidTexWidth/3;

                    break;
                    default: ;
                }

                if (CheckCollisionCircles(sPlayer.position,playerTexWidth,sAsteroids[i].position,roidTexWidth)) {
                    PlayerDeath();
                }
            }
        }
    }

    if (beamCharge <= 100.f) {
        beamCharge += GetFrameTime();
    }

    if (spawnInvincibility> 0) {
        spawnInvincibility -= GetFrameTime();
    }
    if (hasActiveAsteroids() == false) {
        isGameOver = true;
    }
}

    if (isGameOver && IsKeyPressed(KEY_R)) {
        GameReset();
        isGameOver = false;
    }


}
void GameRender(void) {
    //draw the player
    if (spawnInvincibility>0) {
        DrawTexturePro(textures[TEXTURE_PLAYER],
                (Rectangle) { 0, 0, textures[TEXTURE_PLAYER].width, textures[TEXTURE_PLAYER].height },
                (Rectangle) { sPlayer.position.x, sPlayer.position.y, textures[TEXTURE_PLAYER].width/2, textures[TEXTURE_PLAYER].height/2 },
                (Vector2) {textures[TEXTURE_PLAYER].width/4,textures[TEXTURE_PLAYER].height/4},
                sPlayer.rotation +90,
                GRAY);

    }else {

        DrawTexturePro(textures[TEXTURE_PLAYER],
        (Rectangle) { 0, 0, textures[TEXTURE_PLAYER].width, textures[TEXTURE_PLAYER].height },
        (Rectangle) { sPlayer.position.x, sPlayer.position.y, textures[TEXTURE_PLAYER].width/2, textures[TEXTURE_PLAYER].height/2 },
        (Vector2) {textures[TEXTURE_PLAYER].width/4,textures[TEXTURE_PLAYER].height/4},
        sPlayer.rotation +90,
        RAYWHITE);

    }



    //draw basic UI

    if (showDebug) {
        DrawRectangle(5,5, 250, 100, Fade(SKYBLUE,.5f));
        DrawRectangleLines(5,5,250,100,BLUE);


        DrawText(TextFormat("- Player Rotation: (%06.1f)",sPlayer.rotation),15,45,10,YELLOW);
        DrawText(TextFormat("- Player Position: (%06.1f,%06.1f)",sPlayer.position.x,sPlayer.position.y),15,30,10,YELLOW);
        DrawText(TextFormat("- Score: (%i)",asteroidScore),15,60,10,YELLOW);
        DrawText(TextFormat("- Current Asteroids: (%i)",currentAsteroids),15,75,10,YELLOW);
        //DrawText(TextFormat("- Beam Charge : (%f)",),15,100,10,YELLOW);
    }





    //draw asteroids

    for (int i = 0; i < MAX_ASTEROIDS; i++) {
        if (sAsteroids[i].active) {
            DrawTexturePro(textures[sAsteroids[i].type],
                (Rectangle){0, 0,textures[sAsteroids[i].type].width,textures[sAsteroids[i].type].height},
                (Rectangle){sAsteroids[i].position.x,sAsteroids[i].position.y,textures[sAsteroids[i].type].width,textures[sAsteroids[i].type].height },
                (Vector2){textures[sAsteroids[i].type].width/2,textures[sAsteroids[i].type].height/2},
                sAsteroids[i].rotation,
                RAYWHITE);
        }
    }

    //draw shots
    for (int i = 0; i < MAX_SHOTS; i++) {
        if (sShots[i].active) {
            DrawCircle(sShots[i].position.x, sShots[i].position.y, 2.f, RAYWHITE);
        }
    }
    //Draw Lives UI

    for (int i = 0; i < MAX_LIVES; i++) {
        if (sLives[i].active) {
            DrawTexturePro(textures[TEXTURE_PLAYER],
        (Rectangle) { 0, 0, textures[TEXTURE_PLAYER].width, textures[TEXTURE_PLAYER].height },
        (Rectangle) { sLives[i].position.x,sLives[i].position.y, textures[TEXTURE_PLAYER].width/2, textures[TEXTURE_PLAYER].height/2 },
        (Vector2) {textures[TEXTURE_PLAYER].width/4,textures[TEXTURE_PLAYER].height/4},
        sLives[i].rotation,
        RAYWHITE);
        }
    }

    //Draws Pre active super beam
    if (sSuperBeam.active && preDetonation) {
        DrawCircle(sSuperBeam.position.x, sSuperBeam.position.y,10.f, RAYWHITE);
    }

    //Draws active  Beam
    if (sSuperBeam.active && !preDetonation) {
        DrawCircle(sSuperBeam.position.x, sSuperBeam.position.y, 100.f, RAYWHITE);
        //DrawRectanglePro((Rectangle){sPlayer.position.x, sPlayer.position.y,250,400},(Vector2){250/2,0},sPlayer.rotation-90,RAYWHITE);
    }

    //Draw BeamCharge Bar
    float beamChargeNorm = Normalize(beamCharge,0.f,100.f);
    if (beamChargeNorm < 1.f) {
        DrawRectangle(400,20,100*beamChargeNorm,30, YELLOW);
    } else {
        DrawRectangle(400,20,100,30, GREEN); // when fully charged turn green.
    }

    DrawRectangleLines(400,20,100,30, WHITE);

    if (isGameOver) {
        DrawText(TextFormat("Game Over"),screenWidth/2-50,screenHeight/2 ,30,YELLOW);
        DrawText(TextFormat("Press R to Restart"),screenWidth/2 - 50,screenHeight/2 + 30 ,20,YELLOW);

    }
}
void GameShutdown(void) {
    for (int i = 0; i < MAX_TEXTURES; i++) {
        UnloadTexture(textures[i]);
    }


    for (int i = 0; i < MAX_SOUNDS; i++) {
        UnloadSound(sounds[i]);
    }
    CloseAudioDevice();
}
void GameReset(void) {
    sPlayer.position = (Vector2) { screenWidth/2, screenHeight/2};
    sPlayer.speed = (Vector2) { 0, 0};
    sPlayer.rotation = 0;
    sPlayer.acceleration = 0;
    sPlayer.type = TYPE_PLAYER;
    asteroidScore = 0;
    currentAsteroids = MAX_ASTEROIDS;
    beamDelay = 1.f;
    preDetonation = true;
    lives = MAX_LIVES;
    beamCharge = 0.f;

    float livesUIX = 30;

    for (int i = 0; i < lives; i++) {
        sLives[i].active = true;
        sLives[i].position.x = livesUIX;
        sLives[i].position.y = 50;
        livesUIX+= 40;
        sLives[i].type = TYPE_PLAYER;

    }


    for (int i = 0; i < SPAWN_ASTEROIDS; i++) {
        sAsteroids[i].rotation = (float)GetRandomValue(0, 360);
        sAsteroids[i].position = (Vector2) {GetRandomValue(0,screenWidth), GetRandomValue(0,screenHeight)};
        sAsteroids[i].type = GetRandomValue(TYPE_ASTEROID_SMALL,TYPE_ASTEROID_LARGE);
        sAsteroids[i].speed = (Vector2) { (float)GetRandomValue(1,2),(float)GetRandomValue(1,2)};
        sAsteroids[i].active = true;
    }

    for (int i = 0; i < MAX_SHOTS; i++) {
        sShots[i].active = false;
    }

    sSuperBeam.active = false;

}

bool hasActiveAsteroids() {
    for (int    i = 0;    i < MAX_ASTEROIDS; i++) {
        if (sAsteroids[i].active) {
            return true;
        }
    }
    return false;
}

void CheckAsteroidType(sEntity *entity)
 {
    //take asteroid check type and spawn new small asteroid if passed asteroid is bigge than small
    // spawn asteroids from the player in random directions .
;
        // check type of collision
    if (entity->type == TYPE_ASTEROID_MED ) {
        int asteroidsSpawned = 0;
        //Seach for non active asteroid and spawn on entity death
        for (int i = 0; i < MAX_ASTEROIDS; i++) {
            if (!sAsteroids[i].active) {
                sAsteroids[i].rotation = (float)GetRandomValue(0, 360);
                sAsteroids[i].position = entity->position;
                sAsteroids[i].type = TYPE_ASTEROID_SMALL;
                sAsteroids[i].speed = (Vector2) { (float)GetRandomValue(1,2),(float)GetRandomValue(1,2)};
                sAsteroids[i].active = true;

                asteroidsSpawned++;
                //Check if enough child Asteroids have spawned
                if (asteroidsSpawned == 1 ) {
                    break;
                }
            }
        }

    }

    if (entity->type == TYPE_ASTEROID_LARGE ) {
        int asteroidsSpawned = 0;
        for (int i = 0; i < MAX_ASTEROIDS; i++) {
            if (!sAsteroids[i].active) {
                sAsteroids[i].rotation = (float)GetRandomValue(0, 360);
                sAsteroids[i].position = entity->position;
                sAsteroids[i].type = TYPE_ASTEROID_SMALL;
                sAsteroids[i].speed = (Vector2) { (float)GetRandomValue(1,2),(float)GetRandomValue(1,2)};
                sAsteroids[i].active = true;

                asteroidsSpawned++;

                if (asteroidsSpawned == 2 ) {
                    break;
                }
            }
        }

    }
}

void PlayerDeath(void) {
    lives--;
    sLives[lives].active = false;

    if (lives <= 0) {
        isGameOver = true;

    } else {
        sPlayer.position = (Vector2) { screenWidth/2, screenHeight/2};
        sPlayer.speed = (Vector2) { 0, 0};
        sPlayer.rotation = 0;
        sPlayer.acceleration = 0;
        sPlayer.type = TYPE_PLAYER;
        spawnInvincibility = 2.f;
    }



}

//------------------------------------------------------------------------------------
// Program main entry point
//------------------------------------------------------------------------------------
int main(void)
{
#if !defined(_DEBUG)
    SetTraceLogLevel(LOG_NONE);         // Disable raylib trace log messages
#endif

    // Initialization
    //--------------------------------------------------------------------------------------
    InitWindow(screenWidth, screenHeight, "Asteroids RL");

    
    // TODO: Load resources / Initialize variables at this point
    GameStartUp();
    // Render texture to draw full screen, enables screen scaling
    // NOTE: If screen is scaled, mouse input should be scaled proportionally
    target = LoadRenderTexture(screenWidth, screenHeight);
    SetTextureFilter(target.texture, TEXTURE_FILTER_BILINEAR);

#if defined(PLATFORM_WEB)
    emscripten_set_main_loop(UpdateDrawFrame, 60, 1);
#else
    SetTargetFPS(60);     // Set our game frames-per-second
    //--------------------------------------------------------------------------------------

    // Main game loop
    while (!WindowShouldClose())    // Detect window close button
    {
        UpdateDrawFrame();
    }
#endif

    // De-Initialization
    //--------------------------------------------------------------------------------------
    UnloadRenderTexture(target);
    
    // TODO: Unload all loaded resources at this point
    GameShutdown();

    CloseWindow();        // Close window and OpenGL context
    //--------------------------------------------------------------------------------------

    return 0;
}

//--------------------------------------------------------------------------------------------
// Module functions definition
//--------------------------------------------------------------------------------------------
// Update and draw frame
void UpdateDrawFrame(void)
{
    // Update
    //----------------------------------------------------------------------------------
    // TODO: Update variables / Implement example logic at this point
    //----------------------------------------------------------------------------------

        GameUpdate();


    // Draw
    //----------------------------------------------------------------------------------
    // Render game screen to a texture, 
    // it could be useful for scaling or further shader postprocessing
    BeginTextureMode(target);
        ClearBackground(BLACK);
        
        // TODO: Draw your game screen here

        GameRender();
        //DrawText("Welcome to raylib NEXT gamejam!", 150, 140, 30, BLACK);
       // DrawRectangleLinesEx((Rectangle){ 0, 0, screenWidth, screenHeight }, 16, BLACK);
        
    EndTextureMode();
    
    // Render to screen (main framebuffer)
    BeginDrawing();
        ClearBackground(RAYWHITE);
        
        // Draw render texture to screen, scaled if required
        DrawTexturePro(target.texture, (Rectangle){ 0, 0, (float)target.texture.width, -(float)target.texture.height }, (Rectangle){ 0, 0, (float)target.texture.width, (float)target.texture.height }, (Vector2){ 0, 0 }, 0.0f, WHITE);

        // TODO: Draw everything that requires to be drawn at this point, maybe UI?

    EndDrawing();
    //----------------------------------------------------------------------------------  
}