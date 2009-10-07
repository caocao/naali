// For conditions of distribution and use, see copyright notice in license.txt

/**
 *  @file AbstractInventoryDataModel.h
 *  @brief 
 */

#ifndef incl_InventoryModule_AbstractInventoryDataModel_h
#define incl_InventoryModule_AbstractInventoryDataModel_h

#include "AbstractInventoryItem.h"

#include <QObject>

namespace Inventory
{
    /// Pure virtual class.
    class AbstractInventoryDataModel : public QObject
    {
        Q_OBJECT

    public:
        /// Default constructor.
        AbstractInventoryDataModel() {}

        /// Destructor.
        virtual ~AbstractInventoryDataModel() {}

        ///
        virtual void AddFolder(AbstractInventoryItem *newFolder, AbstractInventoryItem *parentFolder) = 0;

        /// @return First folder by the requested name or null if the folder isn't found.
        virtual AbstractInventoryItem *GetFirstChildFolderByName(const QString &name) const = 0;

        /// @return First folder by the requested id or null if the folder isn't found.
        virtual AbstractInventoryItem *GetChildFolderByID(const QString &searchId) const = 0;

        /// Returns folder by requested id, or creates a new one if the folder doesnt exist.
        /// @param id ID.
        /// @param parent Parent folder.
        /// @return Pointer to the existing or just created folder.
        virtual AbstractInventoryItem *GetOrCreateNewFolder(const QString &id, AbstractInventoryItem &parent) = 0;

        /// Returns asset requested id, or creates a new one if the folder doesnt exist.
        /// @param inventory_id Inventory ID.
        /// @param asset_id Asset ID.
        /// @param parent Parent folder.
        /// @return Pointer to the existing or just created asset.
        virtual AbstractInventoryItem *GetOrCreateNewAsset(const QString &inventory_id, const QString &asset_id,
            AbstractInventoryItem &parent, const QString &name) = 0;

    private:
        Q_DISABLE_COPY(AbstractInventoryDataModel);
    };
}
#endif