#include "raylib.h"
#include "raymath.h"
#include <iostream>
#include <vector>
#include <cstdlib>
#include <ctime>
#include <cmath>
#include <algorithm>
#include <string>
#include <thread>
#include <chrono>
using namespace std;

int roundNum = 1;
bool newRound = false;

void normalize(Vector2* temp)
{
    float length = Vector2Length(*temp);
    if (length != 0) *temp = Vector2Scale(*temp, 1.0f / length);
}

float rotate(Vector2 vel) { return atan2(vel.y, vel.x) * RAD2DEG; }

bool eat(Vector2 imagePos, int imageWidth, int imageHeight, Vector2 circlePos, float radius)
{
    float closestX = Clamp(circlePos.x, imagePos.x, imagePos.x + imageWidth);
    float closestY = Clamp(circlePos.y, imagePos.y, imagePos.y + imageHeight);
    float distanceX = circlePos.x - closestX;
    float distanceY = circlePos.y - closestY;
    return (distanceX * distanceX + distanceY * distanceY) < (radius * radius);
}

class Shooter {
public:
    bool isEvolving = false;
    float evolveTimer = 0.0f;
    float evolveDuration = 10.0f;

    float maxspeed, maxforce, tarAngle, shootInterval, shootTimer;
    float curAngle = 0.0f;
    float rotSpeed = 0.1f;
    int counter, points;
    Vector2 pos, vel, tar, seek, desVel, steering;
    Texture2D picture;

    Shooter(float interval)
    {
        picture = LoadTexture("res\\gudetama.png");
        maxspeed = 10.0f;
        maxforce = 0.5f;
        shootInterval = interval;
        shootTimer = 0.0f;
        points = 0; counter = 0;
    }

    int getPoint() { return points; }
    void resetPoints() { points = 0; }
    Texture2D getPic() { return picture; }

    Vector2 getSeek() { return seek; }
    void setSeek(Vector2 s) { seek = s; }

    Vector2 getTar() { return tar; }
    void setTar(Vector2 target) { tar = target; }

    Vector2 getPos() { return pos; }
    void setPos(Vector2 p) { pos = p; }
    void setPos(float x, float y) { pos = { x, y }; }

    Vector2 getVel() { return vel; }
    void setVel(Vector2 v) { vel = v; }

    Vector2 getDesVel() { return desVel; }
    void setDesVel(Vector2 d) { desVel = d; }

    Vector2 getSteering() { return steering; }
    void setSteering(Vector2 s) { steering = s; }

    float getTarAngle() { return tarAngle; }
    void setTarAngle(float t) { tarAngle = t; }

    float getCurAngle() { return curAngle; }
    void setCurAngle(float c) { curAngle = c; }

    float getMaxSpeed() { return maxspeed; }
    float getMaxForce() { return maxforce; }
    float getRotSpeed() { return rotSpeed; };

    void evolveforce() { maxforce *= 1.5; }
    void evolvespeed() { maxspeed *= 1.5; }

    void updatePoints()
    {
        points++;
        counter++;
    }

    void changePic()
    {
        picture = LoadTexture("res\\gudetama2.png");
        //picture = LoadTexture("C:\\Users\\Nandh\\Downloads\\gudetama2.png");
    }
    void keepPic()
    {
        picture = LoadTexture("res\\gudetama.png");
    }

    void mutate(float otherforce, float otherspeed)
    {
        maxforce = (otherforce + maxforce) / 2;
        maxspeed = (otherspeed + maxspeed) / 2;
    }

    void updatePosition(float screenWidth, float screenHeight)
    {
        const float damping = 0.8f;

        pos.x += vel.x;
        pos.y += vel.y;

        vel = Vector2Scale(vel, damping);

        if (pos.x <= 0.001f || pos.x >= screenWidth - 0.001f)
        {
            vel.x *= -1;
            if (pos.x < 0) pos.x = 0;
            else pos.x = screenWidth;
        }
        if (pos.y <= 0.001f || pos.y >= screenHeight - 0.001f)
        {
            vel.y *= -1;
            if (pos.y < 0) pos.y = 0;
            else pos.y = screenHeight;
        }
    }

    void updateShooter(Shooter& shooter)
    {
        Vector2 seek = Vector2Subtract(shooter.getTar(), shooter.getPos());
        float distance = Vector2Length(seek);

        if (distance < 5.0f)
        {
            Vector2 tar = { static_cast<float>(rand() % 751), static_cast<float>(rand() % 401) };
            shooter.setTar(tar);
            return;
        }

        normalize(&seek);
        Vector2 steering = Vector2Scale(seek, shooter.getMaxSpeed());

        if (Vector2Length(steering) > shooter.getMaxForce())
        {
            steering = Vector2Scale(Vector2Normalize(steering), shooter.getMaxForce());
        }

        Vector2 newVel = Vector2Add(shooter.getVel(), steering);
        if (Vector2Length(newVel) > shooter.getMaxSpeed())
        {
            newVel = Vector2Scale(Vector2Normalize(newVel), shooter.getMaxSpeed());
        }

        shooter.setVel(newVel);
        shooter.setPos(Vector2Add(shooter.getPos(), newVel));

        float targetAngle = rotate(newVel);

        float tiltSpeed = 0.5f;
        if (shooter.getCurAngle() < targetAngle)
        {
            shooter.setCurAngle(shooter.getCurAngle() + tiltSpeed);
            if (shooter.getCurAngle() > targetAngle) shooter.setCurAngle(targetAngle); // Clamp
        }
        else if (shooter.getCurAngle() > targetAngle)
        {
            shooter.setCurAngle(shooter.getCurAngle() - tiltSpeed);
            if (shooter.getCurAngle() < targetAngle) shooter.setCurAngle(targetAngle); // Clamp
        }
    }


    void evolve(int p)
    {
        maxforce *= p;
        isEvolving = true;
        evolveTimer = 0.0f;
    }
};

class Projectile {
public:
    Vector2 velocity, position;
    float speed;
    Color color;
    Projectile(Vector2 startPos, float angle, Color projColor)
    {
        position = startPos;
        speed = 5.0f;
        velocity = { speed * cos(angle), speed * sin(angle) };
        color = projColor;
    }
    void updatePos()
    {
        position = Vector2Add(position, velocity);
    }
    bool IsOffScreen(float screenWidth, float screenHeight)
    {
        return position.x < 0 || position.x > screenWidth || position.y < 0 || position.y > screenHeight;
    }
};

class Food {
public:
    bool eaten;
    Vector2 pos;
    float respawnTime;
    float respawnTimer;

    Food()
    {
        eaten = false;
        pos = { 0, 0 };
        respawnTime = 2.5f;
        respawnTimer = 0.0f;
    }

    Vector2 getPos() { return pos; }
    void setPos(Vector2 v) { pos = v; }

    void update(float deltaTime)
    {
        if (eaten)
        {
            respawnTimer += deltaTime;
            if (respawnTimer >= respawnTime)
            {
                pos = { static_cast<float>(rand() % 751), static_cast<float>(rand() % 401) };
                eaten = false;
                respawnTimer = 0.0f;
            }
        }
    }

    void Draw()
    {
        if (!eaten)
            DrawCircle(pos.x, pos.y, 5.0f, BLACK);
    }
};

class Magnet {
public:
    Texture2D texture;
    float maxSpeed = 1.5f;
    float maxForce = 1;
    float radius = 65.0f;
    Texture2D player;
    Vector2 position;
    Vector2 target;
    Vector2 velocity;
    Vector2 direction;
    Vector2 desiredVelocity;
    Vector2 steering;
    Vector2 acceleration;
    float tiltAngle = 0.0f;
    bool isCollecting = false;
    float resetTimer = 0.0f;
    int score = 0;
    bool isEvolving = false;
    float evolveTimer = 0.0f;
    float evolveDuration = 10.0f;

    Magnet() {
        velocity = { 0.0f, 0.0f };
        setPosition();
        setTargetPosition();
        texture = LoadTexture("res\\pinkfairybatman.png");
    }

    Texture2D getPic()
    {
        return texture;
    }

    void setPosition()
    {
        int randX = rand() % (700 + 1);
        int randY = rand() % (350 + 1);

        position.x = randX;
        position.y = randY;
    }

    void setTargetPosition()
    {
        int randX = rand() % (700 + 1);
        int randY = rand() % (350 + 1);

        target.x = randX;
        target.y = randY;
    }

    void draw()
    {
        float deltaTime = GetFrameTime();
        if (isEvolving) {
            DrawCircleV(position, 5.0, DARKPURPLE);
            evolveTimer += deltaTime;

            if (evolveTimer >= evolveDuration) {
                isEvolving = false;
            }
        }

        direction = Vector2Subtract(target, position);
        direction = Vector2Normalize(direction);

        desiredVelocity = Vector2Subtract(target, position);
        desiredVelocity = Vector2Normalize(desiredVelocity);
        desiredVelocity = Vector2Scale(desiredVelocity, maxSpeed);

        steering = Vector2Subtract(desiredVelocity, velocity);
        float steeringLength = Vector2Length(steering);

        if (steeringLength > maxForce)
            steering = Vector2Scale(Vector2Normalize(steering), maxForce);

        acceleration = steering;
        velocity = Vector2Add(velocity, acceleration);

        float vaLength = Vector2Length(velocity);
        if (vaLength > maxSpeed)
            velocity = Vector2Scale(Vector2Normalize(velocity), maxSpeed);

        position.x = Clamp(position.x, 0.0f, 700.0f);
        position.y = Clamp(position.y, 0.0f, 350.0f);
        position = Vector2Add(position, velocity);

        tiltAngle = atan2(velocity.y, velocity.x) * 5;

        if (Vector2Distance(position, target) < 5.0f)
            setTargetPosition();

        if (isCollecting) {
            resetTimer += GetFrameTime();
            if (resetTimer > 1.0f)
                resetAttributes();
        }

        Vector2 textureCenter = {
            position.x + texture.width / 2,
            position.y + texture.height / 2
        };

        DrawCircleV(textureCenter, radius, Fade(RED, 0.25f));

        DrawTextureEx(texture, position, tiltAngle, 1.0f, RAYWHITE);
    }

    Vector2 getPosition()
    {
        return position;
    }

    Vector2 getTargetPosition()
    {
        return target;
    }

    void mutate(Shooter& shooter) {
        float avgForce = (maxForce + shooter.maxforce) / 2;
        maxForce = avgForce;
        shooter.maxforce = avgForce;

        float avgSpeed = (maxSpeed + shooter.maxspeed) / 2;
        maxSpeed = avgSpeed;
        shooter.maxspeed = avgSpeed;
    }

    void setCollecting() {
        isCollecting = true;
        maxSpeed += 0.5f;
        radius = 65.0f;
        texture = LoadTexture("res\\princessbatman.png");
        resetTimer = 0.0f;
    }

    bool collisionWithFood(Food foods[], int foodN) {
        for (int i = 0; i < foodN; i++)
            if (!foods[i].eaten && Vector2Distance(position, foods[i].getPos()) < radius) {
                target = foods[i].getPos();
                foods[i].eaten = true;
                setCollecting();
                score++;
                setTargetPosition();
                return true;
            }
        return false;
    }

    void resetAttributes() {
        isCollecting = false;
        texture = LoadTexture("res\\pinkfairybatman.png");
    }

    int getScore()
    {
        return score;
    }
    void resetScore()
    {
        score = 0;
    }

    void evolve(int p)
    {
        maxSpeed *= p;
        isEvolving = true;
        evolveTimer = 0.0f;
    }
};

int main(void)
{
    float screenWidth = 800, screenHeight = 450;
    InitWindow(screenWidth, screenHeight, "Evolutionary Autonomous Project");
    SetTargetFPS(30);
    float count = 0;
    srand(static_cast<unsigned int>(time(0)));

    vector<Magnet> magnets(4);
    for (Magnet& player : magnets)
        player = Magnet();

    const int n = 4;
    vector<Shooter> shooters = { Shooter(2.0f), Shooter(3.0f), Shooter(4.0f), Shooter(5.0f) };

    vector<Projectile> projectiles;
    vector<float> shootTimers(shooters.size(), 0.0f);

    for (int i = 0; i < shooters.size(); i++)
    {
        shooters[i].setPos(rand() % 751, rand() % 401);
        shooters[i].setTar({ static_cast<float>(rand() % 751), static_cast<float>(rand() % 401) });
    }

    const int foodN = 15;
    Food foods[foodN];
    for (int i = 0; i < foodN; i++)
    {
        Vector2 loc = { static_cast<float>(rand() % 751), static_cast<float>(rand() % 401) };
        foods[i] = Food();
        foods[i].setPos(loc);
    }

    Color projectileColors[] = { RED, GREEN, BLUE, YELLOW };

    float roundTimer = 0.0f;
    const float roundTime = 30.0f;

    float targetChangeTimer = 0.0f;
    const float targetChangeInterval = 1.0f;

    double roundStartTime = 0;
    bool showNewRoundMessage = false;

    while (!WindowShouldClose())
    {
        float deltaTime = GetFrameTime();
        roundTimer += deltaTime;
        targetChangeTimer += deltaTime;

        BeginDrawing();
        ClearBackground(WHITE);

        for (int i = 0; i < foodN; i++)
        {
            foods[i].update(deltaTime);
            foods[i].Draw();
        }

        for (Magnet& player : magnets)
        {
            if (player.collisionWithFood(foods, foodN))
                for (int i = 0; i < foodN; i++)
                {
                    foods[i].update(deltaTime);
                    foods[i].Draw();
                }
            player.draw();
        }//draw mag

        for (int i = 0; i < shooters.size(); i++)
        {
            shootTimers[i] += deltaTime;

            if (shooters[i].isEvolving) {
                DrawCircleV(shooters[i].getPos(), 5.0, DARKPURPLE);
                shooters[i].evolveTimer += deltaTime;

                if (shooters[i].evolveTimer >= shooters[i].evolveDuration) {
                    shooters[i].isEvolving = false;
                }
            }
            if (shootTimers[i] >= shooters[i].shootInterval)
            {
                const float angleStep = 72.0f * (PI / 180.0f);
                for (int j = 0; j < 5; j++)
                {
                    float angle = j * angleStep;
                    Vector2 startPos = {
                        shooters[i].getPos().x + shooters[i].getPic().width / 2,
                        shooters[i].getPos().y + shooters[i].getPic().height / 2
                    };
                    projectiles.emplace_back(startPos, angle, projectileColors[i % 4]);
                }
                shootTimers[i] = 0.0f;
            }//shoot

            for (auto it = projectiles.begin(); it != projectiles.end();)
            {
                it->updatePos();
                bool hitFood = false;

                for (int j = 0; j < foodN; j++)
                    if (!foods[j].eaten && eat(it->position, 5, 10, foods[j].getPos(), 5.0f)) {
                        foods[j].eaten = true;
                        shooters[i].updatePoints();
                        shooters[i].counter % 2 == 1 ? shooters[i].changePic() : shooters[i].keepPic();
                        hitFood = true;
                        break;
                    }

                if (hitFood || it->IsOffScreen(screenWidth, screenHeight))
                    it = projectiles.erase(it);
                else it++;
            }//projectile eat

            shooters[i].updatePosition(screenWidth, screenHeight);
            shooters[i].updateShooter(shooters[i]);

            float offsetX = shooters[i].getPic().width / 2.0f;
            float offsetY = shooters[i].getPic().height / 2.0f;

            Vector2 adjustedPos = {
                shooters[i].getPos().x - offsetX,
                shooters[i].getPos().y - offsetY
            };

            DrawTextureEx(shooters[i].getPic(), adjustedPos, shooters[i].getCurAngle(), 1.0f, WHITE);

            for (int j = 0; j < foodN; j++)
            {
                float shooterw = shooters[i].getPic().width;
                float shooterh = shooters[i].getPic().height;

                if (!foods[j].eaten && eat(shooters[i].getPos(), shooterw, shooterh, foods[j].getPos(), 5.0f))
                {
                    foods[j].eaten = true;
                    shooters[i].updatePoints();

                    if (shooters[i].counter % 2 == 1)
                        shooters[i].changePic();
                    else shooters[i].keepPic();
                }
            }
        }//shooter draw and eat

        for (const auto& proj : projectiles)
            DrawRectangle(proj.position.x, proj.position.y, 5, 10, proj.color);

        if (roundTimer >= roundTime)
        {
            newRound = true;
            if (magnets.size() == 0)
                break;
            if (shooters.size() == 0)
                break;
            roundTimer = 0.0f;
            roundNum++;

            int maxIndex;
            for (int i = 0; i < shooters.size() - 1; i++)
            {
                maxIndex = i;
                for (int j = i + 1; j < shooters.size(); j++)
                    if (shooters[j].getPoint() > shooters[maxIndex].getPoint())
                        maxIndex = j;
                Shooter temp = shooters[i];
                shooters[i] = shooters[maxIndex];
                shooters[maxIndex] = temp;
            }//sort

            for (int i = 0; i < magnets.size() - 1; i++)
            {
                maxIndex = i;
                for (int j = i + 1; j < magnets.size(); j++)
                    if (magnets[j].getScore() > magnets[maxIndex].getScore())
                        maxIndex = j;
                Magnet temp = magnets[i];
                magnets[i] = magnets[maxIndex];
                magnets[maxIndex] = temp;
            }//sort

            if (magnets[magnets.size() - 1].getScore() < shooters[shooters.size() - 1].getPoint())
            {
                magnets.pop_back();
            }
            else if (magnets[magnets.size() - 1].getScore() > shooters[shooters.size() - 1].getPoint())
            {
                shooters.pop_back();
            }
            else {
                magnets.pop_back();
                shooters.pop_back();
            }

            if (shooters.size() >= 2 && magnets.size() >= 2)
            {
                if (shooters[0].getPoint() > magnets[0].getScore() && shooters[1].getPoint() > magnets[1].getScore())
                {
                    shooters[0].evolve(2);
                    magnets[0].evolve(1.8);
                    shooters[1].evolve(1.6);
                    magnets[1].evolve(1.4);
                }
                else if (shooters[0].getPoint() < magnets[0].getScore() && shooters[1].getPoint() < magnets[1].getScore())
                {
                    magnets[0].evolve(2);
                    shooters[0].evolve(1.8);
                    magnets[1].evolve(1.6);
                    shooters[1].evolve(1.4);

                }
                else
                {
                    magnets[0].evolve(2);
                    shooters[0].evolve(2);
                    magnets[1].evolve(1.8);
                    shooters[1].evolve(1.8);
                }
            }   //evolve

            for (Magnet& player : magnets)
                for (Shooter& shooter : shooters) {
                    if (Vector2Distance(player.getPosition(), shooter.getPos()) <= 50.0f)
                    {
                        player.mutate(shooter);

                        shooter.isEvolving = true;
                        player.isEvolving = true;
                    }
                }
            //mutate
            for (int i = 0; i < shooters.size(); i++)
                shooters[i].resetPoints();
            for (int i = 0; i < magnets.size(); i++)
                magnets[i].resetScore();
        }//round end

        string roundprint = "Round " + to_string(roundNum);
        DrawText(roundprint.c_str(), 10, 10, 30, RED);
        EndDrawing();
    }//while

    CloseWindow();
    return 0;
}//main