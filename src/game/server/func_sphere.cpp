// -------------------------------------------------------------------------------------------------
// Code written by Gareth Pugh, uses env_sprite and env_beam entities to create a decorative sphere.
// -------------------------------------------------------------------------------------------------
// May crash the game when too many sprites/beams are created!
// -------------------------------------------------------------------------------------------------

#include "cbase.h"			// Mandatory include.
#include "Sprite.h"			// Needed for sprite drawing.
#include "beam_shared.h"	// Needed for beam drawing.
#include "tier0/memdbgon.h" // Last file to be included.

// -----------------------------------------------------------------
// Creates a sphere of sprites with beams drawn between every point.
// -----------------------------------------------------------------
class CFuncSphere : public CBaseEntity
{
public:
	DECLARE_CLASS(CFuncSphere, CBaseEntity);
	DECLARE_DATADESC();

	void Precache(void);		// Initialize any materials that will be used.
	void Spawn(void);			// What happens when the entity is created?
	void SetupPoints(float r);	// Create the sphere with radius r.

private:
	float radius;				// Radius of the sphere in world units.
	Vector zeroPoint;			// Origin point of the entity.
};

LINK_ENTITY_TO_CLASS(func_sphere, CFuncSphere);

BEGIN_DATADESC(CFuncSphere)
// Keyfields need to be defined in the Hammer editor!
DEFINE_KEYFIELD(radius, FIELD_FLOAT, "radius"),

// Standard fields are defined like this.
DEFINE_FIELD(zeroPoint, FIELD_POSITION_VECTOR),
END_DATADESC()


// --------------------------
// Initialize materials here.
// --------------------------
void CFuncSphere::Precache(void)
{
	BaseClass::Precache();
	PrecacheModel("sprites/glow01.vmt");	// The material choices are arbitrary.
	PrecacheModel("sprites/laserbeam.vmt");
}

// ----------------------
// Entity spawning stuff.
// ----------------------
void CFuncSphere::Spawn(void)
{
	BaseClass::Spawn();
	zeroPoint = GetAbsOrigin();	// Acquire the origin of the entity.
	SetupPoints(radius);		// Create the sphere!
}

// --------------------------------------------------------------
// Use spherical polar coordinates to create a sphere of sprites.
// --------------------------------------------------------------
void CFuncSphere::SetupPoints(float r)
{
	const int noHorizAngles = 8; // The number of horizontal angles to use.
	const int noVertAngles = 10;  // The number of vertical angles to use.

	const float horizAngle = 360 / noHorizAngles; // The size of each horizontal angle.
	const float vertAngle = 180 / noVertAngles;	  // The size of each vertical angle.

	Vector positions[noVertAngles + 1][noHorizAngles];	// Defines a matrix of Vectors that will be points on the sphere.
	CSprite* pSprites[noVertAngles + 1][noHorizAngles];	// A matrix of sprites that will be drawn at each positions Vector.

	// --------------------------------------------------------------------------------------
	// Beams don't parent to the func_sphere properly, parent them to a dummy sprite instead!
	// --------------------------------------------------------------------------------------
	CSprite* invOrig = CSprite::SpriteCreate("sprites/glow01.vmt", zeroPoint, false); // Create a stock sprite.
	invOrig->TurnOff(); // invOrig == Invisible Origin, exists at func_sphere entity = zeroPoint vector.

	// -----------------------------------------------------------------------------------------------
	// Iterate over every vertical/horizontal angle and set the position of every point on the sphere.
	// -----------------------------------------------------------------------------------------------
	for (int i = 0; i <= noVertAngles; i++)
	{
		for (int j = 0; j < noHorizAngles; j++)
		{
			float vAngToUse = i * vertAngle; // The current vertical angle.
			int f = (int)vAngToUse;		   // Integer version of the current vertical angle.

			// Don't define additional points at the top/bottom of the sphere!
			if ((f == 0 || f % 180 == 0) && i >= 0 && j > 0)
				break;

			// --------------------------------------------------------------------------------------------------------
			// Perform the calculations to get each position of the sphere, then create a sprite at that position.
			// Spherical polar coordinates are: x = r*sin(theta)*cos(phi), y = r*sin(theta)*sin(phi), z = r*cos(theta).
			// Where r is the radius of the sphere, theta is the vertical angle and phi is the horizontal angle.
			// --------------------------------------------------------------------------------------------------------
			positions[i][j].x = zeroPoint.x + r * sin(DEG2RAD(i * vertAngle)) * cos(DEG2RAD(j * horizAngle));
			positions[i][j].y = zeroPoint.y + r * sin(DEG2RAD(i * vertAngle)) * sin(DEG2RAD(j * horizAngle));
			positions[i][j].z = zeroPoint.z + r * cos(DEG2RAD(i * vertAngle));

			pSprites[i][j] = CSprite::SpriteCreate("sprites/glow01.vmt", positions[i][j], false);
			pSprites[i][j]->SetScale(0.5f); // Scale and transparency effects are arbitrary.
			pSprites[i][j]->SetTransparency(kRenderGlow, 0, 0, 255, 255, kRenderFxNoDissipation);
			pSprites[i][j]->SetParent(this); // Parent the sprites to the func_sphere entity.

			// -----------------------------------------------
			// Print the position of each point on the sphere.
			// -----------------------------------------------
			Msg("positions[%i][%i] are at: %.4f %.4f %.4f\n", i, j, positions[i][j].x, positions[i][j].y, positions[i][j].z);
		}
	}

	// --------------------------------------------
	// Draw beams between each point on the sphere.
	// --------------------------------------------
	const int beamSegments = 4; // 0 = Horizontal, 1 = Vertical, 2 = Top, 3 = Bottom.
	CBeam* pBeams[beamSegments][noVertAngles][noHorizAngles]; // 3D matrix of beams!

	// --------------------------------------------------------------------------------------------
	// Iterate over every beam segment/vertical/horizontal angle and draw beams between each point.
	// --------------------------------------------------------------------------------------------
	for (int k = 0; k < beamSegments; k++)
	{
		for (int i = 0; i < noVertAngles; i++)
		{
			for (int j = 0; j < noHorizAngles; j++)
			{
				float vAngToUse = i * vertAngle; // The current vertical angle.
				int f = (int)vAngToUse;		   // Integer version of the current vertical angle.

				// Don't draw beams between points at the very top/bottom of the sphere!
				if (f == 0 || f % 180 == 0)
					break;

				// Create each beam with an arbitrary material and width.
				pBeams[k][i][j] = CBeam::BeamCreate("sprites/laserbeam.vmt", 0.5f);

				switch (k) // If, else if, else if, else if etc.
				{
				case 0: // Horizontal angles.
				{
					if (j < noHorizAngles - 1)
						pBeams[0][i][j]->EntsInit(pSprites[i][j], pSprites[i][j + 1]);
					else
						pBeams[0][i][j]->EntsInit(pSprites[i][j], pSprites[i][0]);
					break;
				}
				case 1: // Vertical angles.
				{
					if (i < noVertAngles - 1)
						pBeams[1][i][j]->EntsInit(pSprites[i][j], pSprites[i + 1][j]);
					break;
				}
				case 2: // Top of the sphere.
				{
					pBeams[2][i][j]->EntsInit(pSprites[0][0], pSprites[1][j]);
					break;
				}
				case 3: // Bottom of the sphere.
				{
					pBeams[3][i][j]->EntsInit(pSprites[noVertAngles][0], pSprites[noVertAngles - 1][j]);
					break;
				}
				default: // If we have an invalid case, for some reason.
				{
					break;
				}
				}

				// -------------------------------------
				// Parent the beams to the stock sprite.
				// -------------------------------------
				pBeams[k][i][j]->SetBrightness(255); // Brightness and colour values are arbitrary.
				pBeams[k][i][j]->SetColor(255, 255, 255);
				pBeams[k][i][j]->SetParent(invOrig);
			}
		}
	}
}