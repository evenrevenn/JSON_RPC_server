#ifndef DATABASE_OBJ_H
#define DATABASE_OBJ_H

#include <QObject>
#include <QFile>
#include <QThread>
#include <array>
#include <semaphore>
#include "json_rpc_server.h"

namespace UserMetaTypes
{

class HyperLink : public QObject
{
    Q_OBJECT
public:
    HyperLink():QObject(nullptr),url_(),text_(){}
    HyperLink(const HyperLink &other):QObject(nullptr),url_(other.url_),text_(other.text_){}
    ~HyperLink(){}
    
    Q_INVOKABLE QByteArray toHtml() const
    {
        return "<a href=\"" + url_ + "\">" + text_ + "</a>";
    }
    Q_INVOKABLE void toDebug() const
    {
        QString str = "<a href=\"" + url_ + "\">" + text_ + "</a>";
        qDebug() << str;
    }
    operator const char *() const
    {
        ssize_t size = std::snprintf(nullptr, 0, "<a href=\"%s\">%s</a>", url_.constData(), text_.constData());
        char *buf = new char[size];
        size = std::snprintf(buf, size, "<a href=\"%s\">%s</a>", url_.constData(), text_.constData());
        return buf;
    }

    Q_PROPERTY(QByteArray url MEMBER url_);
    Q_PROPERTY(QByteArray text MEMBER text_);

private:
    QByteArray url_;
    QByteArray text_;

    friend QDebug & operator<<(QDebug &debug,const HyperLink & link){
        return debug << link.url_ << ',' << link.text_;
    }

    friend QDataStream & operator<<(QDataStream &stream, const HyperLink &link){
        ssize_t len = std::snprintf(nullptr, 0, "<a href=\"%s\">%s</a>", link.url_.constData(), link.text_.constData());
        auto buf = std::unique_ptr<char[]>(new char[len + 1]);

        len = std::snprintf(buf.get(), len + 1, "<a href=\"%s\">%s</a>", link.url_.constData(), link.text_.constData());
        stream.writeRawData(buf.get(), len);
        return stream;

        // return stream << "<a href=\"" << link.url_.constData() << "\">" << link.text_.constData() << "</a>";
    }
    friend QDataStream & operator>>(QDataStream &stream, const HyperLink &link)
    {
        /* TODO: add reading from stream */
        QString temp;
        return stream >> temp;
    }
};

class Text : public QObject
{
    Q_OBJECT
public:
    Text():QObject(nullptr),text_(){}
    Text(const Text &other):QObject(nullptr),text_(other.text_){}
    ~Text(){}

    Q_PROPERTY(QByteArray text MEMBER text_);
    
    Q_INVOKABLE QByteArray toHtml() const
    {
        return "<p>" + text_ + "</p>";
    }
    Q_INVOKABLE void toDebug(){
        qDebug() << text_ ;
    }

private:
    QByteArray text_;

    friend QDebug& operator<<(QDebug &debug,const Text & text_field){
        return debug << text_field.text_;
    }

    friend QDataStream & operator<<(QDataStream &stream, const Text &text_field){
        ssize_t len = std::snprintf(nullptr, 0, "<p>%s</p>", text_field.text_.constData());
        auto buf = std::unique_ptr<char[]>(new char[len + 1]);

        len = std::snprintf(buf.get(), len + 1, "<p>%s</p>", text_field.text_.constData());
        stream.writeRawData(buf.get(), len);
        return stream;

        // return stream << "<p>" << text_field.text_ << "</p>";
    }
    friend QDataStream & operator>>(QDataStream &stream, const Text &text_field)
    {
        /* TODO: add reading from stream */
        QString temp;
        return stream >> temp;
    }
};

class Header : public QObject
{
    Q_OBJECT
public:
    Header():QObject(nullptr),text_(){}
    Header(const Header &other):QObject(nullptr),text_(other.text_){}
    ~Header(){}

    Q_PROPERTY(QByteArray text MEMBER text_);
    Q_PROPERTY(int level MEMBER level_);

    Q_INVOKABLE QDataStream & toHtml(QDataStream &stream){
        return stream << "<h" << level_ << '>' << text_\
        << "</h" << level_ << '>';
    }
    Q_INVOKABLE void toDebug(){
        qDebug() << text_ << ',' << level_;
    }

private:
    QByteArray text_;
    int level_;

    friend QDebug& operator<<(QDebug &debug,const Header & header_field){
        return debug << header_field.text_ << ',' << header_field.level_;
    }

    friend QDataStream & operator<<(QDataStream &stream, const Header &header_field){
        ssize_t len = std::snprintf(nullptr, 0, "<h%d>%s</h%d>", header_field.level_, header_field.text_.constData(), header_field.level_);
        auto buf = std::unique_ptr<char[]>(new char[len + 1]);

        len = std::snprintf(buf.get(), len + 1, "<h%d>%s</h%d>", header_field.level_, header_field.text_.constData(), header_field.level_);
        stream.writeRawData(buf.get(), len);
        return stream;

        // return stream << "<h" << header_field.level_ << '>' << header_field.text_\
        // << "</h" << header_field.level_ << '>';
    }
    friend QDataStream & operator>>(QDataStream &stream, const Header &header_field)
    {
        /* TODO: add reading from stream */
        QString temp;
        return stream >> temp;
    }
};

/* Needed to use QMetaType::fromName() */
inline void registerTypes()
{
    qRegisterMetaType<UserMetaTypes::HyperLink>("HyperLink");
    qRegisterMetaType<UserMetaTypes::Text>("Text");
    qRegisterMetaType<UserMetaTypes::Header>("Header");
}

} //namespace UserMetaTypes

Q_DECLARE_METATYPE(UserMetaTypes::HyperLink);
Q_DECLARE_METATYPE(UserMetaTypes::Text);
Q_DECLARE_METATYPE(UserMetaTypes::Header);



namespace CleanUtils{
    class autoCloseFile{
    public:
        void operator()(FILE *file){fclose(file);}
        void operator()(QFile *file){file->close(); delete file;}
    };
    typedef std::unique_ptr<FILE, CleanUtils::autoCloseFile> autoCloseFileOutPtr;
    typedef std::unique_ptr<QFile, CleanUtils::autoCloseFile> autoCloseFileInPtr;
}


class DatabaseObj : public QObject
{
Q_OBJECT
public:
    explicit DatabaseObj(const QString &name, QObject *parent = nullptr);

    Q_INVOKABLE JsonRPCServer::JsonRPCRet_t addProperty(JsonRPCServer::JsonRPCParams_t params);
    Q_INVOKABLE JsonRPCServer::JsonRPCRet_t deleteProperty(JsonRPCServer::JsonRPCParams_t params);
    Q_INVOKABLE JsonRPCServer::JsonRPCRet_t listProperties(JsonRPCServer::JsonRPCParams_t params);

    Q_INVOKABLE JsonRPCServer::JsonRPCRet_t setPropertyAttr(JsonRPCServer::JsonRPCParams_t params);
    // Q_INVOKABLE JsonRPCServer::JsonRPCRet_t resetProperty(JsonRPCServer::JsonRPCParams_t params);

    Q_INVOKABLE JsonRPCServer::JsonRPCRet_t addChild(JsonRPCServer::JsonRPCParams_t params);
    Q_INVOKABLE JsonRPCServer::JsonRPCRet_t deleteChild(JsonRPCServer::JsonRPCParams_t params);
    Q_INVOKABLE JsonRPCServer::JsonRPCRet_t listChildren(JsonRPCServer::JsonRPCParams_t params);

    void notifyRefresh() {refresh_semaphore_.release();}
    
    typedef std::counting_semaphore<128> HtmlPingPongSemaphore_t;

/* RAII accessor to html file */
    class PingPongClientAccess{
    public:
        friend class DatabaseObj;
        operator FILE *(){return file_;}
        size_t length() const {return length_;}
        ~PingPongClientAccess(){ semaphore_.acquire(); }

    private:
        PingPongClientAccess(HtmlPingPongSemaphore_t &semaphores, FILE *f, size_t length):
        file_(f), length_(length), semaphore_(semaphores){ semaphore_.release(); }

        FILE *file_;
        const size_t length_;
        HtmlPingPongSemaphore_t &semaphore_;
    };
    PingPongClientAccess getHTMLFile();

private:

    /* Using rvalue for forwarding reference in case of using lvalue references as Params_t */
    bool extractParamStr(const JsonRPCServer::JsonRPCParams_t &&params, const QString &key, std::string &str) const;
    
    void startRefreshLoop();
    void refreshHtml();
    std::binary_semaphore refresh_semaphore_;
    QThread refresh_thread_;

    /** Ping pong for non-blocking client read */
    /* There is always one valid page file */
    struct htmlFileIO{
        htmlFileIO():semaphores(0){}
        CleanUtils::autoCloseFileInPtr f_in;
        HtmlPingPongSemaphore_t semaphores;
    };
    std::array<htmlFileIO, 2> html_ping_pong_;
    
    struct StatesPair{
        StatesPair():first(true),second(false){}
        void flip(){std::swap(first, second);}

        bool first, second;
    };
    std::atomic<StatesPair> ping_pong_state_;

    std::mutex ping_pong_lock_;
};

#endif //DATABASE_OBJ_H