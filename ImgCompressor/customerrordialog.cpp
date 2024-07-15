#include "customerrordialog.h"
#include <qtimer>
#include <QQuickWindow>


/////////////////////////////////
//
//  class CustomErrorDialog
//

CustomErrorDialog::CustomErrorDialog(QQmlEngine *pQMLEngine, QObject *pParentObj) :
    QObject(pParentObj),
    m_pQMLEngine(pQMLEngine),
    m_pDialogObject(nullptr)
{
    m_pComponent = new QQmlComponent(m_pQMLEngine, QUrl("qrc:/CustomErrorDialog.qml"), this);
}

CustomErrorDialog::~CustomErrorDialog()
{
    cleanup();
}

void CustomErrorDialog::show(const QString& title, const QString& msg)
{
    if (m_pComponent->isReady())
    {
        Q_ASSERT(m_pDialogObject == nullptr);
        m_pDialogObject = m_pComponent->create();

        if (m_pDialogObject)
        {
            QString captionStr = "Error";

            //m_pDialogObject->setProperty("WM_CLASS", QByteArray(title).constData());
            m_pDialogObject->setProperty("title", title);
            m_pDialogObject->setProperty("message", msg);
            QMetaObject::invokeMethod(m_pDialogObject, "show");

            // VMF: Set the title after a short delay, yet hard to say why it works this way, anyway, it works for now, for serious project would be better to impelment it in a smarter way
            QObject *pDialogObject = m_pDialogObject;
            QTimer::singleShot(100, [pDialogObject, captionStr]() {
                QQuickWindow *pWindow = qobject_cast<QQuickWindow*>(pDialogObject);
                if (pWindow)
                {
                    pWindow->setTitle(captionStr);
                }
            });

            // Connect dialog closure to cleanup
            connect(m_pDialogObject, SIGNAL(accepted()), this, SLOT(cleanup()));
            connect(m_pDialogObject, SIGNAL(rejected()), this, SLOT(cleanup()));
        }
    }
    else
    {
        // Print out errors if the component is not ready (tmp, for debugging)
        Q_ASSERT(false); //must never happen, debug using the loop below
        const auto errors = m_pComponent->errors();
        for (const auto& error : errors)
        {
            qDebug() << error.toString();
        }

        // If we couldn't create the dialog, we should self-destruct
        deleteLater();
    }
}

void CustomErrorDialog::cleanup()
{
    if (m_pDialogObject)
    {
        m_pDialogObject->deleteLater();
        m_pDialogObject = nullptr;
    }

    // Self-destruct after cleanup
    this->deleteLater();
}
