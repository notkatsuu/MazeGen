/*******************************************************************************************
*
*   raylib maze generator
*
*   Procedural maze generator using Maze Grid Algorithm
*
*   This game has been created using raylib (www.raylib.com)
*   raylib is licensed under an unmodified zlib/libpng license (View raylib.h for details)
*
*   Copyright (c) 2024 Ramon Santamaria (@raysan5)
*
********************************************************************************************/

#include "raylib.h"
#define RAYGUI_IMPLEMENTATION
#include "raygui.h"                     // Required for immediate-mode UI elements
#include <stdlib.h>                     // Required for: malloc(), free()
#include "raymath.h"

#define MAZE_WIDTH          64
#define MAZE_HEIGHT         64
#define MAZE_DRAW_SCALE     8.0f
#define MAZE_2D_DRAW_SCALE  64.0f
#define MAZE_SPACING_ROWS   3
#define MAZE_SPACING_COLS   3

// Define different game modes
#define MODE_GAME2D     0
#define MODE_GAME3D     1
#define MODE_EDITOR     2

#define MAX_MAZE_ITEMS      16
#define MAX_TIME			120

// Declare new data type: Point
typedef struct Point {
int x;
int y;
} Point;

// Generate procedural maze image, using grid-based algorithm
// NOTE: Functions defined as static are internal to the module
static Image GenImageMaze(int width, int height, int spacingRows, int spacingCols, float skipChance);

// Get shorter path between two points, implements pathfinding algorithm: A*
static Point *LoadPathAStar(Image map, Point start, Point end, int *pointCount);

//----------------------------------------------------------------------------------
// Main entry point
//----------------------------------------------------------------------------------
int main(void)
{
// Initialization
//---------------------------------------------------------
const int screenWidth = 1280;
const int screenHeight = 720;

InitWindow(screenWidth, screenHeight, "raylib maze generator");

    bool enableMusic = true;
    InitAudioDevice();
    Music music = LoadMusicStream("resources/ambient.ogg");
    music.looping=true;
    PlayMusicStream(music);
    
    

// Current application mode
int currentMode = 2;    // 0-Game2D, 1-Game3D, 2-Editor


// Random seed defines the random numbers generation,
// always the same if using the same seed
SetRandomSeed(67218);

// Generate maze image using the grid-based generator
// DONE: [1p] Improve function to support extra configuration parameters 
Image imMaze = GenImageMaze(MAZE_WIDTH, MAZE_HEIGHT,MAZE_SPACING_ROWS,MAZE_SPACING_COLS, 0.75f);

// Load a texture to be drawn on screen from our image data
// WARNING: If imMaze pixel data is modified, texMaze needs to be re-loaded
Texture texMaze = LoadTextureFromImage(imMaze);
    

// Generate 3D mesh from image and load a 3D model from mesh 
Mesh meshMaze = GenMeshCubicmap(imMaze, (Vector3){ 1.0f, 1.0f, 1.0f });
Model mdlMaze = LoadModelFromMesh(meshMaze);
Vector3 mdlPosition = { 0.0f, 0.0f, 0.0f };  // Set model position

// Start and end cell positions (user defined)
Point startCell = { 1, 1 };
Point endCell = { imMaze.width - 2, imMaze.height -2 };

// Player current position on image-coordinates
// WARNING: It could require conversion to world coordinates!
Point playerCell = startCell;
float playerX = playerCell.x;   // Player X position for smooth movement
float playerY = playerCell.y;   // Player Y position for smooth movement
float playerSpeed = 5.0f;       // Player movement speed
float collisionRadius = 0.45f;  // Player collision from center of the cube

// Camera 2D for 2d gameplay mode
// DONE: Initialize camera parameters as required
Camera2D camera2d = { (Vector2) {screenWidth/2,screenHeight/2 },(Vector2) { playerCell.x,playerCell.y }, 0.0f, 2.0f };

// Camera 3D for first-person gameplay mode
// TODO: Initialize camera parameters as required
// NOTE: In a first-person mode, camera.position is actually the player position
// REMEMBER: We are using a different coordinates space than 2d mode
Camera3D cameraFP = { (Vector3) { playerX, 0.5f, playerY}, (Vector3) { 2.0f,0.5f,1.0f}, (Vector3) { 0.0f,1.0f,0.0f }, 90.0f,CAMERA_PERSPECTIVE};


//OrbitalCam
Camera3D cameraOrbit = { (Vector3) { 70.0f, 40.0f, 70.0f }, (Vector3) { MAZE_WIDTH/2, 0, MAZE_HEIGHT/2}, (Vector3) { 0.0f, 2.0f, 0.0f }, 45.0f, CAMERA_PERSPECTIVE };



// Mouse selected cell for maze editing
Point selectedCell = { 0 };

// Maze items position and state
Point mazeItems[MAX_MAZE_ITEMS] = { 0 };
bool mazeItemPicked[MAX_MAZE_ITEMS] = { 0 };
for(int i = 0; i < MAX_MAZE_ITEMS; i++) mazeItemPicked[i] = true;
int mazeItemsCounter = 0;
Texture texItem = LoadTexture("resources/item_atlas01.png");
    
// Player points and time
int playerPoints = 0;
float gameTime = MAX_TIME;

// Define textures to be used as our "biomes"
// DONE: Load additional textures for different biomes
Texture texBiomes[4] = { 0 };
texBiomes[0] = LoadTexture("resources/maze_atlas01.png");
texBiomes[1] = LoadTexture("resources/maze_atlas02.png");
texBiomes[2] = LoadTexture("resources/maze_atlas03.png");
texBiomes[3] = LoadTexture("resources/maze_atlas04.png");
int currentBiome = 0;

// TODO: Define all variables required for game UI elements (sprites, fonts...)
double centerX = GetScreenWidth() / 2;
double centerY = GetScreenHeight() / 2;

double posMaze2Dx = centerX / 2;
double posMaze2Dy = centerY;
Vector2 mazeOffset2D = {posMaze2Dx - texMaze.width / 2 * MAZE_DRAW_SCALE,
                        posMaze2Dy - texMaze.height / 2 * MAZE_DRAW_SCALE};

double posMaze3Dx = GetScreenWidth() - posMaze2Dx;
double posMaze3Dy = posMaze2Dy;
Vector2 mazeOffset3D = {posMaze3Dx - texMaze.width / 2 * MAZE_DRAW_SCALE,
                        posMaze3Dy - texMaze.height / 2 * MAZE_DRAW_SCALE};


//orbit target texture
RenderTexture2D targetTexture = LoadRenderTexture(MAZE_WIDTH * MAZE_DRAW_SCALE, MAZE_HEIGHT * MAZE_DRAW_SCALE);

// TODO: Define all variables required for UI editor (raygui)

//TEXT BOX TO SET THE WIDTH OF THE MAZE
static char xSpacing[4] = "3"; // Increase array size to allow larger numbers
static char ySpacing[4] = "3"; // Increase array size to allow larger numbers

bool activeXSpacing = false; // Flag for xSpacing textbox activity
bool activeYSpacing = false; // Flag for ySpacing textbox activity

// variables for pathfinding
int pointCount = 0;
Point* path = NULL;


SetTargetFPS(60);       // Set our game to run at 60 frames-per-second
//--------------------------------------------------------------------------------------

// Main game loop
while (!WindowShouldClose())    // Detect window close button or ESC key
{
    // Update
    //----------------------------------------------------------------------------------
    // Select current mode as desired

    
    UpdateMusicStream(music);
    if (IsKeyPressed(KEY_Z) && currentMode != 0)
    {
        currentMode = 0;   // Game 2D mode
        playerPoints = 0;
        playerX = playerCell.x + 0.5f;   // correct player X position
        playerY = playerCell.y + 0.5f;	 // correct player Y position
    }
    else if (IsKeyPressed(KEY_X) && currentMode != 1)
    {
        playerX = playerCell.x;  // correct player X position
        playerY = playerCell.y;	 // correct player Y position
        currentMode = 1 ;  // Game 3D mode
    }
    else if (IsKeyPressed(KEY_C) && currentMode != 2)
    {
        playerCell = startCell;
        playerX = playerCell.x;
        playerY = playerCell.y;
        currentMode = 2;  // Editor mode
    }
    if(currentMode == 0 || currentMode == 1)
    {
        gameTime -= GetFrameTime();
        if(gameTime <= 0)
        {
            currentMode = 2;
            playerCell = startCell;
            playerX = playerCell.x;
            playerY = playerCell.y;
            gameTime = MAX_TIME;
        }
    }
    else
    {
        gameTime = MAX_TIME;
    }

    switch (currentMode)
    {
    case MODE_GAME2D:     // Game 2D mode
        {
            // DONE: [2p] Player 2D movement from predefined start point (A) to end point (B)
            // Implement maze 2D player movement logic (cursors || WASD)
            // Use imMaze pixel information to check collisions
            // Detect if current playerCell == endCell to finish game
            float change = playerSpeed * GetFrameTime();
            Point checkCell[2];
            int direction[4][2] = {{0, -1}, {0, 1}, {-1, 0}, {1, 0}}; // Up, Down, Left, Right
            int keys[4] = {KEY_W, KEY_S, KEY_A, KEY_D};
            for (int i = 0; i < 4; i++) { //Itera per cada direccio
                if (IsKeyDown(keys[i])) { //Si la tecla esta d'aquella direcció
                    
                    float newPlayerX = playerX + change * direction[i][0]; //Calcula la nova posició del jugador
                    float newPlayerY = playerY + change * direction[i][1]; 

                    if (direction[i][0] != 0) { // Moving horizontally
                        //Calcula les dues cel·les que tocarà el jugador
                        checkCell[0] = (Point){(int)(newPlayerX + direction[i][0] * collisionRadius), (int)(newPlayerY - collisionRadius)}; 
                        checkCell[1] = (Point){(int)(newPlayerX + direction[i][0] * collisionRadius), (int)(newPlayerY + collisionRadius)};
                    } else { // Moving vertically
                        checkCell[0] = (Point){(int)(newPlayerX - collisionRadius), (int)(newPlayerY + direction[i][1] * collisionRadius)};
                        checkCell[1] = (Point){(int)(newPlayerX + collisionRadius), (int)(newPlayerY + direction[i][1] * collisionRadius)};
                    }
                    
                    
                    for (int j = 0; j < 2; j++) {
                        //Comprova si alguna de les dues cel·les toca una paret
                        if (ColorIsEqual(GetImageColor(imMaze, checkCell[j].x, checkCell[j].y), WHITE)) {
                            if (direction[i][0] != 0) { // Moving horizontally
                                newPlayerX = roundf(playerX*10)/10; //Arrodoneix la posició del jugador (SNAP)
                            } 
                            else { // Moving vertically
                                newPlayerY = roundf(playerY*10)/10;
                            }
                            break;
                        }
                    }
    
                    playerX = newPlayerX;
                    playerY = newPlayerY;
                }
            }

            playerCell.x = (int)round(playerX - 0.5f) ;
            playerCell.y = (int)round(playerY - 0.5f);

            if (playerCell.x == endCell.x && playerCell.y == endCell.y) {
                currentMode = MODE_EDITOR;
                playerCell = startCell;
                playerX = playerCell.x;
                playerY = playerCell.y;
                
            }

            // DONE: [2p] Camera 2D system following player movement around the map
            // Update Camera2D parameters as required to follow player and zoom control

            camera2d.target = (Vector2){ (playerX * MAZE_2D_DRAW_SCALE) ,(playerY * MAZE_2D_DRAW_SCALE) };
            float zoomAmmount = GetMouseWheelMove() * 0.1f;
            if (zoomAmmount != 0)
            {
                camera2d.zoom += zoomAmmount;
                camera2d.zoom = Clamp(camera2d.zoom, 1.0f, 4.0f);
            }

            // TODO: Maze items pickup logic

            for (int i = 0; i < MAX_MAZE_ITEMS; i++)
            {
                if (playerCell.x == mazeItems[i].x && playerCell.y == mazeItems[i].y)
                {
                    if (!mazeItemPicked[i]) playerPoints += 50;
                    mazeItemPicked[i] = true;
                    
                }
            }

        } break;
    case MODE_GAME3D:     // Game 3D mode
        {
            // DONE: [1p] Camera 3D system and �3D maze mode�
            // Implement maze 3d first-person mode -> TIP: UpdateCamera()
            // Use the imMaze map to implement collision detection, similar to 2D

            cameraFP.position.x = playerX;
            cameraFP.position.z = playerY;
            Vector3 previousPosition = cameraFP.position;
            UpdateCamera(&cameraFP, CAMERA_FIRST_PERSON);
            Vector3 direction = { cameraFP.position.x - previousPosition.x, cameraFP.position.y - previousPosition.y, cameraFP.position.z - previousPosition.z };
            bool collisionDirX = ColorIsEqual(GetImageColor(imMaze, (int)round(cameraFP.position.x+direction.x), (int)round(cameraFP.position.z)), WHITE);
            bool collisionDirZ = ColorIsEqual(GetImageColor(imMaze, (int)round(cameraFP.position.x), (int)round(cameraFP.position.z+direction.z)), WHITE);
            if(collisionDirX || collisionDirZ)
            {
                cameraFP.position = previousPosition;
            }
            else
            {
                playerX = cameraFP.position.x;
                playerY = cameraFP.position.z;
                playerCell = (Point){ (int)round(playerX), (int)round(playerY) };
                if(playerCell.x == endCell.x && playerCell.y == endCell.y)
                {
                    currentMode = MODE_EDITOR;
                    playerCell = startCell;
                    playerX = playerCell.x;
                    playerY = playerCell.y;
                }
            }

            mdlMaze.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = texBiomes[currentBiome];

            // DONE: Maze items pickup logic
            for (int i = 0; i < MAX_MAZE_ITEMS; i++)
            {
                if (playerCell.x == mazeItems[i].x && playerCell.y == mazeItems[i].y)
                {
                    if (!mazeItemPicked[i]) playerPoints += 50;
                    mazeItemPicked[i] = true;
                }
            }

        } break;
    case MODE_EDITOR:     // Editor mode
        {
            // DONE: [2p] Visual �map editor mode�. Edit image pixels with mouse.
            // Implement logic to selecte image cell from mouse position -> TIP: GetMousePosition()
            // NOTE: Mouse position is returned in screen coordinates and it has to 
            // transformed into image coordinates
            // Once the cell is selected, if mouse button pressed add/remove image pixels

            // WARNING: Remember that when imMaze changes, texMaze and mdlMaze must be also updated!
            
            // get the mouse position in screen coordinates
            Vector2 mousePos = GetMousePosition();
            
            // transform the mouse position to image coordinates
            selectedCell = (Point){ (int)((mousePos.x - mazeOffset2D.x) / MAZE_DRAW_SCALE), (int)((mousePos.y - mazeOffset2D.y) / MAZE_DRAW_SCALE) };
            bool isInBounds = selectedCell.x >= 0 && selectedCell.x < imMaze.width && selectedCell.y >= 0 && selectedCell.y < imMaze.height;
            bool isPlayerCell = playerCell.x == selectedCell.x && playerCell.y == selectedCell.y;
            
            if (isInBounds && !isPlayerCell)
            {
                bool shouldUpdate = false;
                bool isWall = ColorIsEqual(GetImageColor(imMaze, selectedCell.x, selectedCell.y), WHITE);
                int isItem = -1;
                
                for (int i = 0; i < mazeItemsCounter; i++)
                {
                    if (mazeItems[i].x == selectedCell.x && mazeItems[i].y == selectedCell.y)
                    {
                        isItem = i;
                        break;
                    }
                }

                if (IsMouseButtonDown(MOUSE_LEFT_BUTTON))
                {
                    if (isWall)
                    {
                        ImageDrawPixel(&imMaze, selectedCell.x, selectedCell.y, BLACK);
                        shouldUpdate = true;
                    }
                }

                if (IsMouseButtonDown(MOUSE_RIGHT_BUTTON))
                {
                    if (!isWall && isItem == -1)
                    {
                        ImageDrawPixel(&imMaze, selectedCell.x, selectedCell.y, WHITE);
                        shouldUpdate = true;
                    }
                }

                if (IsKeyPressed(KEY_I))
                {
                    if (isItem != -1)
                    {
                        mazeItemsCounter--;
                        if (isItem < mazeItemsCounter)
                            mazeItems[isItem] = mazeItems[mazeItemsCounter];

                        mazeItemPicked[mazeItemsCounter] = true;
                        break;
                    }
                    else if (mazeItemsCounter < MAX_MAZE_ITEMS && !isWall)
                    {
                        mazeItems[mazeItemsCounter] = selectedCell;
                        mazeItemPicked[mazeItemsCounter] = false;
                        mazeItemsCounter++;
                    }
                }

                if (shouldUpdate)
                {
                    UnloadTexture(texMaze);
                    texMaze = LoadTextureFromImage(imMaze);
                    UnloadModel(mdlMaze);
                    meshMaze = GenMeshCubicmap(imMaze, (Vector3) { 1.0f, 1.0f, 1.0f });
                    mdlMaze = LoadModelFromMesh(meshMaze);
                }
            }


            UpdateCamera(&cameraOrbit, CAMERA_ORBITAL);
            mdlMaze.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = texBiomes[currentBiome];

            // DONE: [2p] Collectible map items: player score
            // Using same mechanism than map editor, implement an items editor, registering
            // points in the map where items should be added for player pickup -> TIP: mazeItems[]



        } break;
    default: break;
    }

    // DONE: [1p] Multiple maze biomes supported
    // Implement changing between the different textures to be used as biomes
    // NOTE: For the 3d model, the current selected texture must be applied to the model material  

    if (currentMode == MODE_GAME2D || currentMode == MODE_GAME3D)
    {
    if (IsKeyPressed(KEY_ONE)) currentBiome = 0;
    else if (IsKeyPressed(KEY_TWO)) currentBiome = 1;
    else if (IsKeyPressed(KEY_THREE)) currentBiome = 2;
    else if (IsKeyPressed(KEY_FOUR)) currentBiome = 3;
    }

    // TODO: EXTRA: Calculate shorter path between startCell (or playerCell) to endCell (A* algorithm)
    // NOTE: Calculation can be costly, only do it if startCell/playerCell or endCell change

    if (IsKeyPressed(KEY_F))
    {
        path = NULL;
        pointCount = 0;
        path = LoadPathAStar(imMaze, startCell, endCell, &pointCount);
    }

    //----------------------------------------------------------------------------------

    // Draw
    //----------------------------------------------------------------------------------
    BeginDrawing();

        ClearBackground(GRAY);
       
        switch (currentMode)
        {
            case MODE_GAME2D:     // Game 2D mode
            {
                // Draw maze using camera2d (for automatic positioning and scale)
                BeginMode2D(camera2d);
                if (!IsCursorHidden())
                {
                    HideCursor();
                    DisableCursor();
                }

                    
                    //draw texture for reference
                    DrawTexture(texBiomes[currentBiome], 0, 0, WHITE);
                    // DONE: Draw maze walls and floor using current texture biome 
                    for(int i = 0; i < imMaze.width; i++)
					{
						for(int j = 0; j < imMaze.height; j++)
						{
							if(ColorIsEqual(GetImageColor(imMaze,i,j),WHITE))
							{
								DrawTexturePro(texBiomes[currentBiome], (Rectangle){ 0, texBiomes[currentBiome].height / 2, texBiomes[currentBiome].width/2, texBiomes[currentBiome].height/2 }, (Rectangle){ i* MAZE_2D_DRAW_SCALE, j* MAZE_2D_DRAW_SCALE, MAZE_2D_DRAW_SCALE, MAZE_2D_DRAW_SCALE
                                }, (Vector2){ 0, 0 }, 0, WHITE);
							}
                            else{
								DrawTexturePro(texBiomes[currentBiome], (Rectangle){ texBiomes[currentBiome].width / 2, texBiomes[currentBiome].height / 2, texBiomes[currentBiome].width / 2, texBiomes[currentBiome].height/2}, (Rectangle){ i* MAZE_2D_DRAW_SCALE, j* MAZE_2D_DRAW_SCALE, MAZE_2D_DRAW_SCALE, MAZE_2D_DRAW_SCALE
                                }, (Vector2){ 0, 0 }, 0, WHITE);
							}
						}
					}
                    // DONE: Draw player rectangle or sprite at player position
                    DrawRectangle(playerX* MAZE_2D_DRAW_SCALE - MAZE_2D_DRAW_SCALE/2, playerY* MAZE_2D_DRAW_SCALE - MAZE_2D_DRAW_SCALE/2,  MAZE_2D_DRAW_SCALE, MAZE_2D_DRAW_SCALE, BLUE);
                    
                    // DONE: Draw maze items 2d (using sprite texture?)
                    for (int i = 0; i < MAX_MAZE_ITEMS; i++)
                    {
                        if(!mazeItemPicked[i])
                            DrawTexturePro(texItem, (Rectangle) { 0, 0, texItem.width / 2, texItem.height }, (Rectangle) { mazeItems[i].x* MAZE_2D_DRAW_SCALE, mazeItems[i].y* MAZE_2D_DRAW_SCALE, MAZE_2D_DRAW_SCALE, MAZE_2D_DRAW_SCALE }, (Vector2) {0,0 }, 0.0f, WHITE);
                    }
                    
                    // TODO: EXTRA: Draw pathfinding result, shorter path from start to end
                   if(path != NULL && pointCount > 0)
                        for (int i = 0; i < pointCount; i++)
                        {
							DrawRectangle(path[i].x* MAZE_2D_DRAW_SCALE, path[i].y* MAZE_2D_DRAW_SCALE, MAZE_2D_DRAW_SCALE, MAZE_2D_DRAW_SCALE, YELLOW);
						}
					// Draw start and end cells
					DrawRectangle(startCell.x* MAZE_2D_DRAW_SCALE, startCell.y* MAZE_2D_DRAW_SCALE, MAZE_2D_DRAW_SCALE, MAZE_2D_DRAW_SCALE, GREEN);
					DrawRectangle(endCell.x* MAZE_2D_DRAW_SCALE, endCell.y* MAZE_2D_DRAW_SCALE, MAZE_2D_DRAW_SCALE, MAZE_2D_DRAW_SCALE, RED);

                    

                EndMode2D();

                // TODO: Draw game UI (score, time...) using custom sprites/fonts
                // NOTE: Game UI does not receive the camera2d transformations,
                // it is drawn in screen space coordinates directly

            } break;
            case MODE_GAME3D:     // Game 3D mode
            {
                // Draw maze using cameraFP (for first-person camera)
                BeginMode3D(cameraFP);
                if (!IsCursorHidden())
                {
                    HideCursor();
                    DisableCursor();
                }
                
                // DONE: Draw maze generated 3d model
                DrawModel(mdlMaze, mdlPosition, 1.0f, WHITE);

                // TODO: Maze items 3d draw (using 3d shape/model?) on required positions

                for (int i = 0; i < mazeItemsCounter; i++)
				{
					if (!mazeItemPicked[i])
					{
                        DrawBillboardRec(cameraFP, texItem, (Rectangle) { 0, 0, texItem.width / 2, texItem.height }, (Vector3) { mazeItems[i].x, 0.5f, mazeItems[i].y }, (Vector2) {0.5f,0.5f}, WHITE);
					}
				}
                EndMode3D();

                // TODO: Draw game UI (score, time...) using custom sprites/fonts
                // NOTE: Game UI does not receive the camera2d transformations,
                // it is drawn in screen space coordinates directly
                
            } break;
            case MODE_EDITOR:     // Editor mode
            {
                if(IsCursorHidden())
                {
                    ShowCursor();
                    EnableCursor();
                }

                // Draw generated maze texture to fill the 64*8 screen space
                DrawTextureEx(texMaze, mazeOffset2D, 0, MAZE_DRAW_SCALE, WHITE);
                // Draw lines rectangle over texture, scaled and centered on screen 
                DrawRectangleLines(mazeOffset2D.x, mazeOffset2D.y, MAZE_WIDTH*MAZE_DRAW_SCALE, MAZE_HEIGHT*MAZE_DRAW_SCALE, RED);

                // DONE: Draw player using a rectangle, consider maze screen coordinates!
                DrawRectangle(playerCell.x * MAZE_DRAW_SCALE + mazeOffset2D.x, playerCell.y * MAZE_DRAW_SCALE + mazeOffset2D.y, MAZE_DRAW_SCALE, MAZE_DRAW_SCALE, BLUE);

                //draw current mouse position only if it is inside the bounds of the maze
                if (selectedCell.x >= 0 && selectedCell.x < MAZE_WIDTH && selectedCell.y >= 0 && selectedCell.y < MAZE_HEIGHT)
                    DrawRectangle(selectedCell.x * MAZE_DRAW_SCALE + mazeOffset2D.x, selectedCell.y * MAZE_DRAW_SCALE + mazeOffset2D.y, MAZE_DRAW_SCALE, MAZE_DRAW_SCALE, GREEN);

                //Draw path if calculated
                if (path != NULL && pointCount > 0)
				{
					for (int i = 0; i < pointCount; i++)
					{
						DrawRectangle(path[i].x * MAZE_DRAW_SCALE + mazeOffset2D.x, path[i].y * MAZE_DRAW_SCALE + mazeOffset2D.y, MAZE_DRAW_SCALE, MAZE_DRAW_SCALE, YELLOW);
					}
				}  
                    
                // Draw all placed items
                for (int i = 0; i < mazeItemsCounter; i++)
                {
                    if (!mazeItemPicked[i])
                        DrawTexturePro(texItem, (Rectangle) { 0, 0, texItem.width / 2, texItem.height }, (Rectangle) { mazeItems[i].x * MAZE_DRAW_SCALE + mazeOffset2D.x, mazeItems[i].y * MAZE_DRAW_SCALE + mazeOffset2D.y, MAZE_DRAW_SCALE, MAZE_DRAW_SCALE}, (Vector2) { 0, 0 }, 0.0f, WHITE);
                    else
                        DrawTexturePro(texItem, (Rectangle) { texItem.width / 2, 0, texItem.width / 2, texItem.height }, (Rectangle) { mazeItems[i].x * MAZE_DRAW_SCALE + mazeOffset2D.x, mazeItems[i].y * MAZE_DRAW_SCALE + mazeOffset2D.y, MAZE_DRAW_SCALE, MAZE_DRAW_SCALE}, (Vector2) { 0, 0 }, 0.0f, WHITE);
                }
                
                // TODO: Draw editor UI required elements -> TIP: raygui immediate mode UI
                // NOTE: In immediate-mode UI, logic and drawing is defined together
                // REFERENCE: https://github.com/raysan5/raygui


                BeginTextureMode(targetTexture);
                ClearBackground(LIGHTGRAY);
                BeginMode3D(cameraOrbit);
                DrawModel(mdlMaze, mdlPosition, 1.0f, WHITE);
                EndMode3D();
                EndTextureMode();
                DrawTexturePro(
                    targetTexture.texture,
                    (Rectangle){0, 0, targetTexture.texture.width, -targetTexture.texture.height},
                    (Rectangle){mazeOffset3D.x, mazeOffset3D.y, MAZE_WIDTH * MAZE_DRAW_SCALE,
                                MAZE_HEIGHT * MAZE_DRAW_SCALE},
                    (Vector2){0, 0}, 0, WHITE);


                    if (GuiButton((Rectangle){mazeOffset2D.x, mazeOffset2D.y-40, 100, 40}, "Save Maze"))
                    {
                        Image imMaze = GenImageMaze(MAZE_WIDTH, MAZE_HEIGHT, MAZE_SPACING_ROWS, MAZE_SPACING_COLS, 0.75f);
                        ExportImage(imMaze, "maze.png");
                        UnloadImage(imMaze);

                        //export maze 3D
                        Mesh mesh = GenMeshCubicmap(imMaze, (Vector3){1.0f, 1.0f, 1.0f});
                        ExportMesh(mesh, "maze.obj");
                        UnloadMesh(mesh);
                    }
                    
                    DrawText("X Spacing", centerX-44, centerY-200, 18, BLACK);

                    //edit mode if clicked
                    if (GuiTextBox((Rectangle){centerX-40, centerY-170, 80, 40}, xSpacing, sizeof(xSpacing), activeXSpacing))
                    {
                        activeXSpacing = true;
                        activeYSpacing = false;
                        
                    }
                    int xSpace = atoi(xSpacing);

                    DrawText("Y Spacing", centerX-44, centerY-120, 18, BLACK);

                    if (GuiTextBox((Rectangle){centerX-40, centerY-90, 80, 40}, ySpacing, sizeof(ySpacing), activeYSpacing))
                    {
                        activeYSpacing = true;
                        activeXSpacing = false;
                    }
                    int ySpace = atoi(ySpacing);
                    DrawText("Skip Chance", centerX-56, centerY, 20, BLACK);
                    float skipChance;
                    
                    GuiSlider((Rectangle){centerX-40, centerY+30, 80, 20}, " ", NULL, &skipChance, 0.5f, 1.0f);
                    
                    if (GuiButton((Rectangle){mazeOffset2D.x + 110, mazeOffset2D.y-40, 100, 40}, "Reload Maze"))
                    {
                        UnloadImage(imMaze);
                        imMaze = GenImageMaze(64, 64,xSpace,ySpace, skipChance);
                        UnloadTexture(texMaze);
                        texMaze = LoadTextureFromImage(imMaze);
                        UnloadModel(mdlMaze);
                        meshMaze = GenMeshCubicmap(imMaze, (Vector3){1.0f, 1.0f, 1.0f});
                        mdlMaze = LoadModelFromMesh(meshMaze);
                        playerCell = startCell;
                        playerX = playerCell.x;
                        playerY = playerCell.y;
                        mazeItemsCounter = 0;
                        for(int i = 0; i < MAX_MAZE_ITEMS; i++) mazeItemPicked[i] = true;
                    }  
            } break;
            default: break;
        }

        DrawFPS(10, 10);
        char text[123];
        sprintf(text, "PlayerPoints : %d", playerPoints);
        DrawText(text, 11, 31, 20, BLACK);
        DrawText(text, 10, 30, 20, WHITE);
        sprintf(text, "Time : %.2f", gameTime);
		DrawText(text, 11, 51, 20, BLACK);
		DrawText(text, 10, 50, 20, WHITE);

    EndDrawing();
    //----------------------------------------------------------------------------------
}

// De-Initialization
//--------------------------------------------------------------------------------------
UnloadTexture(texMaze);     // Unload maze texture from VRAM (GPU)
UnloadImage(imMaze);        // Unload maze image from RAM (CPU)
UnloadTexture(texItem);     // Unload item texture from VRAM (GPU)
for (int i = 0; i < 4; i++) // Unload biomes textures from VRAM (GPU)
    UnloadTexture(texBiomes[i]); 
UnloadModel(mdlMaze);        // Unload maze model from VRAM (GPU)
UnloadMusicStream(music);         // Unload music from RAM (CPU)

CloseWindow();              // Close window and OpenGL context
//--------------------------------------------------------------------------------------

return 0;
}

// Generate procedural maze image, using grid-based algorithm
// NOTE: Black=Walkable cell, White=Wall/Block cell
static Image GenImageMaze(int width, int height, int spacingRows, int spacingCols, float skipChance)
{
// Generate image of plain color (BLACK)
Image imMaze = GenImageColor(width, height, BLACK);

// Allocate an array of point used for maze generation
// NOTE: Dynamic array allocation, memory allocated in HEAP (MAX: Available RAM)
Point *mazePoints = (Point *)malloc(MAZE_WIDTH*MAZE_HEIGHT*sizeof(Point));
int mazePointsCounter = 0;

// Start traversing image data, line by line, to paint our maze
for (int y = 0; y < imMaze.height; y++)
{
    for (int x = 0; x < imMaze.width; x++)
    {
        // Check image borders (1 px)
        if ((x == 0) || (x == (imMaze.width - 1)) || (y == 0) || (y == (imMaze.height - 1)))
        {
            ImageDrawPixel(&imMaze, x, y, WHITE);   // Image border pixels set to WHITE
        }
        else
        {
            // Check pixel module to set maze corridors width and height
            if ((x%spacingCols == 0) && (y%spacingRows == 0))
            {
                // Get change to define a point for further processing
                float chance = (float)GetRandomValue(0, 100)/100.0f;
                
                if (chance >= skipChance)
                {
                    // Set point as wall...
                    ImageDrawPixel(&imMaze, x, y, WHITE);
                    
                    // ...save point for further processing
                    mazePoints[mazePointsCounter] = (Point){ x, y };
                    mazePointsCounter++;
                }
            }
        }
    }
}

// Define an array of 4 directions for convenience
Point directions[4] = {
    { 0, -1 },      // Up
    { 0, 1 },       // Down
    { -1, 0 },      // Left
    { 1, 0 },       // Right
};

// Load a random sequence of points, to be used as indices, so,
// we can access mazePoints[] randomly indexed, instead of following the order we gor them
int *pointIndices = LoadRandomSequence(mazePointsCounter, 0, mazePointsCounter - 1);

// Process every random maze point, moving in one random direction,
// until we collision with another wall (WHITE pixel)
for (int i = 0; i < mazePointsCounter; i++)
{
    Point currentPoint = mazePoints[pointIndices[i]];
    Point currentDir = directions[GetRandomValue(0, 3)];
    currentPoint.x += currentDir.x;
    currentPoint.y += currentDir.y;
    
    // Keep incrementing wall in selected direction until a WHITE pixel is found
    // NOTE: We only check against the color.r component
    while (GetImageColor(imMaze, currentPoint.x, currentPoint.y).r != 255)
    {
        ImageDrawPixel(&imMaze, currentPoint.x, currentPoint.y, WHITE);
        
        currentPoint.x += currentDir.x;
        currentPoint.y += currentDir.y;
    }
}

UnloadRandomSequence(pointIndices);

return imMaze;
}

// TODO: EXTRA: [10p] Get shorter path between two points, implements pathfinding algorithm: A*
// NOTE: The functions returns an array of points and the pointCount
// TODO: EXTRA: [10p] Get shorter path between two points, implements pathfinding algorithm: A*
// NOTE: The functions returns an array of points and the pointCount
static Point* LoadPathAStar(Image map, Point start, Point end, int* pointCount)
{
    printf("Calculating path between points: [%i, %i] -> [%i, %i]\n", start.x, start.y, end.x, end.y);
    Point* path = NULL;
    int pathCounter = 0;

    // PathNode struct definition
    // NOTE: This is a possible useful struct but it's not a requirement
    typedef struct PathNode PathNode;
    struct PathNode {
        Point p;
        int gvalue;
        int hvalue;
        PathNode* parent;
    };

    // TODO: Implement A* algorithm logic
    // NOTE: This function must be self-contained!

    PathNode* openList = (PathNode*)malloc(map.width * map.height * sizeof(PathNode));
    PathNode* closedList = (PathNode*)malloc(map.width * map.height * sizeof(PathNode));
    if (openList == NULL || closedList == NULL)
    {
        printf("Error: Memory could not be allocated\n");
        return NULL;
    }
    int openListCounter = 0;
    int closedListCounter = 0;
    PathNode startNode = { start, 0, 0, NULL };
    openList[openListCounter++] = startNode;
    Point directions[4] = {
        { 0, -1 },      // Up
        { 0, 1 },       // Down
        { -1, 0 },      // Left
        { 1, 0 },       // Right
    };
    while (openListCounter > 0)
    {
        int lowestFI = 0;
        for (int i = 0; i < openListCounter; i++)
        {
            if (openList[i].gvalue + openList[i].hvalue < openList[lowestFI].gvalue + openList[lowestFI].hvalue)
                lowestFI = i;
        }
        closedList[closedListCounter++] = openList[lowestFI];
        if (lowestFI < openListCounter - 1)
            openList[lowestFI] = openList[openListCounter - 1];
        openListCounter--;
        if (openList[lowestFI].p.x == end.x && openList[lowestFI].p.y == end.y)
        {
            printf("Path found\n");
            PathNode* currentNode = &closedList[closedListCounter - 1];
            while (currentNode->p.x != startNode.p.x && currentNode->p.y != startNode.p.y)
            {
                pathCounter++;
                Point* temp = (Point*)realloc(path, pathCounter * sizeof(Point));
                if (temp == NULL)
                {
                    printf("Error: Memory could not be allocated\n");
                    return NULL;
                }
                path = temp;
                path[pathCounter - 1] = currentNode->p;
                printf("Path point: [%i, %i]\n", currentNode->p.x, currentNode->p.y);
                currentNode = currentNode->parent;
            }
            free(openList);
            free(closedList);
            *pointCount = pathCounter;
            return path;
        }
        for (int i = 0; i < 4; i++)
        {
            Point next = { closedList[closedListCounter - 1].p.x + directions[i].x, closedList[closedListCounter - 1].p.y + directions[i].y };
            if (next.x >= 0 && next.x < map.width && next.y >= 0 && next.y < map.height && ColorIsEqual(GetImageColor(map, next.x, next.y), BLACK))
            {
                bool inClosedList = false;
                for (int j = 0; j < closedListCounter; j++)
                {
                    if (closedList[j].p.x == next.x && closedList[j].p.y == next.y)
                    {
                        inClosedList = true;
                        break;
                    }
                }
                if (inClosedList)
                    continue;
                bool inOpenList = false;
                for (int j = 0; j < openListCounter; j++)
                {
                    if (openList[j].p.x == next.x && openList[j].p.y == next.y)
                    {
                        inOpenList = true;
                        openList[j].gvalue = closedList[closedListCounter - 1].gvalue + 1;
                        openList[j].parent = &closedList[closedListCounter - 1];
                        break;
                    }
                }
                if (!inOpenList)
                {
                    PathNode nextNode = { next, closedList[closedListCounter - 1].gvalue + 1, abs(next.x - end.x) + abs(next.y - end.y), &closedList[closedListCounter - 1] };
                    openList[openListCounter++] = nextNode;
                }
            }
        }
    }

    printf("Path not found\n");
    *pointCount = pathCounter;  // Return number of path points 
    return path;                // Return path array (dynamically allocated)
}

