﻿#include "WizToken.h"
#include "WizToken_p.h"

#include <QMutexLocker>
#include <QString>
#include <QDebug>

#include "WizKMServer.h"
#include "WizAsyncApi.h"
#include "WizApiEntry.h"
#include "share/WizThreads.h"

// use 5 minutes locally, server use 20 minutes
#define TOKEN_TIMEOUT_INTERVAL 60 * 5

WizTokenPrivate::WizTokenPrivate(WizToken* token)
    : m_bProcessing(false)
    , m_mutex(new QMutex(QMutex::Recursive))
    , q(token)

{
}

WizTokenPrivate::~WizTokenPrivate()
{
    delete m_mutex;
}

QString WizTokenPrivate::token()
{
    QMutexLocker locker(m_mutex);
    Q_UNUSED(locker);
    //
//    Q_ASSERT(!m_strUserId.isEmpty() && !m_strPasswd.isEmpty());

    if (m_info.strToken.isEmpty())
    {
        WizKMAccountsServer asServer(WizCommonApiEntry::syncUrl());
        if (asServer.login(m_strUserId, m_strPasswd))
        {
            m_info = asServer.getUserInfo();
            m_info.tTokenExpried = QDateTime::currentDateTime().addSecs(TOKEN_TIMEOUT_INTERVAL);
            return m_info.strToken;
        }
        else
        {
            m_lastErrorCode = asServer.getLastErrorCode();
            m_lastErrorMessage = asServer.getLastErrorMessage();
            return QString();
        }
    }
    //
    if (m_info.tTokenExpried >= QDateTime::currentDateTime())
    {
        return m_info.strToken;
    }
    else
    {
        WIZUSERINFO info;
        info.strToken = m_info.strToken;
        info.strKbGUID = m_info.strKbGUID;
        WizKMAccountsServer asServer(WizCommonApiEntry::syncUrl());
        asServer.setUserInfo(info);

        if (asServer.keepAlive(m_info.strToken))
        {
            m_info.tTokenExpried = QDateTime::currentDateTime().addSecs(TOKEN_TIMEOUT_INTERVAL);
            return m_info.strToken;
        }
        else
        {
            QString strToken;
            if (asServer.getToken(m_strUserId, m_strPasswd, strToken))
            {
                m_info.strToken = strToken;
                m_info.tTokenExpried = QDateTime::currentDateTime().addSecs(TOKEN_TIMEOUT_INTERVAL);
                return m_info.strToken;
            }
            else
            {
                m_lastErrorCode = asServer.getLastErrorCode();
                m_lastErrorMessage = asServer.getLastErrorMessage();
                return QString();
            }
        }
    }

    Q_ASSERT(0);
}

void WizTokenPrivate::requestToken()
{
    if (m_bProcessing)
    {
        qDebug() << "Querying token...";
        return;
    }
    //
    class GetTokenRunnable : public QObject
    {
        WizTokenPrivate* m_p;
    public:
        GetTokenRunnable(WizTokenPrivate* p)
            : m_p(p)
        {
        }

        void run()
        {
            m_p->m_bProcessing = true;
            QString token = m_p->token();
            m_p->m_bProcessing = false;
            Q_EMIT m_p->q->tokenAcquired(token);
        }
    };
    WizExecuteOnThread(WIZ_THREAD_NETWORK, [=](){
        GetTokenRunnable runnable(this);
        runnable.run();
    });
}

void WizTokenPrivate::clearToken()
{
    m_info.strToken.clear();
}

void WizTokenPrivate::clearLastError()
{
    m_lastErrorCode = 0;
    m_lastErrorMessage.clear();
}

void WizTokenPrivate::setUserId(const QString& strUserId)
{
    m_strUserId = strUserId;
}

void WizTokenPrivate::setPasswd(const QString& strPasswd)
{
    m_strPasswd = strPasswd;
}

WIZUSERINFO WizTokenPrivate::info()
{
    QMutexLocker locker(m_mutex);
    Q_UNUSED(locker);
    //
    return m_info;
}

int WizTokenPrivate::lastErrorCode() const
{
    QMutexLocker locker(m_mutex);
    Q_UNUSED(locker);
    //
    return m_lastErrorCode;
}

QString WizTokenPrivate::lastErrorMessage() const
{
    QMutexLocker locker(m_mutex);
    Q_UNUSED(locker);
    //
    return m_lastErrorMessage;
}


static WizTokenPrivate* d = 0;
static WizToken* m_instance = 0;

WizToken::WizToken(const QString& strUserId, const QString& strPasswd)
{
    m_instance = this;
    d = new WizTokenPrivate(this);

    d->setUserId(strUserId);
    d->setPasswd(strPasswd);
}

WizToken::~WizToken()
{
    delete d;
    d = 0;
}

WizToken* WizToken::instance()
{
    return m_instance;
}

QString WizToken::token()
{
    Q_ASSERT(m_instance);
    return d->token();
}

void WizToken::requestToken()
{
    Q_ASSERT(m_instance);

    d->requestToken();
}

void WizToken::clearToken()
{
    d->clearToken();
}

void WizToken::clearLastError()
{
    d->clearLastError();
}

void WizToken::setUserId(const QString& strUserId)
{
    Q_ASSERT(m_instance);

    d->setUserId(strUserId);
}

void WizToken::setPasswd(const QString& strPasswd)
{
    Q_ASSERT(m_instance);

    d->setPasswd(strPasswd);
}

WIZUSERINFO WizToken::info()
{
    Q_ASSERT(m_instance);

    return d->info();
}

QString WizToken::lastErrorMessage()
{
    Q_ASSERT(m_instance);

    return d->lastErrorMessage();
}

int WizToken::lastErrorCode()
{
    Q_ASSERT(m_instance);

    return d->lastErrorCode();
}
