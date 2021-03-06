// For conditions of distribution and use, see copyright notice in license.txt

#include <boost/smart_ptr.hpp>

#include "Framework.h"
#include "AttributeInterface.h"
#include "ComponentInterface.h"
#include "ServiceInterface.h"
#include "ServiceManager.h"
#include "Entity.h"
#include "SceneManager.h"

#include <QDomDocument>

namespace Foundation
{

ComponentInterface::ComponentInterface(Foundation::Framework *framework) : framework_(framework), parent_entity_(0)
{
}

ComponentInterface::ComponentInterface(const ComponentInterface &rhs) : QObject(), framework_(rhs.framework_), parent_entity_(rhs.parent_entity_)
{
}

ComponentInterface::~ComponentInterface()
{
}

void ComponentInterface::SetParentEntity(Scene::Entity* entity)
{
    parent_entity_ = entity;
}

Scene::Entity* ComponentInterface::GetParentEntity() const
{
    return parent_entity_;
}

QDomElement ComponentInterface::BeginSerialization(QDomDocument& doc, QDomElement& base_element) const
{
    QDomElement comp_element = doc.createElement("component");
    comp_element.setAttribute("type", QString::fromStdString(TypeName()));
    if (!name_.empty())
        comp_element.setAttribute("name", QString::fromStdString(name_));
    
    if (!base_element.isNull())
        base_element.appendChild(comp_element);
    else
        doc.appendChild(comp_element);
    
    return comp_element;
}

void ComponentInterface::WriteAttribute(QDomDocument& doc, QDomElement& comp_element, const std::string& name, const std::string& value) const
{
    QDomElement attribute_element = doc.createElement("attribute");
    attribute_element.setAttribute("name", QString::fromStdString(name));
    attribute_element.setAttribute("value", QString::fromStdString(value));
    comp_element.appendChild(attribute_element);
}

bool ComponentInterface::BeginDeserialization(QDomElement& comp_element)
{
    std::string type = comp_element.attribute("type").toStdString();
    if (type == TypeName())
    {
        SetName(comp_element.attribute("name").toStdString());
        return true;
    }
    return false;
}

std::string ComponentInterface::ReadAttribute(QDomElement& comp_element, const std::string& name) const
{
    QString name_str = QString::fromStdString(name);
    
    QDomElement attribute_element = comp_element.firstChildElement("attribute");
    while (!attribute_element.isNull())
    {
        if (attribute_element.attribute("name") == name_str)
            return attribute_element.attribute("value").toStdString();
        
        attribute_element = attribute_element.nextSiblingElement("attribute");
    }
    
    return std::string();
}

void ComponentInterface::ComponentChanged(Foundation::ChangeType change)
{
    change_ = change;
    
    if (parent_entity_)
    {
        Scene::SceneManager* scene = parent_entity_->GetScene();
        if (scene)
            scene->EmitComponentChanged(this, change);
    }
    
    // Trigger also internal change
    emit OnChanged();
}

void ComponentInterface::ResetChange()
{
    for (uint i = 0; i < attributes_.size(); ++i)
        attributes_[i]->ResetChange();
    
    change_ = Foundation::None;
}

void ComponentInterface::SerializeTo(QDomDocument& doc, QDomElement& base_element) const
{
    if (!IsSerializable())
        return;
    
    QDomElement comp_element = BeginSerialization(doc, base_element);
    
    for (uint i = 0; i < attributes_.size(); ++i)
    {
        WriteAttribute(doc, comp_element, attributes_[i]->GetNameString(), attributes_[i]->ToString());
    }
}

void ComponentInterface::DeserializeFrom(QDomElement& element, ChangeType change)
{
    if (!IsSerializable())
        return;
        
    if (!BeginDeserialization(element))
        return;
    
    for (uint i = 0; i < attributes_.size(); ++i)
    {
        std::string attr_str = ReadAttribute(element, attributes_[i]->GetNameString());
        attributes_[i]->FromString(attr_str, change);
    }
}

}
