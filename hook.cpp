#include "GameTypes.h"

// Configuration structures
ESPCfg invisibleCfg, espcfg;
AimbotCfg pistolCfg, smgCfg, arCFg, shotgunCfg, sniperCfg;

// Global variables
void* TouchControls = nullptr;
void* pSys = nullptr;
int currWeapon = -1;
std::mutex aimbot_mtx, esp_mtx;
bool shootControl = false;
Enemy localEnemy;
std::vector<Enemy> EnemyList;
int glWidth, glHeight;

// Function declarations
void configureWeapon(AimbotCfg &cfg, int currWeapon);
Vector3 predictEnemyPosition(void *character, float time);
void *getValidEnt3(AimbotCfg cfg, Vector2 rotation);
bool isCharacterVisible(void *character, void *pSys);
void setRotation(void *character, Vector2 rotation);
void ESP();

// Function to get the transform of a character
void *getTransform(void *character) {
    if (!character) return nullptr;
    return *(void **) ((uint64_t) character + string2Offset(OBFUSCATE("0x70")));
}

// Function to get the team of a character
int get_CharacterTeam(void* character) {
    int team = std::stoi(OBFUSCATE("-1"));
    if (character && get_IsInitialized(character)) {
        void* player = *(void**)((uint64_t)character + string2Offset(OBFUSCATE("0x90")));
        if (player) {
            PlayerAdapter playerAdapter;
            PlayerAdapter_CTOR(&playerAdapter, player);
            if (&playerAdapter && player && *(uint64_t*)((uint64_t) player + 0x118) != 0) {
                team = getTeamIndex(&playerAdapter);
            }
        }
    }
    return team;
}

// Function to get the username of a player
std::string get_PlayerUsername(void* player) {
    if (!player) return "";
    std::string username = OBFUSCATE("");
    std::string clantag = OBFUSCATE("");
    PlayerAdapter* playerAdapter = new PlayerAdapter;
    playerAdapter->Player = player;
    username = get_Username(playerAdapter)->getString();
    clantag = get_ClanTag(playerAdapter)->getString();
    if (clantag != std::string(OBFUSCATE(""))) {
        username = std::string(OBFUSCATE("[")) + clantag + std::string(OBFUSCATE("]")) + std::string(OBFUSCATE(" ")) +  username;
    }
    delete playerAdapter;
    return username;
}

// Function to get the position of a bone in a character
Vector3 getBonePosition(void *character, int bone) {
    if (!character) return Vector3(0, 0, 0);
    void *curBone = get_CharacterBodyPart(character, bone);
    if (!curBone) return Vector3(0, 0, 0);
    void *hitSphere = *(void **)((uint64_t) curBone + 0x20);
    if (!hitSphere) return Vector3(0, 0, 0);
    void *transform = *(void **)((uint64_t) hitSphere + 0x30);
    if (!transform) return Vector3(0, 0, 0);
    return get_Position(transform);
}

// Function to check if an angle is within the field of view
bool isInFov2(Vector2 rotation, Vector2 newAngle, AimbotCfg cfg) {
    if (!cfg.fovCheck) return true;
    Vector2 difference = newAngle - rotation;
    if (difference.Y > 180) difference.Y -= 360;
    if (difference.Y < -180) difference.Y += 360;
    return sqrt(difference.X * difference.X + difference.Y * difference.Y) <= cfg.fovValue;
}

// Function to predict the position of an enemy
Vector3 predictEnemyPosition(void *character, float time) {
    Vector3 currentPos = get_Position(getTransform(character));
    Vector3 velocity = get_CharacterVelocity(character);
    return currentPos + velocity * time;
}

// Function to configure weapon settings
void configureWeapon(AimbotCfg &cfg, int currWeapon) {
    switch (currWeapon) {
        case 0: cfg = pistolCfg; break;
        case 1: cfg = arCFg; break;
        case 2: cfg = smgCfg; break;
        case 3: cfg = shotgunCfg; break;
        case 4: cfg = sniperCfg; break;
        case 5: cfg = assaultCfg; break;
    }
}

void *getValidEnt3(AimbotCfg cfg, Vector2 rotation) {
    int id = getLocalId(pSys);
    if (id == 0) return nullptr;
    void *localPlayer = getPlayer(pSys, id);
    if (!localPlayer) return nullptr;
    int localTeam = localEnemy.team;
    float closestEntDist = 99999.0f;
    void *closestCharacter = nullptr;
    Vector3 localHead = getBonePosition(localEnemy.Character, 10);
    if (getIsCrouched(localEnemy.Character)) {
        localHead = localHead - Vector3(0, 0.5, 0);
    }
    for (const auto& currentEnemy : EnemyList) {
        int curTeam = currentEnemy.team;
        int health = get_Health(currentEnemy.Character);
        bool canSet = false;
        Vector2 newAngle;
        if (cfg.aimbot && localEnemy.Character && get_Health(localEnemy.Character) > 0 && health > 0) {
            for (BodyPart part : cfg.aimBones) {
                Vector3 enemyBone = predictEnemyPosition(currentEnemy.Character, 0.1f);
                Vector3 deltavec = enemyBone - localHead;
                float deltLength = sqrt(deltavec.X * deltavec.X + deltavec.Y * deltavec.Y + deltavec.Z * deltavec.Z);
                newAngle.X = -asin(deltavec.Y / deltLength) * (180.0 / PI);
                newAngle.Y = atan2(deltavec.X, deltavec.Z) * 180.0 / PI;
                if (isInFov2(rotation, newAngle, cfg) && localTeam != curTeam && curTeam != -1) {
                    if (cfg.visCheck && get_Health(localEnemy.Character) > 0 && isCharacterVisible(currentEnemy.Character, pSys)) {
                        canSet = true;
                    }
                    void *transform = getTransform(localEnemy.Character);
                    if (transform) {
                        Vector3 localPosition = get_Position(transform);
                        Vector3 currentCharacterPosition = get_Position(getTransform(currentEnemy.Character));
                        Vector3 currentEntDist = Vector3::Distance(localPosition, currentCharacterPosition);
                        if (Vector3::Magnitude(currentEntDist) < closestEntDist && (!cfg.visCheck || canSet)) {
                            closestEntDist = Vector3::Magnitude(currentEntDist);
                            closestCharacter = currentEnemy.Character;
                            break; // Stop after the first valid target
                        }
                    }
                }
            }
        }
    }
    return closestCharacter;
}
}

bool isCharacterVisible(void *character, void *pSys) {
    void *localCharacter = get_LocalCharacter(pSys);
    return localCharacter && !isHeadBehindWall(localCharacter, character);
}
}


    oSetRotation(character, rotation + difference);
}
    if (cfg.triggerbot && closestEnt && localEnemy.Character && get_Health(localEnemy.Character) > 0 && !get_Invulnerable(closestEnt)) {
        int hitIndex = 0;
        void *camera = get_camera();
        if (camera) {
            Ray ray = ScreenPointToRay(camera, Vector2(glWidth / 2, glHeight / 2), 2);
            if (closestEnt) {
                UpdateCharacterHitBuffer(pSys, closestEnt, ray, &hitIndex);
            }
            if (hitIndex && !shootControl) {
                shootControl = 1;
            }
        }
 void setRotation(void *character, Vector2 rotation) {
    std::lock_guard<std::mutex> guard(aimbot_mtx);
    Vector2 newAngle, difference = {0, 0};
    AimbotCfg cfg;
    if (localEnemy.Character) {
        currWeapon = getCurrentWeaponCategory(localEnemy.Character);
        if (currWeapon != -1) {
            configureWeapon(cfg, currWeapon);
        }
    }
    void *closestEnt = (character && localEnemy.Character && get_IsInitialized(localEnemy.Character)) ? getValidEnt3(cfg, rotation) : nullptr;
    if (localEnemy.Character && get_Health(localEnemy.Character) > 0 && closestEnt) {
        Vector3 localHead = getBonePosition(localEnemy.Character, 10);
        if (getIsCrouched(localEnemy.Character)) {
            localHead -= Vector3(0, 0.5, 0);
        }
        for (BodyPart part : cfg.aimBones) {
            Vector3 enemyBone = getBonePosition(closestEnt, part);
            Vector3 deltavec = enemyBone - localHead;
            float deltLength = sqrt(deltavec.X * deltavec.X + deltavec.Y * deltavec.Y + deltavec.Z * deltavec.Z);
            newAngle.X = -asin(deltavec.Y / deltLength) * (180.0 / PI);
            newAngle.Y = atan2(deltavec.X, deltavec.Z) * 180.0 / PI;
            if (cfg.aimbot && character == localEnemy.Character) {
                difference = (cfg.fovCheck ? isInFov2(rotation, newAngle, cfg) : newAngle - rotation) / (cfg.aimbotSmooth ? cfg.smoothAmount : 1);
                break; // Stop after the first valid target
            }
        }
    }
  oSetRotation(character, rotation + difference);
}
}

// ESP function to draw information on the screen
void ESP() {
    AimbotCfg cfg;

    std::lock_guard<std::mutex> guard(esp_mtx);
    auto background = ImGui::GetBackgroundDrawList();
    if (pSys == nullptr || !(esp || pistolCfg.aimbot || shotgunCfg.aimbot || smgCfg.aimbot || arCFg.aimbot || sniperCfg.aimbot)) {
        return;
    }

    int id = getLocalId(pSys);
    if (id == 0) {
        return;
    }

    if (pSys == nullptr) {
        return;
    }

    void *localPlayer = getPlayer(pSys, id);
    if (localPlayer == nullptr) {
        return;
    }

    auto cam = get_camera();
    if (cam == nullptr) {
        return;
    }

    for (int i = 0; i < EnemyList.size(); i++) {
        Enemy currentEnemy = EnemyList[i];
        void *currentCharacter = currentEnemy.Character;
        if (currentCharacter == nullptr) {
            continue;
        }
        void *currentPlayer = currentEnemy.Player;
        if (currentPlayer == nullptr) {
            continue;
        }

        if(localPlayer == currentEnemy.Player){
            localEnemy = currentEnemy;
        }
        int health = get_Health(currentEnemy.Character);
        int localTeam = localEnemy.team;
        int curTeam = currentEnemy.team;

        if (health <= 0 || localTeam == curTeam || curTeam == -1) {
            continue;
        }

        void *transform = getTransform(currentCharacter);
        void *localTransform = getTransform(localEnemy.Character);
        if (transform == nullptr || localTransform == nullptr) {
            continue;
        }

        Vector3 position = get_Position(transform);
        Vector3 transformPos = WorldToScreen(cam, position, 2.4);
        transformPos.Y = glHeight - transformPos.Y;
        Vector3 headPos = getBonePosition(currentCharacter, HEAD);
        Vector3 chestPos = getBonePosition(currentCharacter, CHEST);
        Vector3 wschestPos = WorldToScreen(cam, chestPos, 4);
        Vector3 wsheadPos = WorldToScreen(cam, headPos, 4);
        Vector3 aboveHead = headPos + Vector3(0, 0.4, 0);
        Vector3 headEstimate = position + Vector3(0, 1.48, 0);

        Vector3 wsAboveHead = WorldToScreen(cam, aboveHead, 4);
        Vector3 wsheadEstimate = WorldToScreen(cam, headEstimate, 4);

        wsAboveHead.Y = glHeight - wsAboveHead.Y;
        wsheadEstimate.Y = glHeight - wsheadEstimate.Y;

        float height = transformPos.Y - wsAboveHead.Y;
        float width = (transformPos.Y - wsheadEstimate.Y) / 2;

        Vector3 localPosition = get_Position(localTransform);
        Vector3 currentCharacterPosition = get_Position(transform);
        float currentEntDist = Vector3::Distance(localPosition, currentCharacterPosition);

        espcfg = invisibleCfg;

        if (esp) {
            if (espcfg.snapline && transformPos.Z > 0) {
                DrawLine(ImVec2(glWidth / 2, glHeight), ImVec2(transformPos.X, transformPos.Y), ImColor(espcfg.snaplineColor.x, espcfg.snaplineColor.y, espcfg.snaplineColor.z, (255 - currentEntDist * 2)));
            }

            if (espcfg.bone && transformPos.Z > 0 && currentCharacter != nullptr) {
                DrawBones(currentCharacter, LOWERLEG_LEFT, UPPERLEG_LEFT, espcfg, background);
                DrawBones(currentCharacter, LOWERLEG_RIGHT, UPPERLEG_RIGHT, espcfg, background);
                DrawBones(currentCharacter, UPPERLEG_LEFT, STOMACH, espcfg, background);
                DrawBones(currentCharacter, UPPERLEG_RIGHT, STOMACH, espcfg, background);
                DrawBones(currentCharacter, STOMACH, CHEST, espcfg, background);
                DrawBones(currentCharacter, LOWERARM_LEFT, UPPERARM_LEFT, espcfg, background);
                DrawBones(currentCharacter, LOWERARM_RIGHT, UPPERARM_RIGHT, espcfg, background);
                DrawBones(currentCharacter, UPPERARM_LEFT, CHEST, espcfg, background);
                DrawBones(currentCharacter, UPPERARM_RIGHT, CHEST, espcfg, background);
                Vector3 diff = wschestPos - wsheadPos;
                Vector3 neck = (chestPos + headPos) / 2;
                Vector3 wsneck = WorldToScreen(cam, neck, 2);
                wsneck.Y = glHeight - wsneck.Y;
                wschestPos.Y = glHeight - wschestPos.Y;
                wsheadPos.Y = glHeight - wsheadPos.Y;
                if (wschestPos.Z > 0 && wsneck.Z) {
                    DrawLine(ImVec2(wschestPos.X, wschestPos.Y), ImVec2(wsneck.X, wsneck.Y), ImColor(espcfg.boneColor.x, espcfg.boneColor.y, espcfg.boneColor.z), 3, background);
                }

                if (wsheadPos.Z > 0 && wschestPos.Z > 0) {
                    float radius = sqrt(diff.X * diff.X + diff.Y * diff.Y);
                    background->AddCircle(ImVec2(wsheadPos.X, wsheadPos.Y), radius / 2, IM_COL32(espcfg.boneColor.x * 255, espcfg.boneColor.y * 255, espcfg.boneColor.z * 255, 255), 0, 3.0f);
                }
            }

            if (espcfg.box && transformPos.Z > 0 && wsAboveHead.Z > 0) {
                DrawOutlinedBox2(wsAboveHead.X - width / 2, wsAboveHead.Y, width, height, ImVec4(espcfg.boxColor.x, espcfg.boxColor.y, espcfg.boxColor.z, 255), 3, background);
            }

            if (espcfg.healthesp && transformPos.Z > 0 && wsAboveHead.Z > 0) {
                DrawOutlinedFilledRect(wsAboveHead.X - width / 2 - 12, wsAboveHead.Y + height * (1 - (static_cast<float>(health) / 100.0f)), 3, height * (static_cast<float>(health) / 100.0f), HealthToColor(health), background);
            }

            if (espcfg.healthNumber && transformPos.Z > 0 && wsAboveHead.Z > 0) {
                DrawText(ImVec2(wsAboveHead.X - width / 2 - 17, wsAboveHead.Y + height * (1 - static_cast<float>(health) / 100.0f) - 3), ImVec4(1, 1, 1, 255), std::to_string(health), espFont, background);
            }

            if (espcfg.distance && transformPos.Z > 0) {
                DrawText(ImVec2(transformPos.X + width / 2, transformPos.Y - 12), ImVec4(1, 1, 1, 255), std::to_string(static_cast<int>(currentEntDist)) + "m", espFont, background);
            }

            if (espcfg.name && transformPos.Z > 0 && currentCharacter != nullptr) {
                void *player = get_Player(currentCharacter);
                if (player == nullptr) continue;
                std::string username = get_PlayerUsername(player);
                DrawText(ImVec2(transformPos.X - width / 2, transformPos.Y - 12), espcfg.nameColor, username, espFont, background);
            }
        }
    }
}
