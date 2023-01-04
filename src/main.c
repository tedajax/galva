#include "raylib.h"
#include "raymath.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

typedef struct Player
{
	Vector3 position;
	float yaw;
	float radius;
} Player;

typedef struct Map
{
	int width;
	int height;
	Color* data;
} Map;

void MovePlayer(Player* player, Map map, Vector2 inputMove, float inputTurn);
Vector3 GetPlayerForward(Player player);
void PlayerUpdateCamera(Player player, Camera3D* camera);

#define Sign(v) (((v) < 0) ? -1 : (((v) > 0) ? 1 : 0))

uint32_t
GetMapData(Map map, int cellX, int cellY)
{
	if (cellX < 0 || cellY < 0 || cellX >= map.width || cellY >= map.height)
	{
		return 0;
	}
	Color p = map.data[cellX + cellY * map.width];
	return *((uint32_t*)&p);
}

//------------------------------------------------------------------------------------
// Program main entry point
//------------------------------------------------------------------------------------
int
main(void)
{
	// Initialization
	//--------------------------------------------------------------------------------------
	const int screenWidth = 1280;
	const int screenHeight = 1024;

	InitWindow(screenWidth, screenHeight, "raylib [models] example - first person maze");

	// Define the camera to look into our 3d world
	// Camera camera = {.fovy = 70.0f, .projection = CAMERA_PERSPECTIVE};

	Player player = {
		.position = {16.5, 0.5, 8.5},
		.radius = 0.25f,
	};
	Camera3D camera = {.fovy = 70};

	Image imMap = LoadImage("resources/cubicmap.png"); // Load cubicmap image (RAM)
	Texture2D cubicmap = LoadTextureFromImage(imMap);  // Convert image to texture to display (VRAM)
	Mesh mesh = GenMeshCubicmap(imMap, (Vector3){1.0f, 1.0f, 1.0f});
	Model model = LoadModelFromMesh(mesh);

	// NOTE: By default each cube is mapped to one part of texture atlas
	Texture2D texture = LoadTexture("resources/cubicmap_atlas.png"); // Load map texture
	model.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = texture; // Set map diffuse texture

	// Get map image data to be used for collision detection
	Color* mapPixels = LoadImageColors(imMap);
	Map map = {
		.width = imMap.width,
		.height = imMap.height,
		.data = mapPixels,

	};
	UnloadImage(imMap); // Unload image from RAM

	uint32_t currentTileValue = 0;

	SetTargetFPS(100);
	//--------------------------------------------------------------------------------------

	// Main game loop
	while (!WindowShouldClose()) // Detect window close button or ESC key
	{
		Vector2 inputMove = {0};
		if (IsKeyDown(KEY_A)) inputMove.x -= 1.0f;
		if (IsKeyDown(KEY_D)) inputMove.x += 1.0f;
		if (IsKeyDown(KEY_W)) inputMove.y -= 1.0f;
		if (IsKeyDown(KEY_S)) inputMove.y += 1.0f;
		float inputTurn = 0.0f;
		if (IsKeyDown(KEY_LEFT)) inputTurn -= 1.0f;
		if (IsKeyDown(KEY_RIGHT)) inputTurn += 1.0f;

		MovePlayer(&player, map, inputMove, inputTurn);

		PlayerUpdateCamera(player, &camera);

		int playerCellX = (int)(player.position.x + 0.5f);
		int playerCellY = (int)(player.position.z + 0.5f);

		currentTileValue = GetMapData(map, playerCellX, playerCellY);

		BeginDrawing();

		ClearBackground(RAYWHITE);

		BeginMode3D(camera);
		DrawModel(model, (Vector3){0}, 1.0f, WHITE); // Draw maze map
		EndMode3D();

		DrawTextureEx(
			cubicmap,
			(Vector2){GetScreenWidth() - cubicmap.width * 4.0f - 20, 20.0f},
			0.0f,
			4.0f,
			WHITE);
		DrawRectangleLines(
			GetScreenWidth() - cubicmap.width * 4 - 20,
			20,
			cubicmap.width * 4,
			cubicmap.height * 4,
			GREEN);

		// Draw player position radar
		DrawRectangle(
			GetScreenWidth() - cubicmap.width * 4 - 20 + playerCellX * 4,
			20 + playerCellY * 4,
			4,
			4,
			RED);

		DrawText(
			TextFormat("%02d, %02d : 0x%08x", playerCellX, playerCellY, currentTileValue),
			10,
			20,
			20,
			GREEN);

		EndDrawing();
		//----------------------------------------------------------------------------------
	}

	// De-Initialization
	//--------------------------------------------------------------------------------------
	UnloadImageColors(mapPixels); // Unload color array

	UnloadTexture(cubicmap); // Unload cubicmap texture
	UnloadTexture(texture);  // Unload map texture
	UnloadModel(model);      // Unload map model

	CloseWindow(); // Close window and OpenGL context
	//--------------------------------------------------------------------------------------

	return 0;
}

typedef struct Cell
{
	int x, y;
} Cell;

Cell
GetMapCell(Map map, float worldX, float worldZ)
{
	return (Cell){
		.x = (int)(worldX),
		.y = (int)(worldZ),
	};
}

Cell
ClampCellToMap(Cell cell, Map map)
{
	cell.x = max(min(cell.x, map.width - 1), 0);
	cell.y = max(min(cell.y, map.height - 1), 0);
	return cell;
}

Vector2
WorldToMapSpace(Map map, Vector3 mapPosition, Vector3 worldPosition)
{
	Vector2 result;
	result.x = worldPosition.x - mapPosition.x;
	result.y = worldPosition.z - mapPosition.z;
	return result;
}

bool
CheckMapCircleCollision(Map map, Vector3 center, float radius)
{
	float minX = center.x - radius * 2 - 1;
	float maxX = center.x + radius * 2 + 1;
	float minZ = center.z - radius * 2 - 1;
	float maxZ = center.z + radius * 2 + 1;

	Cell minCell = ClampCellToMap(GetMapCell(map, minX, minZ), map);
	Cell maxCell = ClampCellToMap(GetMapCell(map, maxX, maxZ), map);

	Vector2 position = (Vector2){center.x, center.z};

	for (int cy = minCell.y; cy <= maxCell.y; ++cy)
	{
		for (int cx = minCell.x; cx <= maxCell.x; ++cx)
		{
			if (GetMapData(map, cx, cy) == 0xFFFFFFFF)
			{
				Rectangle cellRect =
					(Rectangle){.x = (float)cx, .y = (float)cy, .width = 1.0f, .height = 1.0f};
				if (CheckCollisionCircleRec(position, radius, cellRect))
				{
					return true;
				}
			}
		}
	}

	return false;
}

void
MovePlayer(Player* player, Map map, Vector2 inputMove, float inputTurn)
{
	Vector3 inputVector = {
		.x = inputMove.x,
		.y = 0,
		.z = inputMove.y,
	};

	// TODO: make not static
	const static float MOVE_SPEED = 4.0f;
	const static float TURN_RATE = DEG2RAD * 360.0f;

	player->yaw += inputTurn * TURN_RATE * GetFrameTime();

	Matrix rotMatrix = MatrixRotateY(-player->yaw);

	// Vector3 forward = GetPlayerForward(*player);
	Vector3 localMovement = Vector3Transform(inputVector, rotMatrix);
	Vector3 movement = Vector3Scale(localMovement, -MOVE_SPEED * GetFrameTime());

	const float SCAN_DIST = 0.05f;

	if (movement.x != 0)
	{
		int sign = Sign(movement.x);
		float delta = SCAN_DIST * -sign;
		Vector3 xMovement = (Vector3){movement.x};
		while (Sign(xMovement.x) == sign
		       && CheckMapCircleCollision(
				   map, Vector3Add(player->position, xMovement), player->radius))
		{
			xMovement.x += delta;
		}

		if (Sign(xMovement.x) != sign)
		{
			xMovement.x = 0;
		}
		movement.x = xMovement.x;
	}

	if (movement.z != 0)
	{
		int sign = Sign(movement.z);
		float delta = SCAN_DIST * -sign;
		Vector3 zMovement = (Vector3){0, 0, movement.z};
		while (Sign(zMovement.z) == sign
		       && CheckMapCircleCollision(
				   map, Vector3Add(player->position, zMovement), player->radius))
		{
			zMovement.z += delta;
		}

		if (Sign(zMovement.z) != sign)
		{
			zMovement.z = 0;
		}
		movement.z = zMovement.z;
	}

	player->position = Vector3Add(player->position, movement);
}

Vector3
GetPlayerForward(Player player)
{
	Vector3 forward = Vector3RotateByAxisAngle((Vector3){0, 0, 1}, (Vector3){0, 1, 0}, -player.yaw);
	return forward;
}

void
PlayerUpdateCamera(Player player, Camera3D* camera)
{
	Vector3 forward = GetPlayerForward(player);
	Vector3 target = Vector3Add(player.position, forward);
	camera->position = player.position;
	camera->target = target;
	camera->up = (Vector3){0, 1, 0};
}
