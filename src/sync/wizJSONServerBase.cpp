#include "wizJSONServerBase.h"
#include "share/wizEventLoop.h"
#include "rapidjson/document.h"

CWizJSONServerBase::CWizJSONServerBase()
    : m_net(std::make_shared<QNetworkAccessManager>())
{

}

CWizJSONServerBase::~CWizJSONServerBase()
{

}

int CWizJSONServerBase::returnCode()
{
    return m_nReturnCode;
}

QString CWizJSONServerBase::returnMessage()
{
    return m_strReturnMessage;
}

QString CWizJSONServerBase::jsonResult()
{
    return m_strJSONResult;
}

void CWizJSONServerBase::request(const QString& strUrl)
{
    m_nReturnCode = 0;
    m_strReturnMessage.clear();
    m_strJSONResult.clear();

    QNetworkReply* reply = m_net->get(QNetworkRequest(strUrl));
    qDebug() << "before create event loop";
    CWizAutoTimeOutEventLoop loop(reply);
    qDebug() << "after create event loop";
    loop.exec();
    //
    if (loop.timeOut() || loop.error() != QNetworkReply::NoError)
    {
        qDebug() << "[JSONRequest]Upload message delete status failed.";
        return;
    }

    m_strJSONResult = loop.result();
    rapidjson::Document d;
    d.Parse<0>(loop.result().toUtf8().constData());

    if (!d.HasMember("return_code"))
    {
        qDebug() << "[JSONRequest]Upload message delete status can not get return code ";
        return;
    }

    m_nReturnCode = d.FindMember("return_code")->value.GetInt();
    m_strReturnMessage = d.FindMember("return_message")->value.GetString();
    if (m_nReturnCode != JSON_RETURNCODE_OK )
    {
        qDebug() << "[JSONRequest]Upload message delete status error :  " << m_nReturnCode << loop.result();
    }
    else
    {
        qDebug() << "[JSONRequest]Upload message delete status OK";
    }
}
