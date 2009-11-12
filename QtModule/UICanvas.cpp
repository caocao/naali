// For conditions of distribution and use, see copyright notice in license.txt

#include "StableHeaders.h"

#include "DebugOperatorNew.h"

#include "UICanvas.h"
#include "Profiler.h"

#ifndef Q_WS_WIN
#include <QX11Info>
#endif

#include <QCoreApplication>
#include <QPicture>
#include <QGraphicsSceneEvent>
#include <QUuid>
#include <QWidget>
#include <QGraphicsProxyWidget>
#include <QDebug>
#include <Ogre.h>
#include <QSize>

#include "UILocationPolicy.h"
#include "UIAppearPolicy.h"

#include <OgreHardwarePixelBuffer.h>
#include <OgreTexture.h>
#include <OgreMaterial.h>
#include <OgreTextAreaOverlayElement.h>
#include <OgreFontManager.h> 
#include <OgrePanelOverlayElement.h>
#include <OgreTextureUnitState.h>

#include "MemoryLeakCheck.h"


namespace QtUI
{

UICanvas::UICanvas(): dirty_(true),
                      renderwindow_changed_(false),
                      surfaceName_(""),
                      overlay_(0),
                      container_(0),
                      state_(0),
                      renderWindowSize_(QSize()),
                      mode_(External),
                      id_(QUuid::createUuid().toString()),
                      locationPolicy_(new UILocationPolicy),
                      appearPolicy_(new UIAppearPolicy),
                      view_(new QGraphicsView)
                   

                     
                      
{
 
    view_->setScene(new QGraphicsScene);
    view_->setViewportUpdateMode(QGraphicsView::FullViewportUpdate);

    QObject::connect(view_->scene(),SIGNAL(changed(const QList<QRectF>&)),this,SLOT(Redraw()));

}

UICanvas::UICanvas(DisplayMode mode, const QSize& parentWindowSize): 
    dirty_(true),
    renderwindow_changed_(false),
    surfaceName_(""),
    overlay_(0),
    container_(0),
    state_(0),
    renderWindowSize_(parentWindowSize),
    mode_(mode),
    id_(QUuid::createUuid().toString()),
    locationPolicy_(new UILocationPolicy),
    appearPolicy_(new UIAppearPolicy),
    view_(new QGraphicsView)
{
    
    view_->setScene(new QGraphicsScene);
    view_->setViewportUpdateMode(QGraphicsView::FullViewportUpdate);
    
    if (mode_ == External) 
    {
        // Deal canvas as normal QWidget. 
        // Currently do nothing
       
    }
    else if (mode_ == Internal)
    {
       
        QSize size = view_->size();
        
        CreateOgreResources(size.width(), size.height());
        QObject::connect(view_->scene(),SIGNAL(changed(const QList<QRectF>&)),this,SLOT(Redraw()));
        
    }
}


UICanvas::~UICanvas()
{
    for (int i = scene_widgets_.size(); i--;)
    {
        QGraphicsProxyWidget* widget = scene_widgets_.takeAt(i);
        delete widget;
    }

    if (mode_ != External || 
        container_ != 0 ||
        overlay_ != 0 ||
        state_ != 0) 
    {
        Ogre::TextureManager::getSingleton().remove(surfaceName_.toStdString().c_str());
        QString surfaceMaterial = QString("mat") + id_;
        Ogre::MaterialManager::getSingleton().remove(surfaceMaterial.toStdString().c_str());

        QString containerName = QString("con") + id_;
        Ogre::OverlayManager::getSingleton().destroyOverlayElement(containerName.toStdString().c_str());

        QString overlayName = QString("over") + id_;
        Ogre::OverlayManager::getSingleton().destroy(overlayName.toStdString().c_str());
    }

    container_ = 0;
    overlay_ = 0;
    
    delete locationPolicy_;
    locationPolicy_ = 0;
    delete appearPolicy_;
    appearPolicy_ = 0;
    delete view_;
    view_ = 0;
    /*
    QGraphicsScene* scene = view_->scene();
    delete scene;
    scene = 0;
    */
}

QPointF UICanvas::GetPosition() const
{
    QPointF position;

    switch(mode_)
    {
    case External:
    {
        position = view_->pos();
        break;
    }
    case Internal:
    {
        double width = container_->getLeft();
        double height = container_->getTop();
        
        // Calculate position in render window coordinate frame. 
        position.rx() = width * renderWindowSize_.width();
        position.ry() = height * renderWindowSize_.height();
        break;
    }
    default:
        break;
    }

    return position;
}

void UICanvas::Resize(int height, int width, Corner anchor)
{
    if ( mode_ == External ) 
    {
        SetSize(width, height);
        return;
    }

    // Case when canvas resize is locked. 

    if ( !appearPolicy_->IsResizable())
        return;

    QSize current_size = view_->size();

    // Assure that new size is bigger then canvas minium size or smaller then maximum size.  

    QSize minium = view_->minimumSize();
    if ( height < minium.height() || width < minium.width() || height <= 0 || width <= 0)
        return;

    QSize maximum = view_->maximumSize();
    
    if ( height > maximum.height() || width > maximum.width() )
        return;

    // Ensure that ogre surface is large enough

    Ogre::TexturePtr texture = Ogre::TextureManager::getSingleton().getByName(surfaceName_.toStdString().c_str());
    if ( (texture->getWidth() < width || texture->getHeight() < height) && ( width/2 < 2048 && height/2 < 2048 ))
          ResizeOgreTexture(2*width, 2*height);        
          
    Ogre::PanelOverlayElement* element = static_cast<Ogre::PanelOverlayElement* >(container_);  
    Ogre::Real u1 = 0, v1 = 0, u2 = 1, v2 = 1;
    Ogre::Real top = element->getTop();
    Ogre::Real left = element->getLeft();
   
    int diffWidth = current_size.width() - width;
    int diffHeight = current_size.height() - height;
   

    // Adjust overlay position and dimension. 

    Ogre::Real xPos = left, yPos = top;

    if ( anchor == TopRight || anchor == BottomRight)
        xPos = left + diffWidth/Ogre::Real(renderWindowSize_.width());
    
    
    if ( anchor == BottomRight || anchor == BottomLeft)
        yPos = top + diffHeight/Ogre::Real(renderWindowSize_.height());  
    
    float relWidth = (float)width/double(renderWindowSize_.width());
    float relHeight = (float)height/double(renderWindowSize_.height());

    container_->setDimensions(relWidth, relHeight);  
    element->setPosition(xPos,yPos);
           
    // Update uv-mapping.
    // It seems that Ogre does this itself (even when the texture is larger than what should be displayed),
    // so don't do it ourselves. Currently the texture will get a bit blurry when dragging.
    
    //Ogre::Real texture_width = texture->getWidth();
    //Ogre::Real texture_height = texture->getHeight();
    //
    //u2 = width/texture_width;
    //v2 = height/texture_height;  
    //element->setUV(u1, v1, u2, v2);
        
    // Ensure Qt-internals

    view_->resize(width,height);
   
    ResizeWidgets(width, height);

    dirty_ = true;
    RenderSceneToOgreSurface();

}

QPoint UICanvas::MapToCanvas(int x, int y)
{
    QPoint pos(x,y);
    
    if ( mode_ != External)
    {
        x -= container_->getLeft() * renderWindowSize_.width();
        y -= container_->getTop() * renderWindowSize_.height();
        pos = view_->mapToScene(QPoint(x,y)).toPoint();
    }
 
    return pos;

}

/*
QPoint UICanvas::MapToCanvas(int x, int y)
{
    if ( mode_ != External)
    {
        x -= container_->getLeft() * renderWindowSize_.width();
        y -= container_->getTop() * renderWindowSize_.height();
    }
 
    return QPoint(x,y);
}
*/

void UICanvas::Activate()
{
    if ( mode_ == External)
        return;

#ifdef Q_WS_WIN
    QSize current_size = view_->size();
    //Qt::FramelessWindowHint
    view_->setWindowFlags(Qt::FramelessWindowHint);
    SetSize(1,1);
    view_->show();
    WId win_id = view_->effectiveWinId();
    ShowWindow(static_cast<HWND>(win_id),SW_HIDE);
    SetSize(current_size.width(), current_size.height());
#endif 

}

void UICanvas::BringToTop()
{
    emit ToTop(id_);
}

void UICanvas::PushToBack()
{
    emit ToBack(id_);
}

void UICanvas::SetSize(int width, int height)
{
    view_->resize(width, height);

    if ( mode_ != External)
    {
        CreateOgreResources(width, height);
        Ogre::TexturePtr texture = Ogre::TextureManager::getSingleton().getByName(surfaceName_.toStdString().c_str());
        float relWidth = (float)texture->getWidth()/double(renderWindowSize_.width());
        float relHeight = (float)texture->getHeight()/double(renderWindowSize_.height());
        container_->setDimensions(relWidth, relHeight);
        Ogre::PanelOverlayElement* element = static_cast<Ogre::PanelOverlayElement* >(container_);  
        element->setUV(0.0f, 0.0f, 1.0f, 1.0f);    
        
        ResizeWidgets(width, height);
    }

    // Repaint canvas. 
    dirty_ = true;
    RenderSceneToOgreSurface();
}

void UICanvas::SetPosition(int x, int y)
{
    switch (mode_)
    {
    case External:
    {
        view_->move(x,y);
        break;
    }
    case Internal:
    {
        float relX = x/double(renderWindowSize_.width()), relY = y/double(renderWindowSize_.height());
        container_->setPosition(relX, relY);
        // todo BUG ?
        view_->move(x,y);
        dirty_ = true;
        break;
    }
    default:
        break;
    }
}

bool UICanvas::IsHidden() const
{
    //todo Refactor so that it uses AppearPolicy.. so that it resolves hidden state depending animation state.
    if (mode_ != External)
    {
        if (overlay_)
            return !overlay_->isVisible(); 
        else
            return true;
    }
    else
    {
        return !view_->isVisible();
    }
}

void UICanvas::Render()
{
    
    if ( mode_ != External && container_->isVisible())
    {
        //todo use AppearPolycy
        /*
        if ( fade_ )
        {
            int time = clock_.elapsed();
            if ( time != 0)
            {
                double timeSinceLastFrame = double((time - lastTime_))/1000.0;
                lastTime_ = time;
                Fade(timeSinceLastFrame);
            }
         
        }
        */

        RenderSceneToOgreSurface();    
    }
}

void UICanvas::SetRenderWindowSize(const QSize& size)
{
    QPoint oldpos = view_->pos();
    
    renderWindowSize_ = size;
   
    if ( mode_ != External)
    {  
        Ogre::TexturePtr texture = Ogre::TextureManager::getSingleton().getByName(surfaceName_.toStdString().c_str());
        float relWidth = (float)texture->getWidth()/double(renderWindowSize_.width());
        float relHeight = (float)texture->getHeight()/double(renderWindowSize_.height());
        container_->setDimensions(relWidth, relHeight);
     
        // Reset position so that canvas stays pixel-perfect
        SetPosition(oldpos.x(), oldpos.y());

        renderwindow_changed_ = true;          
        emit( RenderWindowSizeChanged(renderWindowSize_) );
    }
}

void UICanvas::Show()
{
    if ( mode_ != External)
    {
        
        QList<QGraphicsProxyWidget* >::iterator iter = scene_widgets_.begin();
        for (; iter != scene_widgets_.end(); ++iter)
            (*iter)->show();
        
        //todo use appearpolicy here.
        /*
        if ( use_fading_ )
        {
            alpha_ = 0.0;
            current_dur_ = 0.0;
            fade_ = true;
            state_->setAlphaOperation( Ogre::LBX_MODULATE, Ogre::LBS_MANUAL, Ogre::LBS_TEXTURE, alpha_);
        
            clock_.start();
        }
        */
        container_->show();
        overlay_->show();
      

        dirty_ = true;
        RenderSceneToOgreSurface();
    }
    else
    {
        
        
        QList<QGraphicsProxyWidget* >::iterator iter = scene_widgets_.begin();
        for (; iter != scene_widgets_.end(); ++iter)
            (*iter)->show();

        view_->show();
    }
    
}

void UICanvas::Hide()
{
    if ( mode_ != External)
    {
        //todo use appearpolicy here.
        /*
        if ( use_fading_ )
        {
            fade_ = true;
            fade_on_hiding_ = true;
            current_dur_ = total_dur_;
            clock_.restart();
            return;
        }
        */
        view_->hide();

        QList<QGraphicsProxyWidget* >::iterator iter = scene_widgets_.begin();
        for (; iter != scene_widgets_.end(); ++iter)
            (*iter)->hide();
          
        container_->hide();
        overlay_->hide();
        
        dirty_ = true;
        emit Hidden();
        RenderSceneToOgreSurface();
        
    }
    else
    {
        view_->hide();
        emit Hidden();
    }
}

void UICanvas::AddWidget(QWidget* widget)
{ 
    QGraphicsScene* scene = view_->scene();
    scene_widgets_.append(scene->addWidget(widget));
}

void UICanvas::SetWindowTitle(const QString& title) 
{
	view_->setWindowTitle(title);
}

void UICanvas::SetWindowIcon(const QIcon& icon) 
{
	view_->setWindowIcon(icon);
}



void UICanvas::SetZOrder(int order)
{
    if ( mode_ != External)
        overlay_->setZOrder(order);
}

int UICanvas::GetZOrder() const
{
    if (mode_ != External)
        return overlay_->getZOrder();

    return -1;
}




void UICanvas::drawBackground(QPainter* painter, const QRectF &rect)
{
    QBrush black(Qt::transparent);
    painter->fillRect(rect, black);
}

void UICanvas::RenderSceneToOgreSurface()
{
    // Render if and only if scene is dirty.
    if (((!dirty_) && (!renderwindow_changed_)) || mode_ == External)
        return;

    PROFILE(RenderSceneToOgre);

    // We draw the GraphicsView area to an offscreen QPixmap and blit that onto the Ogre GPU surface.
    QPixmap pixmap(view_->size());
    {
        PROFILE(FillEmpty);
        pixmap.fill(Qt::transparent);
        
    }
    assert(pixmap.hasAlphaChannel());
    QImage img = pixmap.toImage();

    QPainter painter(&img);
    QRectF destRect(0, 0, pixmap.width(), pixmap.height());
    QRect sourceRect(0, 0, pixmap.width(), pixmap.height());
    {
        PROFILE(RenderUI);
        view_->render(&painter, destRect, sourceRect);
    }
    assert(img.hasAlphaChannel());

    ///\todo Can optimize an extra blit away if we paint directly onto the GPU surface.
    Ogre::Box dimensions(0,0, img.rect().width(), img.rect().height());
    Ogre::PixelBox pixel_box(dimensions, Ogre::PF_A8R8G8B8, (void*)img.bits());
    {
        PROFILE(UIToOgreBlit);
        Ogre::TexturePtr texture = Ogre::TextureManager::getSingleton().getByName(surfaceName_.toStdString().c_str());
        texture->getBuffer()->blitFromMemory(pixel_box);
    }

    if (renderwindow_changed_)
    {
        renderwindow_changed_ = false;
        dirty_ = true;
    }
    else
    {
        dirty_ = false;
    }
}
/*
void UICanvas::resizeEvent(QResizeEvent* event)
{
    if ( mode_ != External || scene_widgets_.size() != 1)
        return;

    scene_widgets_[0]->resize(event->size());

}
*/

void UICanvas::ResizeWidgets(int width, int height)
{
    // Hack for the logout-window: count rootlevel widgets and resize only if there's only one
    Core::uint root_widgets = 0;
    for (int i = 0; i < scene_widgets_.size(); ++i)
    {
        if ( scene_widgets_[i]->parent() == 0)
            root_widgets++;
    }
    
    if (root_widgets == 1)
    {
        for (int i = 0; i < scene_widgets_.size(); ++i)
        {
            if ( scene_widgets_[i]->parent() == 0)
                scene_widgets_[i]->resize(QSizeF(width, height));
        }
    }
}



void UICanvas::ResizeOgreTexture(int width, int height)
{
    Ogre::TexturePtr texture = Ogre::TextureManager::getSingleton().getByName(surfaceName_.toStdString().c_str());
    texture->freeInternalResources();
    texture->setWidth(width);
    texture->setHeight(height);
    texture->createInternalResources();
}

void UICanvas::CreateOgreResources(int width, int height)
{
    // If we've already created the resources, just resize the texture to a new size.
    if ( surfaceName_ != "")
    {
        Ogre::TexturePtr texture = Ogre::TextureManager::getSingleton().getByName(surfaceName_.toStdString().c_str());
        if (width == texture->getWidth() && height == texture->getHeight())
            return;
        ResizeOgreTexture(width, height);
        assert(overlay_);
        assert(container_);
        return;
    }

    QString overlayName = QString("over") + id_;
    overlay_ = Ogre::OverlayManager::getSingleton().create(overlayName.toStdString().c_str());

    QString containerName = QString("con") + id_;
    container_ = static_cast<Ogre::OverlayContainer*>(Ogre::OverlayManager::getSingleton()
                                     .createOverlayElement("Panel", containerName.toStdString().c_str()));

    // This is not done so currently -- tuoki 
    // Make the overlay cover 100% of the render window. Note that the UI surface will be 
    // rendered pixel perfect without stretching only if the GraphicsView surface dimension 
    // matches the render window size.
    //container_->setDimensions(1.0,1.0);

    container_->setPosition(0,0);

    // Add container in default overlay
    overlay_->add2D(container_);

    ///\todo Tell Ogre not to generate a mipmap chain. This texture only needs to have only one
    /// mip level.

    surfaceName_ = QString("tex") + id_;

    Ogre::TexturePtr texture = Ogre::TextureManager::getSingleton().createManual(surfaceName_.toStdString().c_str(), 
                                Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME, 
                                Ogre::TEX_TYPE_2D, width, height, 0, 
                                Ogre::PF_A8R8G8B8, Ogre::TU_DYNAMIC_WRITE_ONLY_DISCARDABLE);


    QString surfaceMaterial = QString("mat") + id_;
    Ogre::MaterialPtr material = Ogre::MaterialManager::getSingleton().create(surfaceMaterial.toStdString().c_str(),
                                Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);

    state_ = material->getTechnique(0)->getPass(0)->createTextureUnitState();
    material->getTechnique(0)->getPass(0)->setSceneBlending(Ogre::SBF_SOURCE_ALPHA, Ogre::SBF_ONE_MINUS_SOURCE_ALPHA);
    state_->setTextureName(surfaceName_.toStdString().c_str());

    // Generate pixel perfect texture. 
    float relWidth = (float)texture->getWidth()/double(renderWindowSize_.width());
    float relHeight = (float)texture->getHeight()/double(renderWindowSize_.height());
    container_->setDimensions(relWidth, relHeight);

    container_->setMaterialName(surfaceMaterial .toStdString().c_str());
    container_->setEnabled(true);
    container_->setColour(Ogre::ColourValue(1,1,1,1));
}
/*
void UICanvas::Fade(double timeSinceLastFrame )
{
    
    if ( !IsHidden() && state_ != 0 && !fade_on_hiding_)
    {
      
       // If fading in decrease alpha until it reaches 1.0
       state_->setAlphaOperation( Ogre::LBX_MODULATE, Ogre::LBS_MANUAL, Ogre::LBS_TEXTURE, alpha_);
       current_dur_ += timeSinceLastFrame;
       alpha_ = current_dur_ / total_dur_;
       if ( alpha_ > 1.0)   
       {
        
         lastTime_ = 0;
         alpha_ = 1.0;
         state_->setAlphaOperation( Ogre::LBX_MODULATE, Ogre::LBS_MANUAL, Ogre::LBS_TEXTURE, alpha_);
         fade_ = false;
       }
       
    }
    else if ( state_ != 0 && fade_on_hiding_)
    {
        // If fading out increase alpha until it reaches 0.0
        state_->setAlphaOperation( Ogre::LBX_MODULATE, Ogre::LBS_MANUAL, Ogre::LBS_TEXTURE, alpha_);
        current_dur_ -= timeSinceLastFrame;
	    alpha_ = current_dur_ / total_dur_;
	    if( alpha_ < 0.0 )
        {
           fade_ = false;
           
           hide();

            QList<QGraphicsProxyWidget* >::iterator iter = scene_widgets_.begin();
            for (; iter != scene_widgets_.end(); ++iter)
                (*iter)->hide();
          
            container_->hide();
            overlay_->hide();

            fade_on_hiding_ = false;
            emit Hidden();
        }
       
    }
    dirty_ = true;
   
}
*/

}
