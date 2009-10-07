
#ifndef incl_QtModuleWrapper_h
#define incl_QtModuleWrapper_h

#include <QObject>
#include "QtModule.h"
#include "UICanvas.h"

#include "Foundation.h"

namespace PythonScript 
{
    class QtModuleWrapper : public QObject
    {

        Q_OBJECT

        public:
            QtModuleWrapper(Foundation::Framework *framework);
            ~QtModuleWrapper() {}

        public slots:

            QtUI::UICanvas* CreateCanvas(int mode);

            void DeleteCanvas(QtUI::UICanvas* canvas);

        private:
            Foundation::Framework *framework_;

    };

}

#endif