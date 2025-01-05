struct Ray
{
    Vector3 origin;
    Vector3 direction;
};

enum BodyPart
{
    LOWERLEG_LEFT,
    LOWERLEG_RIGHT,
    UPPERLEG_LEFT,
    UPPERLEG_RIGHT,
    STOMACH,
    CHEST,
    UPPERARM_LEFT,
    UPPERARM_RIGHT,
    LOWERARM_LEFT,
    LOWERARM_RIGHT,
    HEAD
};

struct PlayerAdapter
{
    int pad_0[3];
    void* Player; // 0x10
};

enum WeaponCategory
{
    Pistol,
    AssaultRifles,
    SubmachineGun,
    Shotgun,
    SniperRifle,
    Melee,
    Grenade,
    Utility
};

struct HitData
{
    bool hitCharacter;
    bool traced;
    Vector3 hitWorldPos;
    int victimID;
    BodyPart hitBodyPart;
    Vector3 hitLocalPos;
    int hitAngle;
    Vector3 hitWorldNormal;
    int hitMaterialDef;
};

struct Character
{
    void* character;
    int id;
};

enum ChatMessageType
{
    PUBLIC_CHAT,
    TEAM_CHAT,
    RADIO,
    NOTIFICATION,
    LOCAL_NOTIFICATION,
    BROADCAST,
    FRIENDS,
    PARTY,
    CLAN
};

struct Enemy
{
    void* Character = nullptr;
    void* Player = nullptr;
    std::string Name = "";
    std::string Clan = "";
    int team = -1;
    bool local = false;
};
std::vector<Enemy> EnemyList;

struct TransformData
{
    // Pads may be wrong :(
    Vector3 pos;
    int pad_0[2];
    Vector3 velocity;
    int pad_1[3];
    Vector2 rotation;
};


struct ESPCfg {
    bool snapline = true;
    ImVec4 snaplineColor = ImColor(255,255,255);
    bool bone = true;
    ImVec4 boneColor = ImColor(255,255,255);
    bool box = true;
    ImVec4 boxColor = ImColor(255,255,255);
    bool healthesp = true;
    bool healthNumber = true;
    bool name = true;
    bool distance = true;
    ImVec4 nameColor = ImColor(255,255,255);
    bool weapon = true;
    ImVec4 weaponColor = ImColor(255,255,255);
};
struct BurstData
{
    Ray currentRay;
    void* weaponEgid;
    int damagePercent;
    bool trace;
    float maxRange;
};

struct AimbotCfg {
    bool aimbot = true;
    bool visCheck = true;
    std::vector<BodyPart> aimBones = {HEAD, CHEST, STOMACH}; // Target head, chest, and stomach
    bool aimbotSmooth = true;
    float smoothAmount = 1.0f;
    bool fovCheck = true;
    float fovValue = 360.0f;
    bool drawFov = true;
    bool onShoot = true;
    bool triggerbot = true;
};

struct CustomWeapon
{
    void* EGID = 0;
    int liveId = -1;
    int weaponDefId = -1;
};

struct Color
{
    float r;
    float g;
    float b;
    float a;
};
