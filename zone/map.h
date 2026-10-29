/*
	EQEMu:  Everquest Server Emulator

	Copyright (C) 2001-2014 EQEMu Development Team (http://eqemulator.net)

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; version 2 of the License.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY except by those people which sell it, which
	are required to give you total support for your newly bought product;
	without even the implied warranty of MERCHANTABILITY or FITNESS FOR
	A PARTICULAR PURPOSE.  See the GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program; if not, write to the Free Software
	Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

*/

#ifndef ZONE_MAP_H
#define ZONE_MAP_H

#include "../common/housing/curved_mount_contact.h"
#include "position.h"
#include "../common/housing/planar_mount_contact.h"
namespace Housing::OrientedMount { struct Volume; }
#include <stdio.h>

#include "zone_config.h"

#define BEST_Z_INVALID -99999

extern const ZoneConfig *Config;

class Map
{
public:
	Map();
	~Map();

	float FindBestZ(glm::vec3 &start, glm::vec3 *result) const;
	float FindClosestZ(glm::vec3 &start, glm::vec3 *result) const;
	float FindCeiling(glm::vec3 &start, glm::vec3 *result) const;
	float FindGround(glm::vec3 &start, glm::vec3 *result) const;
	bool LineIntersectsZone(glm::vec3 start, glm::vec3 end, float step, glm::vec3 *result) const;
	bool LineIntersectsZoneNoZLeaps(glm::vec3 start, glm::vec3 end, float step_mag, glm::vec3 *result) const;
	bool CheckLoS(glm::vec3 myloc, glm::vec3 oloc) const;
	bool IntersectsBox(const glm::vec3& minimum,const glm::vec3& maximum) const;
	// Housing-only trusted source contact; all other triangles remain blockers.
	bool IntersectsBoxExceptTriangles(const glm::vec3& minimum,const glm::vec3& maximum,uint32_t source_a,uint32_t source_b) const;
 std::vector<uint32_t> SourceMountPatch(const glm::vec3& minimum,const glm::vec3& maximum,uint32_t axis,uint32_t source_a,uint32_t source_b,const std::vector<uint32_t>& candidates) const;
 bool IntersectsBoxExceptTriangleList(const glm::vec3& minimum,const glm::vec3& maximum,const std::vector<uint32_t>& sources) const;
 std::vector<uint32_t> SourceMountPatch(const Housing::OrientedMount::Volume&,uint32_t axis,uint32_t source_a,uint32_t source_b,const std::vector<uint32_t>& candidates) const;
 bool IntersectsVolumeExceptTriangleList(const Housing::OrientedMount::Volume&,const std::vector<uint32_t>& sources) const;

 std::optional<Housing::PlanarMount::Trace> TracePlanarMount(const glm::vec3& start,const glm::vec3& end) const;
 Housing::CurvedMount::Proof SourceCurvedMountPatch(const Housing::OrientedMount::Volume&,const Housing::PlanarMount::SurfaceSeed&,const std::vector<Housing::CurvedMount::Entry>&,Housing::CurvedMount::Limits={}) const;
 std::vector<uint32_t> SourcePlanarMountPatch(const Housing::OrientedMount::Volume&,const Housing::PlanarMount::SurfaceSeed&,const std::vector<uint32_t>& reviewed_candidates) const;
 // Inventory metadata only. Callers must explicitly enroll exact hash-bound IDs;
 // count alone grants no surface, mode, region or collision exemption authority.
 uint32_t SourceTriangleCount() const;

	bool DoCollisionCheck(glm::vec3 myloc, glm::vec3 oloc, glm::vec3 &outnorm, float &distance) const;

#ifdef USE_MAP_MMFS
	bool Load(std::string filename, bool force_mmf_overwrite = false);
#else
	bool Load(const std::string& filename);
#endif

	static Map *LoadMapFile(std::string file);
private:
	void RotateVertex(glm::vec3 &v, float rx, float ry, float rz);
	void ScaleVertex(glm::vec3 &v, float sx, float sy, float sz);
	void TranslateVertex(glm::vec3 &v, float tx, float ty, float tz);
	bool LoadV1(FILE *f);
	bool LoadV2(FILE *f);

#ifdef USE_MAP_MMFS
	bool LoadMMF(const std::string& map_file_name, bool force_mmf_overwrite);
	bool SaveMMF(const std::string& map_file_name, bool force_mmf_overwrite);
#endif /*USE_MAP_MMFS*/

	struct impl;
	impl *imp;
};

#endif
