#include "database_obj.h"
#include <QMetaType>
#include <QVariant>
#include <QTimer>
#include <semaphore>

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

    notifyRefresh();
    
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

    notifyRefresh();
    
    return Ret_t(QString("Property <%1> deleted").arg(QString::fromStdString(name)), NoError);
}

Q_INVOKABLE Ret_t DatabaseObj::listProperties(Params_t params)
{
    const QString format("{Property <%1>, type: <%2>} ");
    QString ret = "";

    auto names = dynamicPropertyNames();
    for (const auto &name : names){
        ret += format.arg(name, this->property(name.constData()).typeName());
    }

    return Ret_t(ret, NoError);
}

Q_INVOKABLE Ret_t DatabaseObj::setPropertyAttr(Params_t params)
{
    std::string property = "";
    if (!extractParamStr(std::forward<std::remove_reference_t<Params_t>>(params), "property", property)){
        return Ret_t{"", InvalidParams};
    }
    std::string attribute = "";
    if (!extractParamStr(std::forward<std::remove_reference_t<Params_t>>(params), "attribute", attribute)){
        return Ret_t{"", InvalidParams};
    }
    
    auto data_iter = params.constFind("data");
    if (data_iter == params.cend()){
        return Ret_t{"", InvalidParams};
    }
    const QVariant &data = *data_iter;

    QVariant prop_v = this->property(property.c_str());
    QMetaType prop_t = prop_v.metaType();
    QObject *prop_copy = static_cast<QObject *>(prop_t.create(prop_v.data()));

    QVariant prop_copy_v = prop_copy->property(attribute.c_str());
    prop_copy_v = data;
    prop_copy->setProperty(attribute.c_str(), prop_copy_v);
    
    this->setProperty(property.c_str(), QVariant(prop_t, prop_copy));

    return Ret_t(QString("Attribute %1 set to %2").arg(attribute.c_str(), data.toString()), NoError);
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

DatabaseObj::DatabaseObj(const QString &name, QObject *parent):
QObject(parent),
refresh_semaphore_(0)
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

    QByteArray full_path_ping = full_path;
    QByteArray full_path_pong = full_path;
    full_path_ping.append("/page0.html");
    full_path_pong.append("/page1.html");
    

    html_ping_pong_[0].f_in = CleanUtils::autoCloseFileInPtr(new QFile(full_path_ping));
    html_ping_pong_[0].f_in->open(QIODeviceBase::NewOnly);
    html_ping_pong_[0].f_in->close();
    
    if (!CleanUtils::autoCloseFileOutPtr(std::fopen(full_path_ping.constData(), "rb"))){
        std::printf("Error opening html file %s\n", full_path_ping.constData());
        exit(EXIT_FAILURE);
    }
    

    html_ping_pong_[1].f_in = CleanUtils::autoCloseFileInPtr(new QFile(full_path_pong));
    html_ping_pong_[1].f_in->open(QIODeviceBase::NewOnly);
    html_ping_pong_[1].f_in->close();

    if (!CleanUtils::autoCloseFileOutPtr(std::fopen(full_path_pong.constData(), "rb"))){
        std::printf("Error opening html file %s\n", full_path_pong.constData());
        exit(EXIT_FAILURE);
    }

    startRefreshLoop();
}

DatabaseObj::PingPongClientAccess DatabaseObj::getHTMLFile()
{
    StatesPair state = std::atomic_load(&ping_pong_state_);
    htmlFileIO &page = state.first ? html_ping_pong_[0] : html_ping_pong_[1];

    FILE *f = std::fopen(page.f_in->fileName().toLocal8Bit().constData(), "rb");
    size_t len = page.f_in->size();

    return PingPongClientAccess(page.semaphores, f, len);
}

void DatabaseObj::startRefreshLoop()
{
    QTimer *loop_timer = new QTimer();
    loop_timer->setSingleShot(false);
    loop_timer->setInterval(0);
    loop_timer->start();

    refresh_thread_.start();
    loop_timer->moveToThread(&refresh_thread_);
    connect(&refresh_thread_, &QThread::finished, loop_timer, &QObject::deleteLater);

    connect(loop_timer, &QTimer::timeout, this, [&](){
        if (refresh_semaphore_.try_acquire()){
            refreshHtml();
        }
    }, Qt::BlockingQueuedConnection);
}

void DatabaseObj::refreshHtml()
{
    /* Not needed now, but potentially required */
    std::lock_guard<std::mutex> lock(ping_pong_lock_);

    StatesPair state = std::atomic_load(&ping_pong_state_);
    /* Modifying invalid page */
    htmlFileIO &page = state.first ? html_ping_pong_[1] : html_ping_pong_[0];
    
    /* Blocks if any clients still read from invalidated earlier page */
    while(page.semaphores.try_acquire());

    /* Clearing file */
    page.f_in->open(QIODeviceBase::WriteOnly | QIODeviceBase::Truncate/* | QIODeviceBase::Text*/);

    QDataStream filestream(page.f_in.get());
    
    constexpr char head[] = \
"<!DOCTYPE html>\n\
<html>\n\
<body>\n";

    constexpr char tail[] = \
"</body>\n\
</html>\n";

    filestream.writeRawData(head, sizeof(head) - 1);

    QVariant var;
    QMetaType type;
    for (const QByteArray &name : this->dynamicPropertyNames()){
        var = this->property(name);
        type = var.metaType();

        type.save(filestream, var.constData());
    }

    filestream.writeRawData(tail, sizeof(tail));

    page.f_in->close();
    state.flip();
    std::atomic_store(&ping_pong_state_, state);
}
