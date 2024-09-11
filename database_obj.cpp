#include "database_obj.h"
#include <QMetaType>
#include <QVariant>
#include <QEvent>

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

DatabaseObj::DatabaseObj(const QString &name, QObject *parent):QObject(parent)
{
    setObjectName(name);

    QStringList path;
    path.append(name);

    QObject *next = parent;

    /* Upward run to root */
    while (next && next->parent()){
        path.prepend(next->objectName());
        next = next->parent();
    }
    QByteArray full_path = path.join('/').toLocal8Bit();

#ifdef _WIN32
    /* TODO: add windows mkdir */
#else
    struct stat st = {0};
    if (stat(full_path.constData(), &st) == -1){
        mkdir(full_path.constData(), 0777);
    }
#endif
    QByteArray full_path_ping = full_path;
    QByteArray full_path_pong = full_path;
    full_path_ping.append("page0.html");
    full_path_pong.append("page1.html");
    
    html_ping_pong_[0].f_out = CleanUtils::autoCloseFileOutPtr(fopen(full_path_ping.constData(), "r"));
    html_ping_pong_[0].f_in = CleanUtils::autoCloseFileInPtr(new QFile());
    html_ping_pong_[0].f_in->open(html_ping_pong_[0].f_out.get(), QIODeviceBase::Truncate, QFileDevice::AutoCloseHandle);
    
    html_ping_pong_[1].f_out = CleanUtils::autoCloseFileOutPtr(fopen(full_path_pong.constData(), "r"));
    html_ping_pong_[1].f_in = CleanUtils::autoCloseFileInPtr(new QFile());
    html_ping_pong_[1].f_in->open(html_ping_pong_[1].f_out.get(), QIODeviceBase::Truncate, QFileDevice::AutoCloseHandle);
}

bool DatabaseObj::event(QEvent *event)
{
    if (event->type() == QEvent::DynamicPropertyChange){
        refreshHtml();
        return true;
    }

    return false;
}

void DatabaseObj::refreshHtml()
{
    std::lock_guard<std::mutex> lock(ping_pong_lock_);

    StatesPair state = std::atomic_load(&ping_pong_state_);
    /* Modifying invalid page */
    htmlFileIO &page = state.first ? html_ping_pong_[0] : html_ping_pong_[1];
    
    /* Blocks if any clients still read from invalidated earlier page */
    while(page.semaphores.try_acquire());

    /* Clearing file */
    page.f_in->close();
    page.f_in->open(page.f_out.get(), QIODeviceBase::Truncate, QFileDevice::AutoCloseHandle);

    QDataStream filestream(page.f_in.get());
    
    filestream 
    << "<!DOCTYPE html>\n"
    << "<html>\n"
    << "<body>\n";

    QVariant var;
    QMetaType type;
    for (const QByteArray &name : this->dynamicPropertyNames()){
        var = this->property(name);
        type = var.metaType();

        type.save(filestream, var.constData());
    }

    filestream 
    << "</body>\n"
    << "</html>\n";

    state.flip();
    std::atomic_store(&ping_pong_state_, state);
}
