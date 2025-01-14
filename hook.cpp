#include <jni.h>
#include <android/log.h>
#include <thread>
#include <chrono>
#include <string>
#include <vector>
#include <mutex>
#include <cstdlib> // Include for getenv
#include "GameTypes.h"

// Configuration structures
ESPCfg espcfg;
AimbotCfg pistolCfg, smgCfg, arCfg, shotgunCfg, sniperCfg;

// Global variables
void* TouchControls = nullptr;
void* pSys = nullptr;
int currWeapon = -1;
std::mutex aimbot_mtx, esp_mtx;
bool shootControl = false;
Enemy localEnemy;
std::vector<Enemy> EnemyList;
int glWidth, glHeight;
bool isCriticalOpsRunning = false;

// Function declarations
void configureWeapon(AimbotCfg &cfg, int currWeapon);
Vector3 predictEnemyPosition(void *character, float time);
void *getValidEnt3(AimbotCfg cfg, Vector2 rotation);
bool isCharacterVisible(void *character, void *pSys);
void setRotation(void *character, Vector2 rotation);
void ESP();
void checkForCriticalOps(JNIEnv* env);
void stopPlayerMovement();
Vector2 getRecoilOffset();
bool shouldStopForAccuracy(AimbotCfg cfg, Vector2 newAngle, Vector2 rotation);
void RadarHack(); // Radar hack function declaration

// Function to get the transform of a character
void *getTransform(void *character) {
    if (!character) return nullptr;
    return *(void **) ((uint64_t) character + string2Offset(OBFUSCATE("0x70")));
}

// Initialization function for touch controls
extern "C" JNIEXPORT void JNICALL
Java_com_criticalforceentertainment_criticalops_CriticalOpsMainActivity_initTouchControls(JNIEnv* env, jobject obj) {
    // Initialize TouchControls here
    TouchControls = new YourTouchControlClass(); // Example initialization
    std::thread(checkForCriticalOps, env).detach(); // Start the thread to check for Critical Ops
}

// Function to handle touch events
extern "C" JNIEXPORT void JNICALL
Java_com_criticalforceentertainment_criticalops_CriticalOpsMainActivity_onTouchEvent(JNIEnv* env, jobject obj, jint action, jfloat x, jfloat y) {
    if (!isCriticalOpsRunning) return; // Do nothing if Critical Ops is not running

    switch (action) {
        case 0: // ACTION_DOWN
            // Trigger aimbot on touch down
            setRotation(localEnemy.Character, Vector2(x, y));
            break;
        case 2: // ACTION_MOVE
            // Adjust aim as finger moves
            setRotation(localEnemy.Character, Vector2(x, y));
            break;
        case 1: // ACTION_UP
            // Stop aimbot on touch up
            break;
    }
}

// Function to check if Critical Ops is running and enable aimbot
void checkForCriticalOps(JNIEnv* env) {
    while (true) {
        isCriticalOpsRunning = isGameRunning(env, "com.criticalforceentertainment.criticalops");
        if (isCriticalOpsRunning) {
            ESP(); // Start ESP when the game is running
            RadarHack(); // Start Radar Hack when the game is running
            enableAimbot(); // Start Aimbot when the game is running
        } else {
            disableAimbot(); // Stop Aimbot when the game is not running
        }
        std::this_thread::sleep_for(std::chrono::seconds(5)); // Check every 5 seconds
    }
}

// Function to enable aimbot
void enableAimbot() {
    configureWeapon(pistolCfg, currWeapon);
    configureWeapon(smgCfg, currWeapon);
    configureWeapon(arCfg, currWeapon);
    configureWeapon(shotgunCfg, currWeapon);
    configureWeapon(sniperCfg, currWeapon);
    // Additional configuration as needed
}

// Function to disable aimbot
void disableAimbot() {
    pistolCfg.aimbot = false;
    smgCfg.aimbot = false;
    arCfg.aimbot = false;
    shotgunCfg.aimbot = false;
    sniperCfg.aimbot = false;
    // Additional configuration as needed
}

// Actual implementation to check if a game is running
bool isGameRunning(JNIEnv* env, const std::string& packageName) {
    jclass activityManagerClass = env->FindClass("android/app/ActivityManager");
    jmethodID getRunningAppProcesses = env->GetMethodID(activityManagerClass, "getRunningAppProcesses", "()Ljava/util/List;");
    jobject activityManager = env->CallObjectMethod(env->GetStaticObjectField(activityManagerClass, env->GetFieldID(activityManagerClass, "INSTANCE", "Landroid/app/ActivityManager;")));
    jobject runningAppProcesses = env->CallObjectMethod(activityManager, getRunningAppProcesses);
    jclass listClass = env->FindClass("java/util/List");
    jmethodID sizeMethod = env->GetMethodID(listClass, "size", "()I");
    jmethodID getMethod = env->GetMethodID(listClass, "get", "(I)Ljava/lang/Object;");
    jint size = env->CallIntMethod(runningAppProcesses, sizeMethod);

    for (int i = 0; i < size; i++) {
        jobject appProcessInfo = env->CallObjectMethod(runningAppProcesses, getMethod, i);
        jclass appProcessInfoClass = env->FindClass("android/app/ActivityManager$RunningAppProcessInfo");
        jfieldID processNameField = env->GetFieldID(appProcessInfoClass, "processName", "Ljava/lang/String;");
        jstring processName = (jstring) env->GetObjectField(appProcessInfo, processNameField);
        const char* processNameChars = env->GetStringUTFChars(processName, 0);

        if (packageName == processNameChars) {
            env->ReleaseStringUTFChars(processName, processNameChars);
            return true;
        }

        env->ReleaseStringUTFChars(processName, processNameChars);
    }

    return false;
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
    Vector3 acceleration = get_CharacterAcceleration(character); // Assume this function exists
    Vector3 futurePos = currentPos + velocity * time + 0.5f * acceleration * time * time;
    return futurePos;
}

// Function to configure weapon settings
void configureWeapon(AimbotCfg &cfg, int currWeapon) {
    switch (currWeapon) {
        case 0: cfg = pistolCfg; break;
        case 1: cfg = arCfg; break;
        case 2: cfg = smgCfg; break;
        case 3: cfg = shotgunCfg; break;
        case 4: cfg = sniperCfg; break;
    }
    cfg.aimBones = {HEAD}; // Always aim for the head
    cfg.aimbotSmooth = false; // Disable smoothing for instant aim
    cfg.smoothAmount = 0.0f; // No smoothing amount
    cfg.fovCheck = false; // Ignore FOV checks
    cfg.aimbot = true; // Ensure aimbot is enabled
    cfg.onShoot = true; // Ensure aimbot activates on shooting
    cfg.visCheck = false; // Disable visibility checks
    std::cout << "Aimbot Configured: " << cfg.aimbot << std::endl;
}

// Function to get the valid enemy target
void *getValidEnt3(AimbotCfg cfg, Vector2 rotation) {
    int id = getLocalId(pSys);
    if (id == 0) return nullptr;
    void *localPlayer = getPlayer(pSys, id);
    if (!localPlayer) return nullptr;
    int localTeam = localEnemy.team;
    float closestEntDist = 99999.0f;
    void *closestCharacter = nullptr;
    Vector3 localHead = getBonePosition(localEnemy.Character, HEAD);

    if (getIsCrouched(localEnemy.Character)) {
        localHead = localHead - Vector3(0, 0.5, 0);
    }

    for (const auto& currentEnemy : EnemyList) {
        int curTeam = currentEnemy.team;
        int health = get_Health(currentEnemy.Character);
        bool canSet = false;
        Vector2 newAngle;
        if (cfg.aimbot && localEnemy.Character && get_Health(localEnemy.Character) > 0 && health > 0) {
            Vector3 enemyBone = getBonePosition(currentEnemy.Character, HEAD);
            Vector3 deltavec = enemyBone - localHead;
            float deltLength = sqrt(deltavec.X * deltavec.X + deltavec.Y * deltavec.Y + deltavec.Z * deltavec.Z);
            newAngle.X = -asin(deltavec.Y / deltLength) * (180.0 / PI);
            newAngle.Y = atan2(deltavec.X, deltavec.Z) * 180.0 / PI;
            if (isInFov2(rotation, newAngle, cfg) && localTeam != curTeam && curTeam != -1) {
                if (cfg.visCheck && isCharacterVisible(currentEnemy.Character, pSys)) {
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
                    }
                }
            }
        }
    }
    return closestCharacter;
}

// Function to set the rotation for aiming
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
        Vector3 localHead = getBonePosition(localEnemy.Character, HEAD);
        if (getIsCrouched(localEnemy.Character)) {
            localHead -= Vector3(0, 0.5, 0);
        }

        Vector3 targetBone = getBonePosition(closestEnt, HEAD);
        Vector3 deltavec = targetBone - localHead;
        float deltLength = sqrt(deltavec.X * deltavec.X + deltavec.Y * deltavec.Y + deltavec.Z * deltavec.Z);
        newAngle.X = -asin(deltavec.Y / deltLength) * (180.0 / PI);
        newAngle.Y = atan2(deltavec.X, deltavec.Z) * 180.0 / PI;

        // Apply recoil compensation
        Vector2 recoilOffset = getRecoilOffset();
        newAngle -= recoilOffset;

        difference = (newAngle - rotation); // Instant aim adjustment
        oSetRotation(character, rotation + difference);
    }
}

// Function to get the current recoil offset
Vector2 getRecoilOffset() {
    // Implement logic to get the current recoil offset based on the weapon and shooting state
    // Placeholder values for recoil offset
    return Vector2(0.5, 0.5); 
}

// Function to stop player movement
void stopPlayerMovement() {
    // Implement the logic to stop player movement here
    // This is a placeholder function and needs to be implemented according to the game's movement control logic
}

// Function to check if movement should be stopped for accuracy
bool shouldStopForAccuracy(AimbotCfg cfg, Vector2 newAngle, Vector2 rotation) {
    Vector2 difference = newAngle - rotation;
    return (fabs(difference.X) > cfg.accuracyThreshold || fabs(difference.Y) > cfg.accuracyThreshold);
}

// ESP function to draw information on the screen
void ESP() {
    AimbotCfg cfg;

    std::lock_guard<std::mutex> guard(esp_mtx);
    auto background = ImGui::GetBackgroundDrawList();
    if (pSys == nullptr || !(esp || pistolCfg.aimbot || shotgunCfg.aimbot || smgCfg.aimbot || arCfg.aimbot || sniperCfg.aimbot)) {
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
                DrawLine(ImVec2(glWidth / 2, glHeight), ImVec2(transformPos.X, transformPos.Y), ImColor(255, 0, 0, (255 - currentEntDist * 2))); // Red color
            }

            if (espcfg.bone && transformPos.Z > 0 && currentCharacter != nullptr) {
                DrawBones(currentCharacter, LOWERLEG_LEFT, UPPERLEG_LEFT, espcfg, background);
                DrawBones(currentCharacter, LOWERLEG_RIGHT, UPPERLEG_RIGHT, espcfg, background);
                DrawBones(currentCharacter, UPPERLEG_LEFT, STOMACH, espcfg, background);
                DrawBones(currentCharacter, UPPERLEG_RIGHT, STOMACH, espcfg, background);
                DrawBones(currentCharacter, STOMACH, CHEST, espcfg, background
