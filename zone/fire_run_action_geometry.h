#pragma once
#include "fire_run_action_scene.h"
#include <glm/glm.hpp>
#include <array>
// Selected LOCAL source geometry and logical reservations; no serving activation.
namespace FireRunActionSource {
struct GateBinding {GateRole role;uint32_t database_id;uint8_t native_id;uint32_t version;glm::vec4 pose;uint16_t size,lockpick;uint32_t key;uint8_t nokeyring;bool disable_timer;glm::vec3 low,high,before,after;};
inline const std::array<GateBinding,7>& BoundGates(){static const std::array<GateBinding,7> values{{
{GateRole::InnerVault,304062000,1,1,glm::vec4(0.0f,283.35f,12.7702f,384.0f),85,32767,0,1,true,glm::vec3(-20.0f,263.35f,6.0002f),glm::vec3(20.0f,303.35f,30.0002f),glm::vec3(0.0f,273.35f,16.7502f),glm::vec3(0.0f,293.35f,16.7502f)},
{GateRole::WorshipLeft,304062001,2,1,glm::vec4(-236.0f,749.5f,11.77f,332.81044f),85,32767,0,1,true,glm::vec3(-256.0f,729.5f,5.0f),glm::vec3(-216.0f,769.5f,29.0f),glm::vec3(-230.1232f,741.4091f,15.75f),glm::vec3(-241.8768f,757.5909f,15.75f)},
{GateRole::WorshipRight,304062002,3,1,glm::vec4(236.0f,749.5f,11.77f,435.18956f),85,32767,0,1,true,glm::vec3(216.0f,729.5f,5.0f),glm::vec3(256.0f,769.5f,29.0f),glm::vec3(230.1232f,741.4091f,15.75f),glm::vec3(241.8768f,757.5909f,15.75f)},
{GateRole::KeymasterPool,304062003,4,1,glm::vec4(-251.175f,1020.55f,-3.23f,412.54685f),85,32767,0,1,true,glm::vec3(-271.175f,1000.55f,-10.0f),glm::vec3(-231.175f,1040.55f,14.0f),glm::vec3(-254.607f,1011.1574f,0.75f),glm::vec3(-247.743f,1029.9426f,0.75f)},
{GateRole::KeymasterGarden,304062004,5,1,glm::vec4(251.175f,1020.55f,-3.23f,355.45315f),85,32767,0,1,true,glm::vec3(231.175f,1000.55f,-10.0f),glm::vec3(271.175f,1040.55f,14.0f),glm::vec3(254.607f,1011.1574f,0.75f),glm::vec3(247.743f,1029.9426f,0.75f)},
{GateRole::GuardianVault,304062005,6,1,glm::vec4(0.06667f,711.594f,22.77f,384.0f),100,32767,47757,1,true,glm::vec3(-19.93333f,691.594f,16.0f),glm::vec3(20.06667f,731.594f,40.0f),glm::vec3(0.0667f,701.594f,26.75f),glm::vec3(0.0667f,721.594f,26.75f)},
{GateRole::WorldFont,35650,7,0,glm::vec4(0.06667f,711.594f,22.77f,384.0f),100,32767,47759,1,false,glm::vec3(0.06667f,711.594f,22.77f),glm::vec3(0.06667f,711.594f,22.77f),glm::vec3(0.06667f,711.594f,22.77f),glm::vec3(0.06667f,711.594f,22.77f)}
}};return values;}
struct HelperRouteBinding {GateRole target;std::array<glm::vec3,4> anchors;glm::vec3 low,high,passage;};
inline const std::array<HelperRouteBinding,2>& BoundRoutes(){static const std::array<HelperRouteBinding,2> values{{
{GateRole::KeymasterPool,{{glm::vec3(-280.0f,970.0f,0.75f),glm::vec3(-274.0f,986.0f,-2.25f),glm::vec3(-264.0f,1000.0f,0.75f),glm::vec3(-255.2934f,1009.27888f,0.75f)}},glm::vec3(-295.0f,955.0f,-5.25f),glm::vec3(-265.0f,985.0f,6.75f),glm::vec3(-247.743f,1029.9426f,0.75f)},
{GateRole::KeymasterGarden,{{glm::vec3(280.0f,970.0f,0.2093f),glm::vec3(274.0f,986.0f,-0.3919f),glm::vec3(264.0f,1000.0f,0.75f),glm::vec3(255.2934f,1009.27888f,0.75f)}},glm::vec3(265.0f,955.0f,-5.7907f),glm::vec3(295.0f,985.0f,6.2093f),glm::vec3(247.743f,1029.9426f,0.75f)},
}};return values;}

struct MaterialSpawnBinding {uint32_t spawn,npc,item;int activity;};
inline const std::array<MaterialSpawnBinding,15>& BoundMaterialSpawns(){static const std::array<MaterialSpawnBinding,15> values{{
{304062000,490900,47748,0},
{304062001,490900,47748,0},
{304062002,490901,47749,1},
{304062003,490901,47749,1},
{304062004,490902,47750,2},
{304062005,490902,47750,2},
{304062006,490903,47761,4},
{304062007,490904,47760,5},
{304062008,490905,47751,8},
{304062009,490906,47752,9},
{304062010,490907,47753,10},
{304062011,490908,47754,11},
{304062012,490909,47755,12},
{304062013,490910,47757,15},
{304062014,490911,47758,17},
}};return values;}
} // namespace FireRunActionSource
