#ifndef DATABASE_OBJ_CPP
#define DATABASE_OBJ_CPP

#include "database_obj.h"
#include <QMetaType>
#include <QVariant>

using Ret_t = JsonRPCServer::JsonRPCRet_t;
using Params_t = JsonRPCServer::JsonRPCParams_t;
using enum JsonRPCServer::JsonRPCErrorCodes;

Q_INVOKABLE Ret_t DatabaseObj::addProperty(Params_t params)
{
    std::string name = "";
    if (!extractParamStr(std::forward<std::remove_reference_t<Params_t>>(params), "name", name)){
        return Ret_t{"", InvalidParams};
    }
    if (dynamicPropertyNames().contains(QByteArray::fromStdString(name))){
        return Ret_t{"", InvalidParams};
    }

    std::string type_str = "";
    if (!extractParamStr(std::forward<std::remove_reference_t<Params_t>>(params), "type", type_str)){
        return Ret_t{"", InvalidParams};
    }

    QMetaType meta_type = QMetaType::fromName(type_str);
    if (!meta_type.isValid()){
        return Ret_t{"", InvalidParams};
    }

    QVariant prop_val(meta_type, nullptr);
    if (!prop_val.isValid()){
        return Ret_t{"", InvalidParams};
    }

    this->setProperty(name.c_str(), prop_val);
    
    return Ret_t(QString("Property <%1> type <%2> added").arg(QString::fromStdString(name), QString::fromStdString(type_str)), NoError);
}

Q_INVOKABLE Ret_t DatabaseObj::deleteProperty(Params_t params)
{
    std::string name = "";
    if (!extractParamStr(std::forward<std::remove_reference_t<Params_t>>(params), "name", name)){
        return Ret_t{"", InvalidParams};
    }
    if (!dynamicPropertyNames().contains(QByteArray::fromStdString(name))){
        return Ret_t{"", InvalidParams};
    }

    this->setProperty(name.c_str(), QVariant());
    
    return Ret_t(QString("Property <%1> deleted").arg(QString::fromStdString(name)), NoError);
}

Q_INVOKABLE Ret_t DatabaseObj::listProperties(Params_t params)
{
    const QString format("Property <%1>, type: <%2>");
    QString ret = "";

    auto names = dynamicPropertyNames();
    for (const auto &name : names){
        ret += format.arg(name, this->property(name.constData()).typeName());
    }

    return Ret_t(ret, NoError);
}

Q_INVOKABLE Ret_t DatabaseObj::addChild(Params_t params)
{
    return Ret_t();
}

Q_INVOKABLE Ret_t DatabaseObj::deleteChild(Params_t params)
{
    return Ret_t();
}

Q_INVOKABLE Ret_t DatabaseObj::listChildren(Params_t params)
{
    return Ret_t();
}

bool DatabaseObj::extractParamStr(const Params_t &&params, const QString &key, std::string &str) const
{
    auto iter = params.constFind(key);
    if (iter == params.cend()){
        return false;
    }
    const QByteArray val = iter->toByteArray();
    if (val.isEmpty()){
        return false;
    }
    str = val.constData();

    return true;
}

#endif //DATABASE_OBJ_CPP