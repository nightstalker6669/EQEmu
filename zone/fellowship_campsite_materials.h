#ifndef EQEMU_ZONE_FELLOWSHIP_CAMPSITE_MATERIALS_H
#define EQEMU_ZONE_FELLOWSHIP_CAMPSITE_MATERIALS_H
#include "../common/fellowship/campsite_catalogue.h"
#include "../common/fellowship/service.h"
class Client;
bool HasFellowshipCampsiteMaterials(Client&,const Fellowship::Campsites::Definition&);
Fellowship::Result<Fellowship::Snapshot> CreatePaidFellowshipCampsite(Client&,const Fellowship::Campsites::Definition&,const Fellowship::Campfire&,uint64_t now,uint64_t expected_revision,uint32_t expected_fellowship_id);
#endif
