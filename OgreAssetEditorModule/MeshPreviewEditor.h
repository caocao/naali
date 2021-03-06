#ifndef incl_OgreAssetEditorModule_MeshPreviewEditor_h
#define incl_OgreAssetEditorModule_MeshPreviewEditor_h

#include <RexTypes.h>
#include <QWidget>
#include <QLabel>

#include <boost/shared_ptr.hpp>

QT_BEGIN_NAMESPACE
class QPushButton;
QT_END_NAMESPACE

namespace Foundation
{
    class Framework;
    class AssetInterface;
    typedef boost::shared_ptr<AssetInterface> AssetPtr;
}

namespace Naali
{
    //! Label is used to display the mesh in image format.
    class MeshPreviewLabel: public QLabel
    {
        Q_OBJECT
    public:
        MeshPreviewLabel(QWidget *parent = 0, Qt::WindowFlags flags = 0);
        virtual ~MeshPreviewLabel();
    };

    //! AudioPreviewEditor is used to play different audioclips from the inventory and show audio info.
    class MeshPreviewEditor: public QWidget
    {
        Q_OBJECT
    public:

        MeshPreviewEditor(Foundation::Framework *framework,
                           const QString &inventory_id,
                           const asset_type_t &asset_type,
                           const QString &name,
                           QWidget *parent = 0);
        virtual ~MeshPreviewEditor();

        void HandleAssetReady(Foundation::AssetPtr asset);

    public slots:
        /// Close the window.
        void Closed();

    signals:
        /// This signal is emitted when the editor is closed.
        void Closed(const QString &inventory_id, asset_type_t asset_type);

    private:
        void InitializeEditorWidget();

        Foundation::Framework *framework_;
        asset_type_t assetType_;
        QString inventoryId_;

        QWidget     *mainWidget_;
        QPushButton *okButton_;
    };
}

#endif