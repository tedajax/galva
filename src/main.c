#include "raylib.h"

int
main(int argc, char* argv[])
{
	InitWindow(800, 600, "Test");

	SetTargetFPS(100);

	Camera3D camera = {
		.position = (Vector3){10.0f, 10.0f, 10.0f},
		.target = (Vector3){0.0f, 0.0f, 0.0f},
		.up = (Vector3){0.0f, 1.0f, 0.0f},
		.fovy = 70.0f,
		.projection = CAMERA_PERSPECTIVE,
	};

	Vector3 cubePosition = {0.0f, 0.0f, 0.0f};

	SetCameraMode(camera, CAMERA_FREE);

	while (!WindowShouldClose())
	{
		UpdateCamera(&camera);

		BeginDrawing();
		{
			ClearBackground(RAYWHITE);

			BeginMode3D(camera);
			{
				DrawCube(cubePosition, 2.0f, 2.0f, 2.0f, RED);
				DrawCubeWires(cubePosition, 2.0f, 2.0f, 2.0f, MAROON);

				DrawGrid(10, 1.0f);
			}
			EndMode3D();

			DrawText("Testing", 20, 20, 20, LIGHTGRAY);
		}
		EndDrawing();
	}

	CloseWindow();

	return 0;
}