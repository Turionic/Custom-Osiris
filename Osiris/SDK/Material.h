#pragma once

#include <tuple>

#include "VirtualMethod.h"

class MaterialVar {
public:
    VIRTUAL_METHOD(void, setValue, 4, (float value), (this, value))
    VIRTUAL_METHOD(void, setVectorValue, 11, (float x, float y, float z), (this, x, y, z))
    VIRTUAL_METHOD(void, setVecComponentValue, 26, (float value, int component), (this, value, component))
};

enum class MaterialVarFlag {
    NO_DRAW = 1 << 2,


	MATERIAL_VAR_VERTEXCOLOR = (1 << 4),
	MATERIAL_VAR_VERTEXALPHA = (1 << 5),
	MATERIAL_VAR_SELFILLUM = (1 << 6),
	MATERIAL_VAR_ADDITIVE = (1 << 7),
	MATERIAL_VAR_ALPHATEST = (1 << 8),

	IGNOREZ = 1 << 15,
	WIREFRAME = 1 << 28

};

enum MaterialVarFlags_t
{
	MATERIAL_VAR_DEBUG = (1 << 0),
	MATERIAL_VAR_NO_DEBUG_OVERRIDE = (1 << 1),
	MATERIAL_VAR_NO_DRAW = (1 << 2),
	MATERIAL_VAR_USE_IN_FILLRATE_MODE = (1 << 3),

	MATERIAL_VAR_VERTEXCOLOR = (1 << 4),
	MATERIAL_VAR_VERTEXALPHA = (1 << 5),
	MATERIAL_VAR_SELFILLUM = (1 << 6),
	MATERIAL_VAR_ADDITIVE = (1 << 7),
	MATERIAL_VAR_ALPHATEST = (1 << 8),
	//	MATERIAL_VAR_UNUSED					  = (1 << 9),
	MATERIAL_VAR_ZNEARER = (1 << 10),
	MATERIAL_VAR_MODEL = (1 << 11),
	MATERIAL_VAR_FLAT = (1 << 12),
	MATERIAL_VAR_NOCULL = (1 << 13),
	MATERIAL_VAR_NOFOG = (1 << 14),
	MATERIAL_VAR_IGNOREZ = (1 << 15),
	MATERIAL_VAR_DECAL = (1 << 16),
	MATERIAL_VAR_ENVMAPSPHERE = (1 << 17), // OBSOLETE
//	MATERIAL_VAR_UNUSED					  = (1 << 18),
	MATERIAL_VAR_ENVMAPCAMERASPACE = (1 << 19), // OBSOLETE
	MATERIAL_VAR_BASEALPHAENVMAPMASK = (1 << 20),
	MATERIAL_VAR_TRANSLUCENT = (1 << 21),
	MATERIAL_VAR_NORMALMAPALPHAENVMAPMASK = (1 << 22),
	MATERIAL_VAR_NEEDS_SOFTWARE_SKINNING = (1 << 23), // OBSOLETE
	MATERIAL_VAR_OPAQUETEXTURE = (1 << 24),
	MATERIAL_VAR_ENVMAPMODE = (1 << 25), // OBSOLETE
	MATERIAL_VAR_SUPPRESS_DECALS = (1 << 26),
	MATERIAL_VAR_HALFLAMBERT = (1 << 27),
	MATERIAL_VAR_WIREFRAME = (1 << 28),
	MATERIAL_VAR_ALLOWALPHATOCOVERAGE = (1 << 29),
	MATERIAL_VAR_ALPHA_MODIFIED_BY_PROXY = (1 << 30),
	MATERIAL_VAR_VERTEXFOG = (1 << 31),
// NOTE: Only add flags here that either should be read from
// .vmts or can be set directly from client code. Other, internal
// flags should to into the flag enum in IMaterialInternal.h
};

class Material {
public:
    VIRTUAL_METHOD(const char*, getName, 0, (), (this))
    VIRTUAL_METHOD(const char*, getTextureGroupName, 1, (), (this))
    VIRTUAL_METHOD(MaterialVar*, findVar, 11, (const char* name, bool* found = nullptr, bool complain = true), (this, name, found, complain))
    VIRTUAL_METHOD(void, incrementReferenceCount, 12, (), (this))
    VIRTUAL_METHOD(void, decrementReferenceCount, 13, (), (this))
    VIRTUAL_METHOD(void, alphaModulate, 27, (float alpha), (this, alpha))
    VIRTUAL_METHOD(void, colorModulate, 28, (const std::array<float, 3>& color), (this, color[0], color[1], color[2]))
    VIRTUAL_METHOD(void, colorModulate, 28, (float r, float g, float b), (this, r, g, b))
    VIRTUAL_METHOD(void, colorModulate, 28, (const std::tuple<float, float, float>& color), (this, std::get<0>(color), std::get<1>(color), std::get<2>(color)))
    VIRTUAL_METHOD(void, setMaterialVarFlag, 29, (MaterialVarFlag flag, bool on), (this, flag, on))
    VIRTUAL_METHOD(int, getReferenceCount, 56, (), (this))
    VIRTUAL_METHOD(bool, isPrecached, 70, (), (this))
};
