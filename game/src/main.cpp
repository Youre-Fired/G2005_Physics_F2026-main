
#include "raylib.h"
#include "raymath.h"
#define RAYGUI_IMPLEMENTATION
#include "raygui.h"
#include <vector>

int screenWidth = 1200;
int screenHeight = 800;

const unsigned int TARGET_FPS = 50;

enum ColliderShape
{
	CIRCLE,
	HALFPLANE
};

// Base class for physics bodies
class PhysicsBody
{
public:
    Vector2 position = Vector2{ 0, 0 };
    Vector2 velocity = Vector2{ 0, 0 };
    float mass = 1.0f;
	float gravityScale = 1.0f;
    float drag = 0.0f;
    Color color = YELLOW;

    virtual ColliderShape GetShape() = 0;
    virtual void Draw() = 0;
};

// Inherits the properties of PhysicsBody and adds a radius to determine the circle's size
class Circle : public PhysicsBody
{
public:
    float radius;

    ColliderShape GetShape() override
    {
        return CIRCLE;
    }

    void Draw() override
    {
    	DrawCircleV(position, radius, color);
    }
};

// Represents a flat surface that can be used for collision detection.
class HalfPlane : public PhysicsBody
{
public:
	Vector2 GetNormal()
	{
		return normal;
	}

    void SetOrientation(float angleDegrees)
    {
		Vector2 baseNormal = { 0, -1 };
		normal = Vector2Rotate(baseNormal, angleDegrees * DEG2RAD);
	}

    ColliderShape GetShape() override
    {
        return HALFPLANE;
    }

    void Draw() override
    {
		Vector2 perpendicular = Vector2Rotate(normal, 90.0f);
		DrawLineEx(position - perpendicular * 1000, position + perpendicular * 1000, 1, color);
        DrawLineEx(position, position + normal * 50, 1, color);
    }
private:
    Vector2 normal = { 0, -1 };
};

// Resolves the collision between two circles by adjusting their positions based on the overlap.
bool ResolveCollisionCircleCircle(Circle* a, Circle* b)
{
    Vector2 disp = b->position - a->position;
    float dist = Vector2LengthSqr(disp);

    float radA = a->radius;
    float radB = b->radius;

    float sumRadius = radA + radB;
    
	float overlap = sumRadius - dist;
	
    if (overlap > 0)
    { 
		Vector2 mtv = disp / dist * overlap;
        b->position += mtv * 0.5f;
		a->position -= mtv * 0.5f;
    }
	return overlap >= 0;
};


// Returns true if two circles are overlapping based on their positions.
bool IsOverlappingCircleCircle(Circle* a, Circle* b)
{
	Vector2 disp = b->position - a->position;
	float distSqr = Vector2LengthSqr(disp);

	float radA = a->radius;
	float radB = b->radius;

    float sumRadius = radA + radB;
	return distSqr <= sumRadius * sumRadius;
};

// Resolves the collision between a circle and a half-plane by adjusting the circle's position based on the overlap.
bool ResolveCollisionCircleHalfPlane(Circle* circle, HalfPlane* halfPlane)
{
	Vector2 displacement = circle->position - halfPlane->position;
	float distance = Vector2DotProduct(displacement, halfPlane->GetNormal());
	
	float overlap = circle->radius - distance;

	if (overlap > 0)
	{
		Vector2 mtv = halfPlane->GetNormal() * overlap;
		circle->position += mtv;
	}

    return overlap >= 0;
};

// Returns true if a circle is overlapping with a half-plane based on their positions.
bool IsOverlappingCircleHalfPlane(Circle* circle, HalfPlane* halfPlane)
{
	Vector2 displacement = circle->position - halfPlane->position;
	float dot = Vector2DotProduct(displacement, halfPlane->GetNormal());
	return (dot <= circle->radius);
};

// Represents a 2D bounding box defined by its minimum and maximum points.
struct Bounds2D
{
	Bounds2D() : min{ 0, 0 }, max{ 0, 0 } {}
    Bounds2D(float top, float bottom, float left, float right)
    {
		min = { left, bottom };
		max = { right, top };
    }

	Vector2 min;
	Vector2 max;

	bool Contains(Vector2 point)
	{
		return point.x >= min.x && point.x <= max.x && point.y >= min.y && point.y <= max.y;
	}
};

// Controls the physics simulation, including updating positions, checking for collisions, and drawing the bodies.
class PhysicsSimulation
{
public:
    Bounds2D bounds;
	Vector2 gravity = { 0, 200 };
    const float FIXED_DELTA_TIME = 1.0F / (float) TARGET_FPS;
    
    // Check for Collisions
    void CheckCollisions()
    {
        for (int i = 0; i < bodies.size(); i++)
        {
			bodies[i]->color = GREEN;
        }
        
        for (int i = 0; i < bodies.size(); i++)
        {
            for (int j = 0; j < bodies.size(); j++)
            {
				PhysicsBody* bodyA = bodies[i];
                PhysicsBody* bodyB = bodies[j];

				bool isOverlapping = false;

				if (bodyA->GetShape() == CIRCLE && bodyB->GetShape() == CIRCLE)
				{
					isOverlapping = IsOverlappingCircleCircle((Circle*)bodyA, (Circle*)bodyB);
				}
				if (bodyA->GetShape() == CIRCLE && bodyB->GetShape() == HALFPLANE)
				{
					isOverlapping = IsOverlappingCircleHalfPlane((Circle*)bodyA, (HalfPlane*)bodyB);
				}
                if (bodyA->GetShape() == HALFPLANE && bodyB->GetShape() == CIRCLE)
                {
                    isOverlapping = IsOverlappingCircleHalfPlane((Circle*)bodyB, (HalfPlane*)bodyA);
                }

                if (isOverlapping)
                {
                    bodyA->color = RED;
                    bodyB->color = RED;
                }
            }
        }
    }

	// Updates the positions and velocities of all physics bodies based on their current state and the simulation's gravity.
    void Update()
    {
        for (int i = 0; i < bodies.size(); i++)
        {
			//PhysicsBody body = bodies[i];
			bodies[i]->position += bodies[i]->velocity * FIXED_DELTA_TIME;
            bodies[i]->velocity += gravity * bodies[i]->gravityScale *FIXED_DELTA_TIME;

			bodies[i]->velocity *= 1.0f - (bodies[i]->drag * FIXED_DELTA_TIME);
        }

        CheckCollisions();
    }

	// Draws all physics bodies in the simulation.
    void Draw()
    {
        for (int i = 0; i < bodies.size(); i++)
        {
			bodies[i]->Draw();
        }
    }

	// Adds a new physics body to the simulation.
	void Add(PhysicsBody* body)
	{
		bodies.push_back(body);
	}

	// Removes physics bodies that are outside the defined bounds of the simulation.
	void ClearOutOfBoundsBodies()
	{
		for (auto it = bodies.begin(); it != bodies.end();)
		{
            if (!bounds.Contains((*it)->position))
			{
				delete* it;
				it = bodies.erase(it);
			}
			
            if (it != bodies.end())
			{
				it++;
			}
		}
	}

private:
	std::vector<PhysicsBody*> bodies;
};
float margin = 0;

PhysicsSimulation sim;

Vector2 launchPosition;
float launchPositionAdjustmentSpeed = 50.0f;
float launchSpeed = 100.0f;
float launchAngle = 0.0f;
float spawnDrag = 0.0f;
float spawnRadius = 10.0f;

// Draws a preview of the trajectory of a projectile given its starting position, velocity, drag, and gravity.
void DrawTrajectoryPreview(Vector2 startPosition, Vector2 startVelocity, float drag, Vector2 gravity, float fixedDeltaTime)
{
    Vector2 position = startPosition;
    Vector2 velocity = startVelocity;

    const int steps = 100;

    for (int i = 0; i < steps; i++)
    {
        Vector2 previousPosition = position;

        position += velocity * fixedDeltaTime;
        velocity += gravity * fixedDeltaTime;

        velocity *= 1.0f - (drag * fixedDeltaTime);

        DrawLineEx(previousPosition, position, 2.0f, RAYWHITE);
    }
}

int main()
{
	sim.bounds = Bounds2D(screenWidth + margin, (float)0 - margin, 0 - margin, screenHeight + margin);
    
    InitWindow(screenWidth, screenHeight, "Physics-1 | Donal McNeely");
    SetTargetFPS(TARGET_FPS);

    // Creates a target circle
    Circle* target = new Circle;
    target->radius = 50;
	target->position = { 900, (float)(screenHeight - 200) };
    target->gravityScale = 0.0f;
    sim.Add((PhysicsBody*)target);

	launchPosition = { 30, (float) (screenHeight - 50) };

    while (!WindowShouldClose())
    {
        BeginDrawing();
        ClearBackground(Color{ 100,100,166,255 });

        DrawRectangle(0, 0, 260, 120, Color{ 255, 255, 255, 200 });

		GuiSlider(Rectangle{ 100, 0, 100, 20 }, "LaunchSpeed", TextFormat("%.2f", launchSpeed), &launchSpeed, 1, 500);
        GuiSlider(Rectangle{ 100, 30, 100, 20 }, "LaunchAngle", TextFormat("%.2f", launchAngle), &launchAngle, -89.67, 89);
        GuiSlider(Rectangle{ 100, 60, 100, 20 }, "Gravity", TextFormat("%.2f", sim.gravity.y), &sim.gravity.y, -1000, 1000);
        GuiSlider(Rectangle{ 100, 90, 100, 20 }, "Drag", TextFormat("%.2f", spawnDrag), &spawnDrag, 0, 10);
		GuiSlider(Rectangle{ 100, 120, 100, 20 }, "Radius", TextFormat("%.2f", spawnRadius), &spawnRadius, 2, 50);
    
		if (IsKeyDown(KEY_UP))
		{
			launchPosition.y -= launchPositionAdjustmentSpeed * GetFrameTime();
            //std::cout << "Launch position moved up: " << launchPosition.y << std::endl;
		}

		if (IsKeyDown(KEY_DOWN))
		{
			launchPosition.y += launchPositionAdjustmentSpeed * GetFrameTime();
            //std::cout << "Launch position moved down: " << launchPosition.y << std::endl;
		}

        Vector2 velocityPreview =
        {
            cosf(launchAngle * DEG2RAD) * launchSpeed,
            -sinf(launchAngle * DEG2RAD) * launchSpeed
        };

        float previewScale = 0.5f;

        Vector2 previewEnd =
        {
            launchPosition.x + velocityPreview.x * previewScale,
            launchPosition.y + velocityPreview.y * previewScale
        };

        DrawCircleV(launchPosition, 5, RAYWHITE);
        //DrawLineEx(launchPosition, previewEnd, 2.0f, RED);
        DrawTrajectoryPreview(launchPosition, velocityPreview, spawnDrag, sim.gravity, sim.FIXED_DELTA_TIME);

		// Launch a new bird when the space key is pressed
        if (IsKeyPressed(KEY_SPACE))
        {
            Circle* birdToLaunch = new Circle;
			birdToLaunch->color = YELLOW;
			birdToLaunch->radius = spawnRadius;
			birdToLaunch->position = launchPosition;
			birdToLaunch->velocity = velocityPreview;
			birdToLaunch->drag = spawnDrag;

            sim.Add(birdToLaunch);
        }

        sim.ClearOutOfBoundsBodies();
        sim.Update();
        sim.Draw();

        EndDrawing();
	}

    CloseWindow();
    return 0;
}
