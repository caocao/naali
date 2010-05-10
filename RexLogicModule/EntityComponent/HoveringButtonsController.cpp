#include "StableHeaders.h"
#include "HoveringButtonsController.h"
#include <QPushButton>
#include <QMouseEvent>
#include <QDebug>
#include "RexLogicModule.h"

namespace RexLogic
{

    HoveringButtonsController::HoveringButtonsController()
    :text_padding_(20.0f)
    {
        Ui::HoveringButtonsWidget::setupUi(this);
    }

    HoveringButtonsController::~HoveringButtonsController()
    {

    }

    void HoveringButtonsController::ButtonPressed()
    {
        RexLogicModule::LogInfo( "Poke!" );
    }

    void HoveringButtonsController::ForwardMouseClickEvent(Real x, Real y)
    { 
        //To widgetspace
        Real x_widgetspace = x*(Real)this->width();
        Real y_widgetspace = y*(Real)this->height();

<<<<<<< HEAD

=======
>>>>>>> 5aa6bd5698a8734b6fe9ce7cb684b848eb04a506
        QPoint pos(x_widgetspace,y_widgetspace);

        QMouseEvent e(QEvent::MouseButtonPress, pos, Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
        QApplication::sendEvent(this, &e);
        //Iterate through buttons and check if they are pressed 
        for(int i = 0; i< buttongroup->layout()->count();i++)
        {
            
            QLayoutItem* item = buttongroup->layout()->itemAt(i);
            QAbstractButton *button = dynamic_cast<QAbstractButton*>(item->widget());
            if(button)
            {

                if(button->rect().contains(button->mapFromParent(pos)))
                {
                    QMouseEvent e(QEvent::MouseButtonPress, pos - button->pos(),pos, Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
                    QApplication::sendEvent(button, &e);
                }
            }
        }

        
    }


    void HoveringButtonsController::AddButton(QPushButton *button)
    {
       button->setParent(this);
       button->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
       button->setMaximumHeight(70);
       button->setMaximumWidth(140);
       QFont font(button->font());
       QRect rect = button->fontMetrics().tightBoundingRect(button->text());

       qreal scale = (button->maximumWidth() - text_padding_)/rect.width();

       font.setPointSize(font.pointSizeF() * scale);
       button->setFont(font);
       QBoxLayout* layout = dynamic_cast<QBoxLayout*>(this->buttongroup->layout());
       if(!layout)
           return;
       layout->insertWidget(0,button);
       QObject::connect(button, SIGNAL(pressed()), this, SLOT(ButtonPressed()));
       button->show();
    }
}
