// For conditions of distribution and use, see copyright notice in license.txt

#ifndef incl_EC_TerrainPatch_h
#define incl_EC_TerrainPatch_h

#include "ComponentInterface.h"
#include "Foundation.h"
#include "RexUUID.h"

namespace Ogre
{
    class SceneNode;
}

namespace RexLogic
{
    /// Stores all the terrain patches (typically 16x16) in a given terrain.
    class EC_Terrain : public Foundation::ComponentInterface
    {
        DECLARE_EC(EC_Terrain);
    public:
        virtual ~EC_Terrain();

        /// Describes a single patch that is present in the scene.
        struct Patch
        {
            Patch():x(0),y(0), node(0), patch_geometry_dirty(true) {}

            static const int cNumVerticesPerPatchEdge = 16;

            /// X coordinate on the grid of patches. In OpenSim this is [0, 15], but might change.
            int x;

            /// [0, 15].
            int y;

            /// Typically this will be a 16x16 array of height values in world coordinates.
            /// If the length is zero, this patch hasn't been loaded in yet.
            std::vector<float> heightData;

            /// Ogre -specific: Store a reference to the actual render hierarchy node.
            Ogre::SceneNode *node;

            /// If true, the CPU-side heightmap data has changed, but we haven't yet updated
            /// the GPU-side geometry resources since the neighboring patches haven't been loaded
            /// in yet.
            bool patch_geometry_dirty;
        };

        /// The OpenSim world has a 16x16 grid of terrain patches. Alter this to change the
        /// assumption.
        static const int cNumPatchesPerEdge = 16;
        static const int cPatchSize = 16;
        
        /// @return The patch at given (x,y) coordinates. Pass in values in range [0, 15].
        Patch &GetPatch(int x, int y)
        {
            assert(x >= 0);
            assert(y >= 0);
            assert(x < cNumPatchesPerEdge);
            assert(y < cNumPatchesPerEdge);
            return patches[y][x];
        }

        /// Calculate terrain normal
        /// @param x patch horizontal number
        /// @param y patch vertical number
        /// @param xinside horizontal position within patch
        /// @param yinside vertical position within patch
        /// @return normal
        Core::Vector3df CalculateNormal(int x, int y, int xinside, int yinside);
        
        /// @return True if all the patches of the terrain are loaded.
        bool AllPatchesLoaded() const
        {
            for(int i = 0; i < cNumPatchesPerEdge; ++i)
                for(int j = 0; j < cNumPatchesPerEdge; ++j)
                    if (patches[i][j].heightData.size() == 0 || patches[i][j].node == 0)
                        return false;

            return true;
        }

        /// Removes all stored terrain patches and the associated Ogre scene nodes.
        void Destroy();

    private:
        EC_Terrain(Foundation::ModuleInterface* module);

        /// Utility to get heightmap point
        float GetPoint(int x, int y);
        
        /// Stores the actual height patches.
        Patch patches[cNumPatchesPerEdge][cNumPatchesPerEdge];

        /// Just need to have this here to be able to destroy the terrain nodes. ///\todo This pointer could theoretically dangle... 
        Foundation::ModuleInterface* owner_; 
    };
}

#endif